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
#include <wchar.h>

#include "main.h"
#include "platform.h"
#include "ppe_error.h"
#include "resources/resource.h"

thread_local static size_t rscs_base;

int32_t typecmp(const void *a, const void *b) {
	resource_t *ra = *(resource_t **)a;
	resource_t *rb = *(resource_t **)b;

	if (ra->type && rb->type) {
		return wcscmp(ra->type, rb->type);
	}

	if (!ra->type && !rb->type) {
		return (ra->type_id > rb->type_id);
	}

	if (ra->type) {
		return -1;
	} else {
		return 1;
	}
}

int32_t namecmp(const void *a, const void *b) {
	resource_t *ra = *(resource_t **)a;
	resource_t *rb = *(resource_t **)b;

	if (ra->name && rb->name) {
		return wcscmp(ra->name, rb->name);
	}

	if (!ra->name && !rb->name) {
		return (ra->name_id > rb->name_id);
	}

	if (ra->name) {
		return -1;
	} else {
		return 1;
	}
}

int32_t langcmp(const void *a, const void *b) {
	resource_t *ra = *(resource_t **)a;
	resource_t *rb = *(resource_t **)b;

	if (ra->language && rb->language) {
		return wcscmp(ra->language, rb->language);
	}

	if (!ra->language && !rb->language) {
		return (ra->language_id > rb->language_id);
	}

	if (ra->language) {
		return -1;
	} else {
		return 1;
	}
}

size_t resource_table_serialize(const section_t *section, const size_t offset, resource_table_t *resource_table) {
	ppelib_reset_error();

	// Don't want this push to cause CI to fail
	if (offset) {
	}
	if (section) {
	}

	//size_t size = section->contents_size;
	//uint8_t *buffer = section->contents;
	rscs_base = section->virtual_address;

	qsort(resource_table->resources, resource_table->size, sizeof(resource_t *), &langcmp);
	qsort(resource_table->resources, resource_table->size, sizeof(resource_t *), &namecmp);
	qsort(resource_table->resources, resource_table->size, sizeof(resource_t *), &typecmp);

	return 0;
}
