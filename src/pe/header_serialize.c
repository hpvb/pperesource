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

#include "pe/constants.h"
#include "ppe_error.h"

#include "utils.h"

#include "header_private.h"

size_t header_serialize(const header_t *header, uint8_t *buffer, const size_t offset) {
	ppelib_reset_error();

	if (!buffer) {
		goto out;
	}

	write_uint16_t(buffer + offset + 0, header->machine);
	write_uint16_t(buffer + offset + 2, header->number_of_sections);
	write_uint32_t(buffer + offset + 4, header->time_date_stamp);
	write_uint32_t(buffer + offset + 8, header->pointer_to_symbol_table);
	write_uint32_t(buffer + offset + 12, header->number_of_symbols);
	write_uint16_t(buffer + offset + 16, header->size_of_optional_header);
	write_uint16_t(buffer + offset + 18, header->characteristics);
	write_uint16_t(buffer + offset + 20, header->magic);
	write_uint8_t(buffer + offset + 22, header->major_linker_version);
	write_uint8_t(buffer + offset + 23, header->minor_linker_version);
	write_uint32_t(buffer + offset + 24, header->size_of_code);
	write_uint32_t(buffer + offset + 28, header->size_of_initialized_data);
	write_uint32_t(buffer + offset + 32, header->size_of_uninitialized_data);
	write_uint32_t(buffer + offset + 36, header->address_of_entry_point);
	write_uint32_t(buffer + offset + 40, header->base_of_code);
	write_uint32_t(buffer + offset + 52, header->section_alignment);
	write_uint32_t(buffer + offset + 56, header->file_alignment);
	write_uint16_t(buffer + offset + 60, header->major_operating_system_version);
	write_uint16_t(buffer + offset + 62, header->minor_operating_system_version);
	write_uint16_t(buffer + offset + 64, header->major_image_version);
	write_uint16_t(buffer + offset + 66, header->minor_image_version);
	write_uint16_t(buffer + offset + 68, header->major_subsystem_version);
	write_uint16_t(buffer + offset + 70, header->minor_subsystem_version);
	write_uint32_t(buffer + offset + 72, header->win32_version_value);
	write_uint32_t(buffer + offset + 76, header->size_of_image);
	write_uint32_t(buffer + offset + 80, header->size_of_headers);
	write_uint32_t(buffer + offset + 84, header->checksum);
	write_uint16_t(buffer + offset + 88, header->subsystem);
	write_uint16_t(buffer + offset + 90, header->dll_characteristics);

	if (header->magic == PE32_MAGIC) {
		write_uint32_t(buffer + offset + 44, (uint32_t)header->base_of_data);
		write_uint32_t(buffer + offset + 48, (uint32_t)header->image_base);
		write_uint32_t(buffer + offset + 92, (uint32_t)header->size_of_stack_reserve);
		write_uint32_t(buffer + offset + 96, (uint32_t)header->size_of_stack_commit);
		write_uint32_t(buffer + offset + 100, (uint32_t)header->size_of_heap_reserve);
		write_uint32_t(buffer + offset + 104, (uint32_t)header->size_of_heap_commit);
		write_uint32_t(buffer + offset + 108, (uint32_t)header->loader_flags);
		write_uint32_t(buffer + offset + 112, (uint32_t)header->number_of_rva_and_sizes);

	} else if (header->magic == PE32PLUS_MAGIC) {
		write_uint64_t(buffer + offset + 44, header->image_base);
		write_uint64_t(buffer + offset + 92, header->size_of_stack_reserve);
		write_uint64_t(buffer + offset + 100, header->size_of_stack_commit);
		write_uint64_t(buffer + offset + 108, header->size_of_heap_reserve);
		write_uint64_t(buffer + offset + 116, header->size_of_heap_commit);
		write_uint32_t(buffer + offset + 124, header->loader_flags);
		write_uint32_t(buffer + offset + 128, header->number_of_rva_and_sizes);

	} else {
		ppelib_set_error("Unknown magic type");
		return 0;
	}

out:
	switch (header->magic) {
	case PE32_MAGIC:
		return 116;
	case PE32PLUS_MAGIC:
		return 132;
	default:
		return 0;
	}
}
