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

#include "pe/codepages.h"
#include "pe/constants.h"
#include "pe/languages.h"

#include "platform.h"
#include "ppe_error.h"

#include "resources/resource.h"
#include "resources/versioninfo.h"
#include "utils.h"

static size_t get_string(const uint8_t *buffer, size_t size, size_t offset, size_t max_size, char **outstring) {
	size_t string_size = 0;

	if (offset > size) {
		ppelib_set_error("Can't read past end of buffer");
		return 0;
	}

	max_size = MIN(max_size, size - offset);

	if (max_size > 0 && max_size < 2) {
		ppelib_set_error("Can't read past end of buffer");
		return 0;
	}

	for (size_t i = offset; i < offset + max_size - 1; i += 2) {
		uint16_t val = read_uint16_t(buffer + i);
		if (!val) {
			string_size = (i - offset);
			break;
		}
	}

	if (!string_size) {
		if (offset + max_size < size) {
			string_size = max_size;
		} else {
			string_size = size - offset;
		}
	}

	*outstring = get_utf16_string(buffer, size, offset, string_size);
	return string_size;
}

size_t find_next_value(const uint8_t *buffer, size_t size, size_t offset) {
	if (offset > size) {
		ppelib_set_error("Can't read past end of buffer");
		return 0;
	}

	for (size_t i = offset; i < size - 2; i += 2) {
		uint16_t val = read_uint16_t(buffer + i);
		if (val) {
			return i;
			break;
		}
	}

	return size;
}

static size_t varfileinfo_deserialize(const uint8_t *buffer, size_t size, size_t offset, version_info_t *versioninfo) {
	char *key = NULL;

	size_t consumed = 0;

	if (size < offset + 8) {
		return 2;
		//printf("offset: %zi, size: %zi\n", offset, size);
		goto out;
	}

	uint16_t length = read_uint16_t(buffer + offset + 0);
	uint16_t value_length = read_uint16_t(buffer + offset + 2);

	if (!length) {
		return 2;
	}

	//uint16_t type = read_uint16_t(buffer + offset + 4);
	get_string(buffer, size, offset + 6, 24, &key);
	if (!key) {
		ppelib_set_error("Failed to parse varfileinfo (no key)");
		goto out;
	}

	if (strcmp(key, "Translation") != 0) {
		ppelib_set_error("No Translation found in VarFileInfo");
		goto out;
	}

	consumed = 30;

	size_t value_offset = TO_NEAREST(30, 4);
	size_t numb_values = value_length / 4;

	if (offset + value_offset + value_length > size) {
		ppelib_set_error("Not enough space for varfileinfo data");
		goto out;
	}

	size_t old_numb_languages = versioninfo->numb_languages;
	versioninfo->numb_languages += numb_values;
	versioninfo->languages = realloc(versioninfo->languages, versioninfo->numb_languages * 4);

	for (size_t i = old_numb_languages; i < versioninfo->numb_languages; ++i) {
		versioninfo->languages[i].language = read_uint16_t(buffer + offset + value_offset);
		versioninfo->languages[i].codepage = read_uint16_t(buffer + offset + value_offset + 2);
		value_offset += 4;
		consumed += 4;
	}

out:
	free(key);
	return MAX(length, consumed);
}

static size_t stringtable_deserialize(const uint8_t *buffer, size_t size, size_t offset, version_info_t *versioninfo) {
	uint16_t length = 0;
	size_t consumed = 0;
	char *key = NULL;
	char *s_key = NULL;
	char *s_val = NULL;

	if (size < offset + 8) {
		ppelib_set_error("Too litte room for stringtable");
		goto out;
	}

	length = read_uint16_t(buffer + offset + 0);
	if (!length) {
		return 2;
	}

	if (size < offset + length) {
		ppelib_set_error("Not enough space for stringtable");
		goto out;
	}

	//	uint16_t value_length = read_uint16_t(buffer + offset + 2);
	//	uint16_t type = read_uint16_t(buffer + offset + 4);
	size_t key_offset = offset + 6;
	while (!buffer[key_offset]) {
		++key_offset;
		if (key_offset > size - 1) {
			ppelib_set_error("Failed to find stringtable start");
			goto out;
		}
	}

	get_string(buffer, size, key_offset, 16, &key);
	if (!key) {
		ppelib_set_error("Failed to parse stringtable");
		goto out;
	}

	if (strlen(key) != 8) {
#ifndef FUZZ
		ppelib_set_error("Failed to parse stringtable language");
		goto out;
#endif
	}

	char *end = key + 7;
	uint32_t langpage = (uint32_t)strtol(key, &end, 16);

	uint16_t language = (uint16_t)(langpage >> 16);
	uint16_t codepage = (uint16_t)(langpage & 0xffff);

	consumed = 24;

	size_t string_offset = find_next_value(buffer, size, offset + consumed);
	//	printf("Length: %i, Value_length: %i, type %i\n", length, value_length, type);
	while (consumed < length) {
		//size_t string_offset = find_next_value(buffer, size, offset + consumed);

		//		printf("key: 0x%04lX\n", string_offset);
		//		printf("  Length: %i, Consumed: %zi\n", length, consumed);
		if (size < string_offset + 8) {
			goto out;
		}

		uint16_t s_pad = 0;
		if (string_offset > 2) {
			s_pad = read_uint16_t(buffer + string_offset - 2);
		}
		uint16_t s_length = read_uint16_t(buffer + string_offset + 0);
		uint16_t s_value_length = read_uint16_t(buffer + string_offset + 2);
		uint16_t s_type = read_uint16_t(buffer + string_offset + 4);
		//		printf("  Length: %i, value_length: %i, type: %i\n", s_length, s_value_length, s_type);

		if (s_pad || !s_length || s_length > length || s_value_length > length || s_type > 1 || s_length == s_value_length) {
			string_offset += 2;
			continue;
		}

		size_t key_offset = string_offset + 6;

		while (!buffer[key_offset]) {
			++key_offset;
			if (key_offset > size - 1) {
				ppelib_set_error("Unable to locate key data");
				goto out;
			}
		}

		size_t s_key_size = get_string(buffer, size, key_offset, s_length, &s_key);
		if (!s_key) {
			//	ppelib_set_error("Failed to parse key string");
			//	goto out;
			free(s_key);
			s_key = NULL;
			string_offset += 6;
			continue;
		}

		if (!strlen(s_key)) {
			// garbage?
			free(s_key);
			s_key = NULL;
			string_offset += 6;
			continue;
		}
		//consumed += 6 + s_key_size + 2;
		size_t value_offset = TO_NEAREST(string_offset + 6 + s_key_size + 2, 4);
		//		printf("val: 0x%04lX\n", value_offset);

		if (!s_value_length) {
			consumed += 6 + s_key_size + 2;
			string_offset += 6 + s_key_size + 2;
			free(s_key);
			s_key = NULL;
			continue;
		}

		if (size < value_offset + 4) {
			ppelib_set_error("Too little room for value");
			goto out;
		}

		if (s_value_length > 2) {
			while (!buffer[value_offset]) {
				++value_offset;
				if (value_offset > size - 1) {
					ppelib_set_error("Unable to locate value data");
					goto out;
				}
			}
		}

		size_t s_val_size = get_string(buffer, size, value_offset, (s_value_length - 1u) * 2, &s_val);
		if (!s_val) {
			ppelib_set_error("Failed to parse value string");
			goto out;
		}

		consumed += s_length;

		if (consumed > length) {
			goto out;
		}

		if (strlen(s_val)) {
			versioninfo_set_value(versioninfo, language, codepage, s_key, s_val);
			string_offset = value_offset + s_val_size;
		} else {
			string_offset = value_offset + 2;
		}

		free(s_key);
		free(s_val);

		s_key = NULL;
		s_val = NULL;

		//printf("new key: 0x%04lX\n", string_offset);
		string_offset = TO_NEAREST(string_offset, 4);
		//string_offset = find_next_value(buffer, size, string_offset);
	}
out:
	free(key);
	free(s_key);
	free(s_val);
	//	printf("Length: %i, Consumed: %zi\n", length, consumed);
	return MIN(length, consumed);
}

static size_t stringinfo_deserialize(const uint8_t *buffer, size_t size, size_t offset, version_info_t *versioninfo) {
	uint16_t length = 0;
	size_t consumed = 0;
	char *key = NULL;

	if (size < offset + 8) {
		return 2;
		//printf("offset: %zi, size: %zi\n", offset, size);
		goto out;
	}

	length = read_uint16_t(buffer + offset + 0);
	//uint16_t value_length = read_uint16_t(buffer + offset + 2);

	if (!length) {
		return 2;
	}

	//uint16_t type = read_uint16_t(buffer + offset + 4);
	get_string(buffer, size, offset + 6, 30, &key);
	if (!key) {
		ppelib_set_error("Failed to parse child (no key)");
		goto out;
	}

	if (strcmp(key, "StringFileInfo") == 0) {
		consumed = 36;

		char parsed_one_table = 0;
		while (length - consumed > 22) {
			size_t stringtable_offset = find_next_value(buffer, size, offset + consumed);

			consumed += stringtable_deserialize(buffer, size, stringtable_offset, versioninfo);
			if (ppelib_error_peek()) {
				if (parsed_one_table) {
					// Garbage after the table.
					ppelib_reset_error();
				}
				goto out;
			}

			parsed_one_table = 1;
		}
	} else if (strcmp(key, "VarFileInfo") == 0) {
		consumed = 30;
		size_t var_offset = find_next_value(buffer, size, offset + consumed);
		consumed += varfileinfo_deserialize(buffer, size, var_offset, versioninfo);

	} else {
		consumed += 2;
		goto out;
	}

out:
	free(key);
	return MAX(length, consumed);
}

static void fixedfileinfo_deserialize(const uint8_t *buffer, size_t size, size_t offset, version_info_t *versioninfo) {
	if (size < offset + 52) {
		ppelib_set_error("Too little room for vsfixedfileinfo");
		return;
	}

	uint32_t signature = read_uint32_t(buffer + offset);
	if (signature != 0xFEEF04BD) {
#ifndef FUZZ
		ppelib_set_error("VS_FIXEDFILEINFO signature not found");
		return;
#endif
	}

	versioninfo->version = read_uint32_t(buffer + offset + 4);

	versioninfo->file_version.minor_version = read_uint16_t(buffer + offset + 8);
	versioninfo->file_version.major_version = read_uint16_t(buffer + offset + 10);
	versioninfo->file_version.build_version = read_uint16_t(buffer + offset + 12);
	versioninfo->file_version.patch_version = read_uint16_t(buffer + offset + 14);

	versioninfo->product_version.minor_version = read_uint16_t(buffer + offset + 16);
	versioninfo->product_version.major_version = read_uint16_t(buffer + offset + 18);
	versioninfo->product_version.build_version = read_uint16_t(buffer + offset + 20);
	versioninfo->product_version.patch_version = read_uint16_t(buffer + offset + 22);

	versioninfo->flags_mask = read_uint32_t(buffer + offset + 24);
	versioninfo->flags = read_uint32_t(buffer + offset + 28);
	versioninfo->os = read_uint32_t(buffer + offset + 32);
	versioninfo->type = read_uint32_t(buffer + offset + 36);
	versioninfo->subtype = read_uint32_t(buffer + offset + 40);
	versioninfo->date = read_uint64_t(buffer + offset + 44);
}

void versioninfo_deserialize(resource_t *resource, version_info_t *versioninfo) {
	ppelib_reset_error();

	uint8_t *buffer = resource->data;
	size_t size = resource->size;
	size_t offset = 0;

	versioninfo->resource = resource;

	if (size < 6) {
		ppelib_set_error("Too little room for vsersioninfo");
		return;
	}

	size_t consumed = 0;

	char *key = NULL;

	uint16_t length = read_uint16_t(buffer + offset + 0);
	uint16_t value_length = read_uint16_t(buffer + offset + 2);
	//uint16_t type = read_uint16_t(buffer + offset + 4);
	get_string(buffer, size, offset + 6, 32, &key);
	if (!key) {
		return;
	}

	if (strcmp(key, "VS_VERSION_INFO") != 0) {
#ifndef FUZZ
		ppelib_set_error("VS_VERSION_INFO key not found");
		goto out;
#endif
	}

	consumed += 38;

	if (consumed == length) {
		goto out;
	}

	size_t fixedfileinfo_offset = TO_NEAREST(offset + 38, 4);
	if (value_length == 52) {
		fixedfileinfo_deserialize(buffer, size, fixedfileinfo_offset, versioninfo);
		if (ppelib_error_peek()) {
			goto out;
		}
	}

	consumed += value_length;

	if (consumed == length) {
		goto out;
	}

	size_t child_offset = find_next_value(buffer, size, offset + 38 + value_length);

	while (consumed < length) {
		consumed += stringinfo_deserialize(buffer, size, child_offset, versioninfo);
		child_offset = TO_NEAREST(offset + consumed, 4);
		if (ppelib_error_peek()) {
			goto out;
		}
	}

out:
	free(key);
}
