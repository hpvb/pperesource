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

#ifndef SRC_RESOURCES_STRING_TABLE_H_
#define SRC_RESOURCES_STRING_TABLE_H_

#include <inttypes.h>
#include <stddef.h>

typedef struct string_table_string {
	uint32_t offset;
	uint16_t bytes;

	const char *string;
	char *utf16_string;
} string_table_string_t;

typedef struct string_table {
	size_t size;
	size_t bytes;

	uint32_t base_offset;

	string_table_string_t *strings;
} string_table_t;

void string_table_free(string_table_t *table);
void string_table_put(string_table_t *table, const char *string);
string_table_string_t *string_table_find(string_table_t *table, const char *string);
void string_table_serialize(string_table_t *table, uint8_t *buffer);

#endif /* SRC_RESOURCES_STRING_TABLE_H_ */
