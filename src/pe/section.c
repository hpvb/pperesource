/* Copyright 2021 Hein-Pieter van Braam-Stewart
 *
 * This file is part of ppelib (Portable Portable Executable LIBrary)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "platform.h"
#include "ppe_error.h"

#include "section_private.h"

static int sectioncmp(const void *a, const void *b) {
	section_t *sa = *(section_t **)a;
	section_t *sb = *(section_t **)b;

	return (sa->virtual_address > sb->virtual_address);
}

void sort_sections(ppelib_file_t *pe) {
	qsort(pe->sections, pe->header.number_of_sections, sizeof(section_t *), &sectioncmp);
}

size_t section_rva_to_offset(const section_t *section, size_t rva) {
	size_t rva_base = 0;

	if (section->pointer_to_raw_data > section->virtual_address) {
		rva_base = section->pointer_to_raw_data - section->virtual_address;
	} else {
		rva_base = section->virtual_address - section->pointer_to_raw_data;
	}
	rva_base += section->pointer_to_raw_data;

	if (rva - rva_base > section->contents_size) {
		ppelib_set_error("RVA out of range");
		return 0;
	}

	return rva - rva_base;
}

void *section_rva_to_pointer(const section_t *section, size_t rva) {
	size_t offset = section_rva_to_offset(section, rva);

	return section->contents + offset;
}

section_t *section_find_by_virtual_address(ppelib_file_t *pe, size_t va) {
	for (uint16_t i = 0; i < pe->header.number_of_sections; ++i) {
		section_t *section = pe->sections[i];
		size_t section_va_end = section->virtual_address + section->size_of_raw_data;

		if (section->virtual_address <= va && section_va_end > va) {
			return section;
		}
	}

	return NULL;
}

section_t *section_find_by_physical_address(ppelib_file_t *pe, size_t address) {
	for (uint16_t i = 0; i < pe->header.number_of_sections; ++i) {
		section_t *section = pe->sections[i];
		size_t section_va_end = section->pointer_to_raw_data + section->contents_size;

		if (section->pointer_to_raw_data <= address && section_va_end >= address) {
			return section;
		}
	}

	return NULL;
}

uint16_t section_create(ppelib_file_t *pe, char name[9], uint32_t virtual_size, uint32_t raw_size,
		uint32_t characteristics, uint8_t *data) {
	ppelib_reset_error();

	size_t name_size = strnlen(name, 9);
	if (name_size == 9) {
		ppelib_set_error("Section name not NULL terminated");
		return 0;
	}

	if (name_size == 0) {
		ppelib_set_error("Section name is NULL");
		return 0;
	}

	void *old_ptr = pe->sections;
	pe->sections = realloc(pe->sections, (pe->header.number_of_sections + 1) * sizeof(void *));
	if (!pe->sections) {
		pe->sections = old_ptr;
		ppelib_set_error("Couldn't allocate section");
		return 0;
	}

	pe->sections[pe->header.number_of_sections] = calloc(sizeof(section_t), 1);
	if (!pe->sections[pe->header.number_of_sections]) {
		pe->sections = realloc(pe->sections, pe->header.number_of_sections);
		ppelib_set_error("Couldn't allocate section");
		return 0;
	}

	section_t *section = pe->sections[pe->header.number_of_sections];
	if (raw_size) {
		section->contents = malloc(raw_size);
		if (!section->contents) {
			free(section);
			pe->sections = realloc(pe->sections, pe->header.number_of_sections);
			ppelib_set_error("Couldn't allocate section");
			return 0;
		}
		section->contents_size = raw_size;

		if (data) {
			memcpy(section->contents, data, raw_size);
		}
	}

	pe->header.number_of_sections++;
	strcpy(section->name, name);
	section->name[8] = 0;

	section->virtual_size = virtual_size;
	section->size_of_raw_data = raw_size;
	section->characteristics = characteristics;

	return pe->header.number_of_sections - 1;
}

void section_excise(ppelib_file_t *pe, uint16_t section_index, size_t start, size_t end) {
	ppelib_reset_error();

	if (section_index > pe->header.number_of_sections) {
		ppelib_set_error("Section index out of range");
		return;
	}

	section_t *section = pe->sections[section_index];

	if (end > section->contents_size) {
		ppelib_set_error("Can't delete past section end");
		return;
	}

	if (start >= end) {
		return;
	}

	if (end - start > UINT32_MAX) {
		ppelib_set_error("Section offset out of range");
		return;
	}

	uint16_t retval = buffer_excise(&pe->sections[section_index]->contents, section->contents_size, start, end);
	if (!retval) {
		ppelib_set_error("Failed to allocate new section contents");
		return;
	}

	section->contents_size -= (end - start);
}

void section_insert_capacity(ppelib_file_t *pe, uint16_t section_index, size_t size, size_t offset) {
	ppelib_reset_error();

	if (section_index > pe->header.number_of_sections) {
		ppelib_set_error("Section index out of range");
		return;
	}

	if (size > UINT32_MAX) {
		ppelib_set_error("Section size out of range");
		return;
	}

	section_t *section = pe->sections[section_index];

	if (section->contents_size + size > UINT32_MAX) {
		ppelib_set_error("Section size out of range");
		return;
	}

	if (offset > section->contents_size) {
		ppelib_set_error("Can't insert space after total size");
		return;
	}

	uint8_t *oldptr = section->contents;
	section->contents = realloc(section->contents, section->contents_size + size);
	if (!section->contents) {
		ppelib_set_error("Failed to allocate new section contents");
		section->contents = oldptr;
		return;
	}

	if (offset != section->contents_size) {
		memmove(section->contents + offset + size, section->contents + offset, section->contents_size - offset);
	}

	section->contents_size += size;
}

void section_resize(ppelib_file_t *pe, uint16_t section_index, size_t size) {
	ppelib_reset_error();

	if (section_index > pe->header.number_of_sections) {
		ppelib_set_error("Section index out of range");
		return;
	}

	if (size > UINT32_MAX) {
		ppelib_set_error("Section size out of range");
		return;
	}

	section_t *section = pe->sections[section_index];

	if (size == section->contents_size) {
		return;
	}

	if (size < section->contents_size) {
		section_excise(pe, section_index, size, section->contents_size);
		return;
	}

	uint8_t *oldptr = section->contents;
	section->contents = realloc(section->contents, size);
	if (!section->contents) {
		ppelib_set_error("Failed to allocate new section contents");
		section->contents = oldptr;
		return;
	}

	section->contents_size = size;
}

uint16_t section_find_index(ppelib_file_t *pe, section_t *section) {
	ppelib_reset_error();

	for (uint16_t i = 0; i < pe->header.number_of_sections; ++i) {
		if (pe->sections[i] == section) {
			return i;
		}
	}

	ppelib_set_error("Section not found");
	return 0;
}
