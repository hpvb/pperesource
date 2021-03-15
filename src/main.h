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

#ifndef PPELIB_MAIN_H_
#define PPELIB_MAIN_H_

typedef struct data_directory data_directory_t;

#include "pe/data_directory_private.h"
#include "pe/header_private.h"
#include "pe/section_private.h"
#include "resources/resource.h"

typedef struct ppelib_file {
	size_t start_of_section_va;
	size_t pe_header_offset;

	size_t start_of_section_data;
	size_t end_of_section_data;

	size_t entrypoint_offset;
	section_t *entrypoint_section;

	header_t header;
	data_directory_t *data_directories;

	section_t **sections;

	resource_table_t resource_table;

	size_t stub_size;
	uint8_t *stub;

	size_t overlay_size;
	uint8_t *overlay;
} ppelib_file_t;

EXPORT_SYM void ppelib_destroy(ppelib_file_t *pe);
EXPORT_SYM ppelib_file_t *ppelib_create_from_buffer(const uint8_t *buffer, size_t size);
EXPORT_SYM ppelib_file_t *ppelib_create_from_file(const char *filename);
EXPORT_SYM size_t ppelib_write_to_buffer(const ppelib_file_t *pe, uint8_t *buffer, size_t buf_size);
EXPORT_SYM size_t ppelib_write_to_file(const ppelib_file_t *pe, const char *filename);

void ppelib_recalculate(ppelib_file_t *pe);

#endif /* PPELIB_MAIN_H_ */
