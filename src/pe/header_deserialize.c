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

#include "header_private.h"
#include "pe/constants.h"
#include "ppe_error.h"
#include "utils.h"

size_t ppelib_header_deserialize(const uint8_t *buffer, const size_t size,
		const size_t offset, header_t *header) {
	ppelib_reset_error();

	if (offset + 92 > size) {
		ppelib_set_error("Not enough space for COFF headers");
		return 0;
	}

	header->machine = read_uint16_t(buffer + offset + 0);
	header->number_of_sections = read_uint16_t(buffer + offset + 2);
	header->time_date_stamp = read_uint32_t(buffer + offset + 4);
	header->pointer_to_symbol_table = read_uint32_t(buffer + offset + 8);
	header->number_of_symbols = read_uint32_t(buffer + offset + 12);
	header->size_of_optional_header = read_uint16_t(buffer + offset + 16);
	header->characteristics = read_uint16_t(buffer + offset + 18);
	header->magic = read_uint16_t(buffer + offset + 20);
	header->major_linker_version = read_uint8_t(buffer + offset + 22);
	header->minor_linker_version = read_uint8_t(buffer + offset + 23);
	header->size_of_code = read_uint32_t(buffer + offset + 24);
	header->size_of_initialized_data = read_uint32_t(buffer + offset + 28);
	header->size_of_uninitialized_data = read_uint32_t(buffer + offset + 32);
	header->address_of_entry_point = read_uint32_t(buffer + offset + 36);
	header->base_of_code = read_uint32_t(buffer + offset + 40);
	header->section_alignment = read_uint32_t(buffer + offset + 52);
	header->file_alignment = read_uint32_t(buffer + offset + 56);
	header->major_operating_system_version = read_uint16_t(buffer + offset + 60);
	header->minor_operating_system_version = read_uint16_t(buffer + offset + 62);
	header->major_image_version = read_uint16_t(buffer + offset + 64);
	header->minor_image_version = read_uint16_t(buffer + offset + 66);
	header->major_subsystem_version = read_uint16_t(buffer + offset + 68);
	header->minor_subsystem_version = read_uint16_t(buffer + offset + 70);
	header->win32_version_value = read_uint32_t(buffer + offset + 72);
	header->size_of_image = read_uint32_t(buffer + offset + 76);
	header->size_of_headers = read_uint32_t(buffer + offset + 80);
	header->checksum = read_uint32_t(buffer + offset + 84);
	header->subsystem = read_uint16_t(buffer + offset + 88);
	header->dll_characteristics = read_uint16_t(buffer + offset + 90);

	if (header->magic == PE32_MAGIC) {
		if (offset + 116 > size) {
			ppelib_set_error("Not enough space for PE headers");
			return 0;
		}
		header->base_of_data = read_uint32_t(buffer + offset + 44);
		header->image_base = read_uint32_t(buffer + offset + 48);
		header->size_of_stack_reserve = read_uint32_t(buffer + offset + 92);
		header->size_of_stack_commit = read_uint32_t(buffer + offset + 96);
		header->size_of_heap_reserve = read_uint32_t(buffer + offset + 100);
		header->size_of_heap_commit = read_uint32_t(buffer + offset + 104);
		header->loader_flags = read_uint32_t(buffer + offset + 108);
		header->number_of_rva_and_sizes = read_uint32_t(buffer + offset + 112);

		return 116;
	} else if (header->magic == PE32PLUS_MAGIC) {
		if (offset + 132 > size) {
			ppelib_set_error("Not enough space for PE+ headers");
			return 0;
		}
		header->image_base = read_uint64_t(buffer + offset + 44);
		header->size_of_stack_reserve = read_uint64_t(buffer + offset + 92);
		header->size_of_stack_commit = read_uint64_t(buffer + offset + 100);
		header->size_of_heap_reserve = read_uint64_t(buffer + offset + 108);
		header->size_of_heap_commit = read_uint64_t(buffer + offset + 116);
		header->loader_flags = read_uint32_t(buffer + offset + 124);
		header->number_of_rva_and_sizes = read_uint32_t(buffer + offset + 128);

		return 132;
	} else {
		ppelib_set_error("Unknown magic type");
		return 0;
	}
}
