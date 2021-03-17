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

#ifndef SRC_RESOURCES_VERSIONINFO_H_
#define SRC_RESOURCES_VERSIONINFO_H_

#include <inttypes.h>
#include <stddef.h>

typedef struct resource resource_t;

typedef struct dictionary_entry {
	char *key;
	char *value;
} dictionary_entry_t;

typedef struct language {
	uint16_t language;
	uint16_t codepage;
} language_t;

typedef struct dictionary {
	language_t language;

	size_t size;
	dictionary_entry_t **entries;
} dictionary_t;

typedef struct version {
	uint16_t major_version;
	uint16_t minor_version;
	uint16_t patch_version;
	uint16_t build_version;
} version_t;

typedef struct version_info {
	version_t file_version;
	version_t product_version;

	uint32_t version;
	uint32_t flags_mask;
	uint32_t flags;
	uint32_t os;
	uint32_t type;
	uint32_t subtype;
	uint64_t date;

	size_t numb_fileinfo;
	dictionary_t **fileinfo;

	size_t numb_languages;
	language_t *languages;

	resource_t *resource;
} version_info_t;

void versioninfo_set_value(version_info_t *versioninfo, const uint16_t language, const uint16_t codepage, const char *key, const char *value);
void versioninfo_set_file_version(version_info_t *versioninfo, const uint16_t major, const uint16_t minor, const uint16_t patch, const uint16_t build);
void versioninfo_set_product_version(version_info_t *versioninfo, const uint16_t major, const uint16_t minor, const uint16_t patch, const uint16_t build);

const char *versioninfo_get_value(version_info_t *versioninfo, const uint16_t language, const uint16_t codepage, const char *key);

void versioninfo_deserialize(resource_t *resource, version_info_t *versioninfo);
void versioninfo_serialize(version_info_t *versioninfo);

void versioninfo_free(version_info_t *versioninfo);
void versioninfo_print(const version_info_t *versioninfo);

#endif /* SRC_RESOURCES_VERSIONINFO_H_ */
