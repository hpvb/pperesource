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

size_t get_resource_by_type_id(const resource_table_t *resource_table, uint32_t type, resource_t **resource) {
	ppelib_reset_error();
	*resource = NULL;

	size_t size = 0;

	for (size_t i = 0; i < resource_table->size; ++i) {
		if (resource_table->resources[i]->type_id == type) {
			if (!*resource) {
				*resource = resource_table->resources[i];
			}

			++size;
		}
	}

	return size;
}
