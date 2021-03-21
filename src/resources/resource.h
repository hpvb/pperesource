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

#include "resources/icon_group.h"
#include "resources/versioninfo.h"

typedef struct resource {
	uint32_t type_characteristics;
	uint32_t type_date_time_stamp;
	uint16_t type_major_version;
	uint16_t type_minor_version;

	uint32_t name_characteristics;
	uint32_t name_date_time_stamp;
	uint16_t name_major_version;
	uint16_t name_minor_version;

	uint32_t type_id;
	uint32_t name_id;
	uint32_t language_id;
	uint32_t codepage;
	uint32_t reserved;

	char *type;
	char *name;
	char *language;

	size_t size;
	uint8_t *data;
} resource_t;

typedef struct resource_table {
	size_t size;

	uint32_t characteristics;
	uint32_t date_time_stamp;
	uint16_t major_version;
	uint16_t minor_version;

	resource_t **resources;

	size_t numb_versioninfo;
	version_info_t *versioninfo;

	size_t numb_icon_group;
	icon_group_t *icongroups;
} resource_table_t;

void resource_table_free(resource_table_t *resource_table);
void resource_delete(resource_table_t *resource_table, resource_t *resource);

size_t resource_table_deserialize(const section_t *section, const size_t offset, resource_table_t *resource_table);
size_t resource_table_serialize(const section_t *section, const size_t offset, resource_table_t *resource_table);
void resource_table_print(resource_table_t *resource_table);
void update_resource_table(ppelib_file_t *pe);

size_t resource_get_by_type_name(char *type);
size_t resource_count_by_type_id(const resource_table_t *resource_table, uint32_t type);
resource_t *resource_get_by_type_id(const resource_table_t *resource_table, uint32_t type, size_t idx);

size_t resource_get_numb_versioninfo(const resource_table_t *resource_table);
version_info_t *resource_get_versioninfo(const resource_table_t *resource_table, size_t idx);

size_t resource_get_numb_icon_group(const resource_table_t *resource_table);
icon_group_t *resource_get_icon_group(const resource_table_t *resource_table, size_t idx);
#endif /* SRC_RESOURCES_RESOURCE_H_ */
