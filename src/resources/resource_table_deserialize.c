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

#include "main.h"
#include "platform.h"
#include "ppe_error.h"
#include "resources/resource.h"

thread_local static size_t rscs_base;

thread_local static uint32_t type_characteristics;
thread_local static uint32_t type_date_time_stamp;
thread_local static uint16_t type_major_version;
thread_local static uint16_t type_minor_version;

thread_local static uint32_t name_characteristics;
thread_local static uint32_t name_date_time_stamp;
thread_local static uint16_t name_major_version;
thread_local static uint16_t name_minor_version;

thread_local static uint32_t type;
thread_local static uint32_t name;
thread_local static uint32_t language;

static size_t parse_resource_table(const uint8_t *buffer, const size_t size, const size_t offset, resource_table_t *resource_table, uint32_t level);

static char *get_len_string(const uint8_t *buffer, const size_t size, const size_t offset) {
	if (offset > size) {
		ppelib_set_error("Can't read past end of buffer");
		return NULL;
	}

	if (offset + 2 > size) {
		ppelib_set_error("Not enough space for string");
		return NULL;
	}

	uint16_t string_size = read_uint16_t(buffer + offset);
	string_size *= 2;

	if (offset + 2 + string_size > size) {
		ppelib_set_error("Not enough space for string data");
		return NULL;
	}

	//printf("get_len_string: size: %i\n", string_size);
	return get_utf16_string(buffer, size, offset + 2, string_size);
}

static size_t parse_resource(const uint8_t *buffer, const size_t size, const size_t offset, resource_table_t *resource_table) {
	if (offset > size || size - offset < 16) {
		ppelib_set_error("Not enough space for resource data entry");
		return 0;
	}

	uint32_t data_rva = read_uint32_t(buffer + offset + 0);
	uint32_t data_size = read_uint32_t(buffer + offset + 4);
	uint32_t codepage = read_uint32_t(buffer + offset + 8);
	uint32_t reserved = read_uint32_t(buffer + offset + 12);

	char *type_s = NULL;
	char *name_s = NULL;
	char *language_s = NULL;

	size_t data_offset = data_rva - rscs_base;
	if (data_offset > size || data_offset + data_size > size) {
		ppelib_set_error("Not enough space for resource data");
		return 0;
	}

	if (CHECK_BIT(type, HIGH_BIT32)) {
		type_s = get_len_string(buffer, size, type ^ HIGH_BIT32);
		if (ppelib_error_peek()) {
			goto out;
		}
	}

	if (CHECK_BIT(name, HIGH_BIT32)) {
		name_s = get_len_string(buffer, size, name ^ HIGH_BIT32);
		if (ppelib_error_peek()) {
			goto out;
		}
	}

	if (CHECK_BIT(language, HIGH_BIT32)) {
		language_s = get_len_string(buffer, size, language ^ HIGH_BIT32);
		if (ppelib_error_peek()) {
			goto out;
		}
	}

	++resource_table->size;
	resource_table->resources = realloc(resource_table->resources, sizeof(resource_t *) * resource_table->size);
	if (!resource_table->resources) {
		ppelib_set_error("Failed to allocate resource");
		goto out;
	}

	resource_table->resources[resource_table->size - 1] = calloc(sizeof(resource_t), 1);
	resource_t *resource = resource_table->resources[resource_table->size - 1];

	resource->type_characteristics = type_characteristics;
	resource->type_date_time_stamp = type_date_time_stamp;
	resource->type_major_version = type_major_version;
	resource->type_minor_version = type_minor_version;

	resource->name_characteristics = name_characteristics;
	resource->name_date_time_stamp = name_date_time_stamp;
	resource->name_major_version = name_major_version;
	resource->name_minor_version = name_minor_version;

	if (type_s) {
		resource->type = type_s;
	} else {
		resource->type_id = type;
	}
	if (name_s) {
		resource->name = name_s;
	} else {
		resource->name_id = name;
	}
	if (language_s) {
		resource->language = language_s;
	} else {
		resource->language_id = language;
	}

	resource->codepage = codepage;
	resource->reserved = reserved;

	resource->size = data_size;
	resource->data = malloc(data_size);
	memcpy(resource->data, buffer + data_offset, data_size);

	return 0;

out:
	free(type_s);
	free(name_s);
	free(language_s);
	type_s = NULL;
	name_s = NULL;
	language_s = NULL;
	return 0;
}

static size_t parse_resource_entry(const uint8_t *buffer, const size_t size, const size_t offset, resource_table_t *resource_table, uint32_t level) {
	if (offset > size || size - offset < 8) {
		ppelib_set_error("Not enough space for resource directory entry");
		return 0;
	}

	uint32_t id = read_uint32_t(buffer + offset + 0);
	uint32_t next_offset = read_uint32_t(buffer + offset + 4);

	switch (level) {
	case 0:
		type = id;
		break;
	case 1:
		name = id;
		break;
	case 2:
		language = id;
		break;
	default:
		ppelib_set_error("Unexpected directory depth");
		return 0;
	}

	if (CHECK_BIT(next_offset, HIGH_BIT32)) {
		parse_resource_table(buffer, size, next_offset ^ HIGH_BIT32, resource_table, level + 1);
	} else {
		parse_resource(buffer, size, next_offset, resource_table);
	}

	return 0;
}

static size_t parse_resource_table(const uint8_t *buffer, const size_t size, const size_t offset, resource_table_t *resource_table, uint32_t level) {
	if (offset > size || size - offset < 16) {
		ppelib_set_error("Not enough space for resource directory table");
		return 0;
	}

	uint32_t characteristics = read_uint32_t(buffer + offset + 0);
	uint32_t date_time_stamp = read_uint32_t(buffer + offset + 4);
	uint16_t major_version = read_uint16_t(buffer + offset + 8);
	uint16_t minor_version = read_uint16_t(buffer + offset + 10);

	uint16_t number_of_name_entries = read_uint16_t(buffer + offset + 12);
	uint16_t number_of_id_entries = read_uint16_t(buffer + offset + 14);

	switch (level) {
	case 0:
		resource_table->characteristics = characteristics;
		resource_table->date_time_stamp = date_time_stamp;
		resource_table->major_version = major_version;
		resource_table->minor_version = minor_version;
		break;
	case 1:
		type_characteristics = characteristics;
		type_date_time_stamp = date_time_stamp;
		type_major_version = major_version;
		type_minor_version = minor_version;
		break;
	case 2:
		name_characteristics = characteristics;
		name_date_time_stamp = date_time_stamp;
		name_major_version = major_version;
		name_minor_version = minor_version;
		break;
	default:
		ppelib_set_error("Unexpected directory depth");
		return 0;
	}

	size_t entry_offset = offset + 16;

	for (uint16_t i = 0; i < number_of_name_entries + number_of_id_entries; ++i) {
		parse_resource_entry(buffer, size, entry_offset, resource_table, level);
		if (ppelib_error_peek()) {
			return 0;
		}

		entry_offset += 8;
	}

	return 0;
}

size_t resource_table_deserialize(const section_t *section, const size_t offset, resource_table_t *resource_table) {
	ppelib_reset_error();

	size_t size = section->contents_size;
	uint8_t *buffer = section->contents;
	rscs_base = section->virtual_address;

	if (size - offset < 16) {
		ppelib_set_error("Not enough space for resource directory table");
		return 0;
	}

	parse_resource_table(buffer, size, offset, resource_table, 0);
	if (ppelib_error_peek()) {
		return 0;
	}

	return 0;
}
