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

#ifndef SRC_RESOURCES_RESOURCE_H_
#define SRC_RESOURCES_RESOURCE_H_

#include <inttypes.h>
#include <stddef.h>

#include "pe/constants.h"
#include "pe/section_private.h"

typedef struct resource {
	uint32_t characteristics;
	uint32_t date_time_stamp;
	uint16_t major_version;
	uint16_t minor_version;

	uint32_t type_id;
	uint32_t name_id;
	uint32_t language_id;
	uint32_t codepage;
	uint32_t reserved;

	wchar_t *type;
	wchar_t *name;
	wchar_t *language;

	size_t size;
	uint8_t *data;
} resource_t;

typedef struct resource_table {
	size_t size;

	resource_t **resources;
} resource_table_t;

size_t resource_table_deserialize(const section_t *section, const size_t offset, resource_table_t *resource_table);
void resource_table_print(resource_table_t *resource_table);

#endif /* SRC_RESOURCES_RESOURCE_H_ */
