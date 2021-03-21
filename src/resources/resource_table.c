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

void resource_free(resource_t *resource) {
	free(resource->type);
	free(resource->name);
	free(resource->language);
	free(resource->data);
	free(resource);
}

void resource_table_free(resource_table_t *resource_table) {
	for (size_t i = 0; i < resource_table->numb_versioninfo; ++i) {
		versioninfo_free(&resource_table->versioninfo[i]);
	}
	free(resource_table->versioninfo);

	for (size_t i = 0; i < resource_table->numb_icon_group; ++i) {
		icon_group_free(&resource_table->icongroups[i]);
	}
	free(resource_table->icongroups);

	for (size_t i = 0; i < resource_table->size; ++i) {
		resource_free(resource_table->resources[i]);
	}
	free(resource_table->resources);
}

size_t resource_count_by_type_id(const resource_table_t *resource_table, uint32_t type) {
	ppelib_reset_error();

	size_t size = 0;

	for (size_t i = 0; i < resource_table->size; ++i) {
		if (resource_table->resources[i]->type_id == type) {
			++size;
		}
	}

	return size;
}

resource_t *resource_get_by_type_id(const resource_table_t *resource_table, uint32_t type, size_t idx) {
	ppelib_reset_error();

	size_t size = 0;

	for (size_t i = 0; i < resource_table->size; ++i) {
		if (resource_table->resources[i]->type_id == type) {
			if (size == idx) {
				return resource_table->resources[i];
			}
			++size;
		}
	}

	return NULL;
}

void resource_delete(resource_table_t *resource_table, resource_t *resource) {
	for (size_t i = 0; i < resource_table->size; ++i) {
		if (resource_table->resources[i] == resource) {
			resource_free(resource);

			--resource_table->size;
			memmove(&resource_table->resources[i], &resource_table->resources[i + 1], (resource_table->size - i) * sizeof(void *));
			resource_table->resources = realloc(resource_table->resources, resource_table->size * sizeof(void *));
			return;
		}
	}
}

size_t resource_get_numb_icon_group(const resource_table_t *resource_table) {
	ppelib_reset_error();

	return resource_table->numb_icon_group;
}

icon_group_t *resource_get_icon_group(const resource_table_t *resource_table, size_t idx) {
	ppelib_reset_error();

	if (idx >= resource_table->numb_icon_group) {
		ppelib_set_error("Index out of range");
		return NULL;
	}

	return &resource_table->icongroups[idx];
}

size_t resource_get_numb_versioninfo(const resource_table_t *resource_table) {
	ppelib_reset_error();

	return resource_table->numb_versioninfo;
}

version_info_t *resource_get_versioninfo(const resource_table_t *resource_table, size_t idx) {
	ppelib_reset_error();

	if (idx >= resource_table->numb_versioninfo) {
		ppelib_set_error("Index out of range");
		return NULL;
	}

	return &resource_table->versioninfo[idx];
}
