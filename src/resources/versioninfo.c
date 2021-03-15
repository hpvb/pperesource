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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wchar.h>

#include "pe/codepages.h"
#include "pe/constants.h"
#include "pe/languages.h"

#include "platform.h"
#include "ppe_error.h"

#include "resources/resource.h"
#include "utils.h"

wchar_t *get_wchar_string(const uint8_t *buffer, size_t size, size_t offset) {
	size_t string_size = 0;

	if (offset > size) {
		ppelib_set_error("Can't read past end of buffer");
		return NULL;
	}

	for (size_t i = offset; i < size - 2; i += 2) {
		uint16_t val = read_uint16_t(buffer + i);
		if (!val) {
			string_size = (i - offset) / 2;
			break;
		}
	}

	if (!string_size) {
		ppelib_set_error("No string found");
		return NULL;
	}

	wchar_t *string = calloc((string_size + 1u) * sizeof(wchar_t), 1);
	if (!string) {
		ppelib_set_error("Failed to allocate string");
		return NULL;
	}

	if (sizeof(wchar_t) == 2) {
		memcpy(string, buffer + offset, string_size * 2u);
	} else {
		for (uint16_t i = 0; i < string_size; ++i) {
			memcpy(string + i, buffer + offset + (i * 2), 2);
		}
	}

	return string;
}

void versioninfo_deserialize(const uint8_t *buffer, size_t size, size_t offset) {
	ppelib_reset_error();

	if (size < 6) {
		ppelib_set_error("Too little room for vsersioninfo");
		return;
	}

	wchar_t *key = NULL;
	wchar_t *sfi_key = NULL;
	wchar_t *st_key = NULL;
	wchar_t *s_key = NULL;
	wchar_t *s_value = NULL;

	uint16_t length = read_uint16_t(buffer + offset + 0);
	uint16_t value_length = read_uint16_t(buffer + offset + 2);
	uint16_t type = read_uint16_t(buffer + offset + 4);
	key = get_wchar_string(buffer, size, offset + 6);
	if (!key) {
		return;
	}

	if (wcscmp(key, L"VS_VERSION_INFO") != 0) {
		ppelib_set_error("VS_VERSION_INFO key not found");
		goto out;
	}

	size_t vs_fixedfileinfo_offset = TO_NEAREST(offset + 38, 4);

	if (size < vs_fixedfileinfo_offset + 52) {
		ppelib_set_error("Too little room for vsfixedfileinfo");
		goto out;
	}

	uint32_t signature = read_uint32_t(buffer + vs_fixedfileinfo_offset);
	if (signature != 0xFEEF04BD) {
		ppelib_set_error("VS_FIXEDFILEINFO signature not found");
		goto out;
	}

	uint32_t struct_version = read_uint32_t(buffer + vs_fixedfileinfo_offset + 4);
	uint32_t file_version_hi = read_uint32_t(buffer + vs_fixedfileinfo_offset + 8);
	uint32_t file_version_lo = read_uint32_t(buffer + vs_fixedfileinfo_offset + 12);
	uint32_t product_version_hi = read_uint32_t(buffer + vs_fixedfileinfo_offset + 16);
	uint32_t product_version_lo = read_uint32_t(buffer + vs_fixedfileinfo_offset + 20);
	uint32_t file_flags_mask = read_uint32_t(buffer + vs_fixedfileinfo_offset + 24);
	uint32_t file_flags = read_uint32_t(buffer + vs_fixedfileinfo_offset + 28);
	uint32_t file_os = read_uint32_t(buffer + vs_fixedfileinfo_offset + 32);
	uint32_t file_type = read_uint32_t(buffer + vs_fixedfileinfo_offset + 36);
	uint32_t file_subtype = read_uint32_t(buffer + vs_fixedfileinfo_offset + 40);
	uint32_t file_date_hi = read_uint32_t(buffer + vs_fixedfileinfo_offset + 44);
	uint32_t file_date_lo = read_uint32_t(buffer + vs_fixedfileinfo_offset + 48);

	size_t stringfileinfo_offset = TO_NEAREST(vs_fixedfileinfo_offset + 52, 4);

	if (size < stringfileinfo_offset + 8) {
		ppelib_set_error("Too little room for stringfileinfo");
		goto out;
	}

	uint16_t sfi_length = read_uint16_t(buffer + stringfileinfo_offset + 0);
	uint16_t sfi_value_length = read_uint16_t(buffer + stringfileinfo_offset + 2);
	uint16_t sfi_type = read_uint16_t(buffer + stringfileinfo_offset + 4);
	sfi_key = get_wchar_string(buffer, size, stringfileinfo_offset + 6);
	if (!sfi_key) {
		goto out;
	}

	if (wcscmp(sfi_key, L"StringFileInfo") != 0) {
		printf("KEY: %ls\n", sfi_key);
		ppelib_set_error("StringFileInfo key not found");
		goto out;
	}

	size_t stringtable_offset = TO_NEAREST(stringfileinfo_offset + 36, 4);
	if (size < stringtable_offset + 8) {
		ppelib_set_error("Too litte room for stringtable");
		goto out;
	}

	uint16_t st_length = read_uint16_t(buffer + stringtable_offset + 0);
	uint16_t st_value_length = read_uint16_t(buffer + stringtable_offset + 2);
	uint16_t st_type = read_uint16_t(buffer + stringtable_offset + 4);
	st_key = get_wchar_string(buffer, size, stringtable_offset + 6);
	if (!st_key) {
		goto out;
	}

	size_t string_offset = TO_NEAREST(stringtable_offset + (wcslen(st_key) * 2) + 2 + 6, 4);
	if (size < string_offset + 8) {
		ppelib_set_error("Too litte room for string");
		goto out;
	}

	uint16_t s_length = read_uint16_t(buffer + string_offset + 0);
	uint16_t s_value_length = read_uint16_t(buffer + string_offset + 2);
	uint16_t s_type = read_uint16_t(buffer + string_offset + 4);
	s_key = get_wchar_string(buffer, size, string_offset + 6);
	if (!s_key) {
		goto out;
	}

	size_t value_offset = TO_NEAREST(string_offset + (wcslen(s_key) * 2) + 2 + 6, 4);
	s_value = get_wchar_string(buffer, size, value_offset + 0);
	if (!s_value) {
		goto out;
	}

	printf("length: %i\n", length);
	printf("value_length: %i\n", value_length);
	printf("type: %i\n", type);
	printf("Key: %ls\n", key);

	printf("------\n");
	printf("Signature: 0x%08X\n", signature);
	printf("struct_version: %i\n", struct_version);
	printf("file_version_hi: %i\n", file_version_hi);
	printf("file_version_lo: %i\n", file_version_lo);
	printf("product_version_hi: %i\n", product_version_hi);
	printf("product_version_lo: %i\n", product_version_lo);
	printf("file_flags_mask: %i\n", file_flags_mask);
	printf("file_flags: %i\n", file_flags);
	printf("file_os: %i\n", file_os);
	printf("file_type: %i\n", file_type);
	printf("file_subtype: %i\n", file_subtype);
	printf("file_date_hi: %i\n", file_date_hi);
	printf("file_date_lo: %i\n", file_date_lo);

	printf("------\n");
	printf("sfi_length: %i\n", sfi_length);
	printf("sfi_value_length: %i\n", sfi_value_length);
	printf("sfi_type: %i\n", sfi_type);
	printf("sfi_key: %ls\n", sfi_key);

	printf("------\n");
	printf("st_length: %i\n", st_length);
	printf("st_value_length: %i\n", st_value_length);
	printf("st_type: %i\n", st_type);
	printf("st_key: %ls\n", st_key);

	printf("------\n");
	printf("s_length: %i\n", s_length);
	printf("s_value_length: %i\n", s_value_length);
	printf("s_type: %i\n", s_type);
	printf("s_key: %ls\n", s_key);
	printf("s_value: %ls\n", s_value);

	printf("------\n");
	printf("File Version: %d.%d.%d.%d\n",
			(file_version_hi >> 16) & 0xffff,
			(file_version_hi >> 0) & 0xffff,
			(file_version_lo >> 16) & 0xffff,
			(file_version_lo >> 0) & 0xffff);

	printf("Product Version: %d.%d.%d.%d\n",
			(product_version_hi >> 16) & 0xffff,
			(product_version_hi >> 0) & 0xffff,
			(product_version_lo >> 16) & 0xffff,
			(product_version_lo >> 0) & 0xffff);
out:

	free(key);
	free(sfi_key);
	free(st_key);
	free(s_key);
	free(s_value);
}
