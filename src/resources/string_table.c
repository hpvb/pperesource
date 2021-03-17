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
#include "resources/string_table.h"

void string_table_serialize(string_table_t *table, uint8_t *buffer) {
	for (size_t i = 0; i < table->size; ++i) {
		size_t offset = table->base_offset + table->strings[i].offset;

		const char *string = table->strings[i].utf16_string;
		size_t size = table->strings[i].bytes;

		if (size / 2 > UINT16_MAX) {
			ppelib_set_error("String too long");
			return;
		}

		//		printf("string_table_serialize: writing string %zi\n", size);
		write_uint16_t(buffer + offset, (uint16_t)(size / 2));
		memcpy(buffer + offset + 2, string, size);
	}
}

string_table_string_t *string_table_find(string_table_t *table, const char *string) {
	//	printf("string_table_find: '%s'\n", string);
	for (size_t i = 0; i < table->size; ++i) {
		if (strcmp(string, table->strings[i].string) == 0) {
			return &table->strings[i];
		}
	}

	return NULL;
}

void string_table_put(string_table_t *table, const char *string) {
	char *string_utf16;
	size_t s_size = convert_utf8_string(string, &string_utf16);

	//	printf("Utf16-pointer: %p\n", string_utf16);
	//	printf("String_table_put: %zi, '%s'\n", s_size, string);
	if (s_size > UINT16_MAX) {
		ppelib_set_error("String size too long");
		free(string_utf16);
		return;
	}

	if (string_table_find(table, string)) {
		free(string_utf16);
		return;
	}

	table->size++;
	table->bytes += s_size + 2;

	table->strings = realloc(table->strings, sizeof(string_table_string_t) * table->size);
	table->strings[table->size - 1].string = string;
	table->strings[table->size - 1].utf16_string = string_utf16;
	table->strings[table->size - 1].bytes = (uint16_t)s_size;

	if (table->size > 1) {
		uint32_t prev_offset = table->strings[table->size - 2].offset;
		uint32_t prev_length = table->strings[table->size - 2].bytes;
		table->strings[table->size - 1].offset = prev_offset + prev_length + 2;
	} else {
		table->strings[table->size - 1].offset = 0;
	}
}

void string_table_free(string_table_t *table) {
	for (size_t i = 0; i < table->size; ++i) {
		free(table->strings[i].utf16_string);
	}

	free(table->strings);
}
