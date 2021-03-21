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

#include <ctype.h>
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

void versioninfo_free(version_info_t *versioninfo) {
	for (size_t i = 0; i < versioninfo->numb_fileinfo; ++i) {
		for (size_t k = 0; k < versioninfo->fileinfo[i]->size; ++k) {
			free(versioninfo->fileinfo[i]->entries[k]->key);
			free(versioninfo->fileinfo[i]->entries[k]->value);
			free(versioninfo->fileinfo[i]->entries[k]);
		}
		free(versioninfo->fileinfo[i]->entries);
		free(versioninfo->fileinfo[i]);
	}

	free(versioninfo->languages);
	free(versioninfo->fileinfo);
}

dictionary_t *create_fileinfo(version_info_t *versioninfo, const uint16_t language, const uint16_t codepage) {
	size_t idx = versioninfo->numb_fileinfo;
	++versioninfo->numb_fileinfo;

	versioninfo->fileinfo = realloc(versioninfo->fileinfo, sizeof(void *) * versioninfo->numb_fileinfo);
	versioninfo->fileinfo[idx] = calloc(sizeof(dictionary_t), 1);
	versioninfo->fileinfo[idx]->language.language = language;
	versioninfo->fileinfo[idx]->language.codepage = codepage;

	return versioninfo->fileinfo[idx];
}

dictionary_t *find_or_create_fileinfo(version_info_t *versioninfo, const uint16_t language, const uint16_t codepage) {
	for (size_t i = 0; i < versioninfo->numb_fileinfo; ++i) {
		if (versioninfo->fileinfo[i]->language.language == language &&
				versioninfo->fileinfo[i]->language.codepage == codepage) {
			return versioninfo->fileinfo[i];
		}
	}

	return create_fileinfo(versioninfo, language, codepage);
}

char *strip_string(const char *string) {
	char *strip_string = strdup(string);
	size_t strip_string_len = strlen(string);
	for (size_t i = strip_string_len; i > 0; --i) {
		if (!isblank(strip_string[i])) {
			if (i < strip_string_len) {
				strip_string[i + 1] = 0;
				break;
			}
		}
	}

	return strip_string;
}

void versioninfo_set_value(version_info_t *versioninfo, const uint16_t language, const uint16_t codepage, const char *key, const char *value) {
	ppelib_reset_error();

	//	printf("versioninfo_set_value: %s = %s\n", key, value);
	dictionary_t *fileinfo = find_or_create_fileinfo(versioninfo, language, codepage);

	char *strip_key = strip_string(key);
	char *strip_value = strip_string(value);

	for (size_t i = 0; i < fileinfo->size; ++i) {
		if (strcmp(fileinfo->entries[i]->key, strip_key) == 0) {
			free(fileinfo->entries[i]->value);
			fileinfo->entries[i]->value = strip_value;
			free(strip_key);
			return;
		}
	}

	size_t idx = fileinfo->size;
	++fileinfo->size;

	fileinfo->entries = realloc(fileinfo->entries, sizeof(void *) * fileinfo->size);
	fileinfo->entries[idx] = malloc(sizeof(dictionary_entry_t));
	fileinfo->entries[idx]->key = strip_key;
	fileinfo->entries[idx]->value = strip_value;
}

void versioninfo_print(const version_info_t *versioninfo) {
	printf("File Version: %i.%i.%i.%i\n",
			versioninfo->file_version.major_version,
			versioninfo->file_version.minor_version,
			versioninfo->file_version.patch_version,
			versioninfo->file_version.build_version);

	printf("Product Version: %i.%i.%i.%i\n",
			versioninfo->file_version.major_version,
			versioninfo->file_version.minor_version,
			versioninfo->file_version.patch_version,
			versioninfo->file_version.build_version);

	printf("Struct version: 0x%08X\n", versioninfo->version);
	printf("Flags mask: 0x%08X\n", versioninfo->flags_mask);
	printf("Flags: 0x%08X\n", versioninfo->flags);
	printf("OS: 0x%08X\n", versioninfo->os);
	printf("Type: 0x%08X\n", versioninfo->type);
	printf("SubType: 0x%08X\n", versioninfo->subtype);

	char time_date_stamp[50];
	struct tm tm_time;
	time_t time = (time_t)versioninfo->date;
	gmtime_r(&time, &tm_time);
	strftime(time_date_stamp, sizeof(time_date_stamp), "%a, %d %b %Y %H:%M:%S %z", &tm_time);
	time_date_stamp[sizeof(time_date_stamp) - 1] = 0;
	printf("Date: (%016" PRIx64 ") %s\n", versioninfo->date, time_date_stamp);

	for (size_t i = 0; i < versioninfo->numb_fileinfo; ++i) {
		dictionary_t *fileinfo = versioninfo->fileinfo[i];
		printf("Language: 0x%04X Codepage: 0x%04X\n", fileinfo->language.language, fileinfo->language.codepage);

		for (size_t l = 0; l < fileinfo->size; ++l) {
			printf("  '%s': '%s'\n", fileinfo->entries[l]->key, fileinfo->entries[l]->value);
		}
	}

	if (versioninfo->numb_languages) {
		printf("Translations: ");
		for (size_t i = 0; i < versioninfo->numb_languages; ++i) {
			if (i > 0)
				printf(", ");
			printf("0x%04X 0x%04X",
					versioninfo->languages[i].language, versioninfo->languages[i].codepage);
		}
		printf("\n");
	}
}
