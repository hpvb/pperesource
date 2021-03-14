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

thread_local size_t rscs_base;

thread_local uint32_t characteristics;
thread_local uint32_t date_time_stamp;
thread_local uint16_t major_version;
thread_local uint16_t minor_version;

thread_local uint32_t type;
thread_local uint32_t name;
thread_local uint32_t language;

size_t parse_resource_table(const uint8_t *buffer, const size_t size, const size_t offset, resource_table_t *resource_table, uint32_t level);

wchar_t *get_string(const uint8_t *buffer, size_t size, size_t offset) {
	if (offset + 2 > size) {
		ppelib_set_error("Section too small for string");
		return NULL;
	}

	uint16_t s_size = read_uint16_t(buffer + offset + 0);

	if (offset + 2u + (s_size * 2u) > size) {
		ppelib_set_error("Section too small for string");
		return NULL;
	}

	wchar_t *string = calloc((s_size + 1u) * sizeof(wchar_t), 1);
	if (!string) {
		ppelib_set_error("Failed to allocate string");
		return NULL;
	}

	if (sizeof(wchar_t) == 2) {
		memcpy(string, buffer + 2u, s_size * 2u);
	} else {
		for (uint16_t i = 0; i < s_size; ++i) {
			memcpy(string + i, buffer + offset + 2 + (i * 2), 2);
		}
	}

	return string;
}

size_t parse_resource(const uint8_t *buffer, const size_t size, const size_t offset, resource_table_t *resource_table) {
	if (offset > size || size - offset < 16) {
		ppelib_set_error("Not enough space for resource data entry");
		return 0;
	}

	uint32_t data_rva = read_uint32_t(buffer + offset + 0);
	uint32_t data_size = read_uint32_t(buffer + offset + 4);
	uint32_t codepage = read_uint32_t(buffer + offset + 8);
	uint32_t reserved = read_uint32_t(buffer + offset + 12);

	wchar_t *type_s = NULL;
	wchar_t *name_s = NULL;
	wchar_t *language_s = NULL;

	size_t data_offset = data_rva - rscs_base;
	if (data_offset > size || data_offset + data_size > size) {
		ppelib_set_error("Not enough space for resource data");
		return 0;
	}

	if (CHECK_BIT(type, HIGH_BIT32)) {
		type_s = get_string(buffer, size, type ^ HIGH_BIT32);
	}

	if (CHECK_BIT(name, HIGH_BIT32)) {
		name_s = get_string(buffer, size, name ^ HIGH_BIT32);
	}

	if (CHECK_BIT(language, HIGH_BIT32)) {
		language_s = get_string(buffer, size, language ^ HIGH_BIT32);
	}

	if (ppelib_error_peek()) {
		goto out;
	}

	++resource_table->size;
	resource_table->resources = realloc(resource_table->resources, sizeof(resource_t *) * resource_table->size);
	if (!resource_table->resources) {
		ppelib_set_error("Failed to allocate resource");
		goto out;
	}

	resource_table->resources[resource_table->size - 1] = calloc(sizeof(resource_t), 1);
	if (!resource_table->resources[resource_table->size - 1]) {
		ppelib_set_error("Failed to allocate resource");
		goto out;
	}

	resource_t *resource = resource_table->resources[resource_table->size - 1];

	resource->characteristics = characteristics;
	resource->date_time_stamp = date_time_stamp;
	resource->major_version = major_version;
	resource->minor_version = minor_version;

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
	return 0;
}

size_t parse_resource_entry(const uint8_t *buffer, const size_t size, const size_t offset, resource_table_t *resource_table, uint32_t level) {
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

size_t parse_resource_table(const uint8_t *buffer, const size_t size, const size_t offset, resource_table_t *resource_table, uint32_t level) {
	if (offset > size || size - offset < 16) {
		ppelib_set_error("Not enough space for resource directory table");
		return 0;
	}

	characteristics = read_uint32_t(buffer + offset + 0);
	date_time_stamp = read_uint32_t(buffer + offset + 4);
	major_version = read_uint16_t(buffer + offset + 8);
	minor_version = read_uint16_t(buffer + offset + 10);

	uint16_t number_of_name_entries = read_uint16_t(buffer + offset + 12);
	uint16_t number_of_id_entries = read_uint16_t(buffer + offset + 14);

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
