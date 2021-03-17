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

#ifndef SRC_RESOURCES_RESOURCE_TABLE_PRIVATE_H_
#define SRC_RESOURCES_RESOURCE_TABLE_PRIVATE_H_

#include <inttypes.h>

typedef struct resource_directory_table resource_directory_table_t;

typedef struct resource_data_entry {
	uint32_t data_rva;
	uint32_t data_size;
	uint32_t codepage;
	uint32_t reserved;

	uint8_t *data;
} resource_data_entry_t;

typedef struct resource_directory_entry {
	uint32_t name_id;
	uint32_t entry_offset;

	const char *name;
	resource_directory_table_t *directory_table;
	resource_data_entry_t *data_entry;
} resource_directory_entry_t;

typedef struct resource_directory_table {
	uint32_t characteristics;
	uint32_t time_date_stamp;
	uint16_t major_version;
	uint16_t minor_version;

	size_t number_of_entries;
	resource_directory_entry_t *entries;
} resource_directory_table_t;

#endif /* SRC_RESOURCES_RESOURCE_TABLE_PRIVATE_H_ */
