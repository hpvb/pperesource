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
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "main.h"
#include "platform.h"
#include "ppe_error.h"
#include "resources/resource.h"
#include "resources/resource_table_private.h"
#include "resources/string_table.h"

thread_local static size_t rscs_base;
thread_local static size_t data_entries_offset;
thread_local static size_t data_offset;
thread_local static string_table_t string_table;
thread_local static size_t total_size;

int typecmp(const void *a, const void *b) {
	resource_t *ra = *(resource_t **)a;
	resource_t *rb = *(resource_t **)b;

	if (ra->type && rb->type) {
		return wcscmp(ra->type, rb->type);
	}

	if (!ra->type && !rb->type) {
		return (ra->type_id > rb->type_id);
	}

	if (ra->type) {
		return -1;
	} else {
		return 1;
	}
}

int namecmp(const void *a, const void *b) {
	resource_t *ra = *(resource_t **)a;
	resource_t *rb = *(resource_t **)b;

	if (ra->name && rb->name) {
		return wcscmp(ra->name, rb->name);
	}

	if (!ra->name && !rb->name) {
		return (ra->name_id > rb->name_id);
	}

	if (ra->name) {
		return -1;
	} else {
		return 1;
	}
}

int langcmp(const void *a, const void *b) {
	resource_t *ra = *(resource_t **)a;
	resource_t *rb = *(resource_t **)b;

	if (ra->language && rb->language) {
		return wcscmp(ra->language, rb->language);
	}

	if (!ra->language && !rb->language) {
		return (ra->language_id > rb->language_id);
	}

	if (ra->language) {
		return -1;
	} else {
		return 1;
	}
}

size_t resource_table_data_entries_number(const resource_directory_table_t *resource_table, size_t in_size) {
	size_t subdirs_size = 0;
	size_t data_entries_number = 0;

	for (uint32_t i = 0; i < resource_table->number_of_entries; ++i) {
		if (resource_table->entries[i].directory_table) {
			subdirs_size += resource_table_data_entries_number(resource_table->entries[i].directory_table, in_size);
		} else {
			++data_entries_number;
		}
	}

	return in_size + subdirs_size + data_entries_number;
}

size_t resource_table_directory_size(const resource_directory_table_t *resource_table, size_t in_size) {
	size_t subdirs_size = 0;
	if (!resource_table) {
		return 0;
	}

	for (uint32_t i = 0; i < resource_table->number_of_entries; ++i) {
		subdirs_size += resource_table_directory_size(resource_table->entries[i].directory_table, in_size);
	}

	size_t entries_size = resource_table->number_of_entries * 8u;

	return in_size + subdirs_size + 16 + entries_size;
}

size_t resource_table_write(const resource_directory_table_t *resource_table, uint8_t *buffer, size_t offset) {
	size_t furthest = offset;
	size_t next_entry = 0;

	size_t number_of_name_entries = 0;
	size_t number_of_id_entries = 0;

	for (size_t i = 0; i < resource_table->number_of_entries; ++i) {
		if (resource_table->entries[i].name) {
			number_of_name_entries++;
		} else {
			number_of_id_entries++;
		}
	}

	if (number_of_name_entries > UINT16_MAX) {
		ppelib_set_error("Too many name entries");
		return 0;
	}

	if (number_of_id_entries > UINT16_MAX) {
		ppelib_set_error("Too many id entries");
		return 0;
	}

	if (buffer) {
		uint8_t *table = buffer + offset;

		write_uint32_t(table + 0, resource_table->characteristics);
		write_uint32_t(table + 4, resource_table->time_date_stamp);
		write_uint16_t(table + 8, resource_table->major_version);
		write_uint16_t(table + 10, resource_table->minor_version);
		write_uint16_t(table + 12, (uint16_t)number_of_name_entries);
		write_uint16_t(table + 14, (uint16_t)number_of_id_entries);
	}
	offset += 16;

	furthest = MAX(furthest, offset);
	next_entry = offset + ((number_of_name_entries + number_of_id_entries) * 8);

	for (size_t i = 0; i < resource_table->number_of_entries; ++i) {
		const resource_directory_entry_t *e = &resource_table->entries[i];
		const resource_data_entry_t *d = resource_table->entries[i].data_entry;
		const resource_directory_table_t *t = resource_table->entries[i].directory_table;

		uint32_t name_offset_or_id = 0;

		if (e->name) {
			string_table_string_t *string = string_table_find(&string_table, e->name);
			name_offset_or_id = (string_table.base_offset + string->offset) ^ HIGH_BIT32;
		} else {
			name_offset_or_id = e->name_id;
		}

		if (t) {
			if (next_entry > UINT32_MAX) {
				ppelib_set_error("Sub-directory offset out of range");
				return 0;
			}

			uint32_t entry_offset = (uint32_t)next_entry;
			next_entry += resource_table_directory_size(t, 0);

			if (buffer) {
				uint8_t *entry = buffer + offset;

				write_uint32_t(entry + 0, name_offset_or_id);
				write_uint32_t(entry + 4, entry_offset ^ HIGH_BIT32);
			}

			size_t t_furthest = resource_table_write(t, buffer, entry_offset);
			offset += 8;
			furthest = MAX(furthest, t_furthest);
			furthest = MAX(furthest, offset);

		} else {
			if (data_entries_offset > UINT32_MAX) {
				ppelib_set_error("Data entry offset out of range");
				return 0;
			}

			uint32_t entry_offset = (uint32_t)data_entries_offset;

			if (buffer) {
				uint8_t *entry = buffer + offset;

				write_uint32_t(entry + 0, name_offset_or_id);
				write_uint32_t(entry + 4, entry_offset);

				write_uint32_t(buffer + entry_offset + 0, (uint32_t)rscs_base + (uint32_t)data_offset);
				write_uint32_t(buffer + entry_offset + 4, d->data_size);
				write_uint32_t(buffer + entry_offset + 8, d->codepage);
				write_uint32_t(buffer + entry_offset + 12, d->reserved);

				memcpy(buffer + data_offset, d->data, d->data_size);
			}

			data_entries_offset = entry_offset + 16;
			data_offset = TO_NEAREST(data_offset + d->data_size, 8);

			offset += 8;

			furthest = MAX(furthest, data_offset);
			furthest = MAX(furthest, offset);
		}
	}

	return furthest;
}

resource_directory_table_t *get_or_create_table(resource_directory_table_t *base, const wchar_t *name, const uint32_t id) {
	for (uint32_t i = 0; i < base->number_of_entries; ++i) {
		if (name && base->entries[i].name) {
			if (wcscmp(base->entries[i].name, name) == 0) {
				return base->entries[i].directory_table;
			}
		} else {
			if (base->entries[i].name_id == id) {
				return base->entries[i].directory_table;
			}
		}
	}

	resource_directory_entry_t *directory_entry;
	++base->number_of_entries;
	base->entries = realloc(base->entries, sizeof(resource_directory_entry_t) * base->number_of_entries);
	directory_entry = &base->entries[base->number_of_entries - 1];
	memset(directory_entry, 0, sizeof(resource_directory_entry_t));

	directory_entry->name = name;
	directory_entry->name_id = id;

	directory_entry->directory_table = calloc(sizeof(resource_directory_table_t), 1);
	return directory_entry->directory_table;
}

void resource_create(resource_directory_table_t *root, resource_t *resource) {
	resource_directory_table_t *type_table = NULL;
	resource_directory_table_t *name_table = NULL;

	type_table = get_or_create_table(root, resource->type, resource->type_id);
	type_table->characteristics = resource->type_characteristics;
	type_table->time_date_stamp = resource->type_date_time_stamp;
	type_table->major_version = resource->type_major_version;
	type_table->minor_version = resource->type_minor_version;

	name_table = get_or_create_table(type_table, resource->name, resource->name_id);
	name_table->characteristics = resource->name_characteristics;
	name_table->time_date_stamp = resource->name_date_time_stamp;
	name_table->major_version = resource->name_major_version;
	name_table->minor_version = resource->name_minor_version;

	resource_directory_entry_t *directory_entry;
	++name_table->number_of_entries;
	name_table->entries = realloc(name_table->entries, sizeof(resource_directory_entry_t) * name_table->number_of_entries);
	directory_entry = &name_table->entries[name_table->number_of_entries - 1];

	memset(directory_entry, 0, sizeof(resource_directory_entry_t));

	directory_entry->name = resource->language;
	directory_entry->name_id = resource->language_id;

	directory_entry->data_entry = calloc(sizeof(resource_data_entry_t), 1);
	resource_data_entry_t *data_entry = directory_entry->data_entry;

	data_entry->codepage = resource->codepage;
	data_entry->data_size = (uint32_t)resource->size;
	data_entry->data = resource->data;
}

void resource_directory_free(resource_directory_table_t *base) {
	if (!base) {
		return;
	}

	for (size_t i = 0; i < base->number_of_entries; ++i) {
		resource_directory_free(base->entries[i].directory_table);
		free(base->entries[i].data_entry);
	}

	free(base->entries);
	free(base);
}

size_t resource_table_serialize(const section_t *section, const size_t offset, resource_table_t *resource_table) {
	ppelib_reset_error();

	if (section) {
		rscs_base = section->virtual_address;
	} else {
		rscs_base = 0;
	}

	if (!resource_table->size) {
		return 0;
	}

	qsort(resource_table->resources, resource_table->size, sizeof(resource_t *), &langcmp);
	qsort(resource_table->resources, resource_table->size, sizeof(resource_t *), &namecmp);
	qsort(resource_table->resources, resource_table->size, sizeof(resource_t *), &typecmp);

	resource_directory_table_t *root = calloc(sizeof(resource_directory_table_t), 1);
	memset(&string_table, 0, sizeof(string_table));
	root->characteristics = resource_table->characteristics;
	root->time_date_stamp = resource_table->date_time_stamp;
	root->major_version = resource_table->major_version;
	root->minor_version = resource_table->minor_version;

	for (size_t i = 0; i < resource_table->size; ++i) {
		resource_t *resource = resource_table->resources[i];
		resource_create(root, resource);

		if (resource->type) {
			string_table_put(&string_table, resource->type);
		}

		if (resource->name) {
			string_table_put(&string_table, resource->name);
		}

		if (resource->language) {
			string_table_put(&string_table, resource->language);
		}
	}

	size_t string_table_offset = resource_table_directory_size(root, 0);

	string_table.base_offset = (uint32_t)string_table_offset;

	size_t data_entries = resource_table_data_entries_number(root, 0);
	data_entries_offset = TO_NEAREST(string_table_offset + string_table.bytes, 4);
	data_offset = data_entries_offset + (data_entries * 16);

	if (section) {
		memset(section->contents, 0, section->contents_size);
		total_size = resource_table_write(root, section->contents, offset);
		string_table_serialize(&string_table, section->contents + offset);
	} else {
		total_size = resource_table_write(root, NULL, offset);
	}

	string_table_free(&string_table);
	resource_directory_free(root);

	return total_size;
}

void update_resource_table(ppelib_file_t *pe) {
	ppelib_recalculate(pe);
	section_t *section = NULL;

	if (pe->header.number_of_rva_and_sizes > DIR_RESOURCE_TABLE) {
		section = pe->data_directories[DIR_RESOURCE_TABLE].section;
	}

	if (section) {
		resource_table_serialize(section, 0, &pe->resource_table);
	}
}
