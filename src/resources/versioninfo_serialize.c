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

thread_local static uint8_t *buffer[1];
thread_local static size_t buffer_size;

static void resize_buffer(size_t size) {
	if (buffer_size < size) {
		size_t old_size = buffer_size;
		buffer_size = size;
		*buffer = realloc(*buffer, buffer_size);
		memset(*buffer + old_size, 0, size - old_size);
	}
}

static uint16_t string_serialize(size_t offset, dictionary_entry_t *entry) {
	uint16_t length = 0;
	uint16_t type = 1;

	char *key = NULL;
	char *value = NULL;
	size_t key_len = convert_utf8_string(entry->key, &key);
	size_t value_len = convert_utf8_string(entry->value, &value);

	length += (uint16_t)(key_len);
	length += (uint16_t)(value_len);

	size_t key_offset = 6;
	size_t value_offset = TO_NEAREST(key_offset + key_len + 2, 4);

	resize_buffer(offset + value_offset + value_len);
	uint8_t *buf = *buffer + offset;

	memcpy(buf + key_offset, key, key_len);
	memcpy(buf + value_offset, value, value_len);

	length = (uint16_t)(value_offset + value_len + 2);
	write_uint16_t(buf, length);
	write_uint16_t(buf + 2, ((uint16_t)value_len / 2) + 1);
	write_uint16_t(buf + 4, type);

	free(key);
	free(value);

	return length;
}

static uint16_t stringtable_serialize(size_t offset, dictionary_t *fileinfo) {
	uint16_t length = 24;
	uint16_t value_length = 0;
	uint16_t type = 1;

	resize_buffer(offset + length);
	uint8_t *buf = *buffer + offset;

	char langcode[9];
	snprintf(langcode, 9, "%04x%04x", fileinfo->language.language, fileinfo->language.codepage);

	for (size_t i = 0; i < 8; ++i) {
		write_uint16_t(buf + 6 + (i * 2), (uint16_t)langcode[i]);
	}

	for (size_t i = 0; i < fileinfo->size; ++i) {
		size_t child_offset = TO_NEAREST(offset + length, 4);
		size_t child_length = string_serialize(child_offset, fileinfo->entries[i]);
		child_length = TO_NEAREST(child_length, 4);
		length += (uint16_t)child_length;
	}

	buf = *buffer + offset;
	write_uint16_t(buf, length);
	write_uint16_t(buf + 2, value_length);
	write_uint16_t(buf + 4, type);

	return length;
}

static uint16_t varfileinfo_serialize(size_t offset, version_info_t *versioninfo) {
	const uint8_t key[] = "VarFileInfo";
	const uint8_t translation[] = "Translation";

	uint16_t length = 6 + ((sizeof(key) + 1) * 2);
	uint16_t value_length = 0;
	uint16_t type = 1;

	size_t translation_offset = TO_NEAREST(length, 4);
	size_t string_offset = translation_offset + 6;
	size_t codepages_offset = TO_NEAREST(translation_offset + ((sizeof(translation) + 1) * 2), 4);
	codepages_offset += 4;
	uint16_t codepages_size = (uint16_t)(versioninfo->numb_languages * sizeof(uint32_t));

	length = (uint16_t)(codepages_offset + codepages_size);
	uint16_t translation_size = length - (uint16_t)translation_offset;

	resize_buffer(offset + length);
	uint8_t *buf = *buffer + offset;

	for (size_t i = 0; i < sizeof(key); ++i) {
		write_uint16_t(buf + 6 + (i * 2), key[i]);
	}

	write_uint16_t(buf + translation_offset, translation_size);
	write_uint16_t(buf + translation_offset + 2, codepages_size);
	write_uint16_t(buf + translation_offset + 4, 0);

	for (size_t i = 0; i < sizeof(translation); ++i) {
		write_uint16_t(buf + string_offset + (i * 2), translation[i]);
	}

	for (size_t i = 0; i < versioninfo->numb_languages; ++i) {
		write_uint16_t(buf + codepages_offset + (i * 4), versioninfo->languages[i].language);
		write_uint16_t(buf + codepages_offset + (i * 4) + 2, versioninfo->languages[i].codepage);
	}

	write_uint16_t(buf, length);
	write_uint16_t(buf + 2, value_length);
	write_uint16_t(buf + 4, type);

	return length;
}

static uint16_t stringfileinfo_serialize(size_t offset, dictionary_t *fileinfo) {
	uint16_t length = 36;
	uint16_t value_length = 0;
	uint16_t type = 1;

	size_t next_offset = TO_NEAREST(length, 4);

	resize_buffer(offset + next_offset);
	uint8_t *buf = *buffer + offset;

	const uint8_t key[] = "StringFileInfo";
	for (size_t i = 0; i < sizeof(key); ++i) {
		write_uint16_t(buf + 6 + (i * 2), key[i]);
	}

	length += stringtable_serialize(offset + next_offset, fileinfo);

	buf = *buffer + offset;
	write_uint16_t(buf, length);
	write_uint16_t(buf + 2, value_length);
	write_uint16_t(buf + 4, type);

	return length;
}

static uint16_t fixedfileinfo_serialize(size_t offset, version_info_t *versioninfo) {
	resize_buffer(offset + 52);

	write_uint32_t(*buffer + offset, 0xFEEF04BD);

	write_uint32_t(*buffer + offset + 4, versioninfo->version);

	write_uint16_t(*buffer + offset + 8, versioninfo->file_version.minor_version);
	write_uint16_t(*buffer + offset + 10, versioninfo->file_version.major_version);
	write_uint16_t(*buffer + offset + 12, versioninfo->file_version.build_version);
	write_uint16_t(*buffer + offset + 14, versioninfo->file_version.patch_version);

	write_uint16_t(*buffer + offset + 16, versioninfo->product_version.minor_version);
	write_uint16_t(*buffer + offset + 18, versioninfo->product_version.major_version);
	write_uint16_t(*buffer + offset + 20, versioninfo->product_version.build_version);
	write_uint16_t(*buffer + offset + 22, versioninfo->product_version.patch_version);

	write_uint32_t(*buffer + offset + 24, versioninfo->flags_mask);
	write_uint32_t(*buffer + offset + 28, versioninfo->flags);
	write_uint32_t(*buffer + offset + 32, versioninfo->os);
	write_uint32_t(*buffer + offset + 36, versioninfo->type);
	write_uint32_t(*buffer + offset + 40, versioninfo->subtype);
	write_uint64_t(*buffer + offset + 44, versioninfo->date);

	return 52;
}

void versioninfo_serialize(version_info_t *versioninfo) {
	resource_t *resource = versioninfo->resource;
	*buffer = NULL;
	buffer_size = 0;

	resize_buffer(90);

	uint16_t length = 38;
	uint16_t value_length = 52;
	uint16_t type = 0;

	write_uint16_t(*buffer + 2, value_length);
	write_uint16_t(*buffer + 4, type);

	const uint8_t vsi_key[] = "VS_VERSION_INFO";
	for (size_t i = 0; i < sizeof(vsi_key); ++i) {
		write_uint16_t(*buffer + 6 + (i * 2), vsi_key[i]);
	}

	size_t offset = TO_NEAREST(38, 4);
	uint16_t item_length = fixedfileinfo_serialize(offset, versioninfo);
	length += item_length;

	for (size_t i = 0; i < versioninfo->numb_fileinfo; ++i) {
		//for (size_t i = 0; i < 1; ++i) {
		offset = TO_NEAREST(offset + item_length, 4);
		item_length = stringfileinfo_serialize(offset, versioninfo->fileinfo[i]);
		length += item_length;
	}
	offset = TO_NEAREST(offset + item_length, 4);

	length += varfileinfo_serialize(offset, versioninfo);
	//resize_buffer(TO_NEAREST(buffer_size, 4));

	length = (uint16_t)TO_NEAREST(length, 4);
	write_uint16_t(*buffer, length);

	free(resource->data);
	resource->data = *buffer;
	resource->size = buffer_size;
}
