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

#ifndef PPELIB_HEADER_PRIVATE_H_
#define PPELIB_HEADER_PRIVATE_H_

#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>

#include "pe/constants.h"

typedef struct ppelib_file ppelib_file_t;

#define HEADER_SIZE 92

#define DATA_DIRECTORY_SIZE 8
#define PE_OPTIONAL_HEADER_SIZE 96
#define PEPLUS_OPTIONAL_HEADER_SIZE 112

#include "platform.h"
#include "utils.h"

typedef struct header {
	uint16_t machine;
	uint16_t number_of_sections;
	uint32_t time_date_stamp;
	uint32_t pointer_to_symbol_table;
	uint32_t number_of_symbols;
	uint16_t size_of_optional_header;
	uint16_t characteristics;
	uint16_t magic;
	uint8_t major_linker_version;
	uint8_t minor_linker_version;
	uint32_t size_of_code;
	uint32_t size_of_initialized_data;
	uint32_t size_of_uninitialized_data;
	uint32_t address_of_entry_point;
	uint32_t base_of_code;
	uint32_t base_of_data;
	uint64_t image_base;
	uint32_t section_alignment;
	uint32_t file_alignment;
	uint16_t major_operating_system_version;
	uint16_t minor_operating_system_version;
	uint16_t major_image_version;
	uint16_t minor_image_version;
	uint16_t major_subsystem_version;
	uint16_t minor_subsystem_version;
	uint32_t win32_version_value;
	uint32_t size_of_image;
	uint32_t size_of_headers;
	uint32_t checksum;
	uint16_t subsystem;
	uint16_t dll_characteristics;
	uint64_t size_of_stack_reserve;
	uint64_t size_of_stack_commit;
	uint64_t size_of_heap_reserve;
	uint64_t size_of_heap_commit;
	uint32_t loader_flags;
	uint32_t number_of_rva_and_sizes;
} header_t;

size_t header_serialize(const header_t *header, uint8_t *buffer, const size_t offset);
size_t header_deserialize(const uint8_t *buffer, const size_t size, const size_t offset, header_t *header);
void header_fprint(FILE *stream, const header_t *header);
void header_print(const header_t *header);
uint8_t header_is_null(const header_t *header);

#endif /* PPELIB_HEADER_PRIVATE_H_  */
