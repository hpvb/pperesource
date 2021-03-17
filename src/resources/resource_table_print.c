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
#include <time.h>

#include "pe/codepages.h"
#include "pe/constants.h"
#include "pe/languages.h"

#include "platform.h"
#include "ppe_error.h"

#include "resources/resource.h"
#include "utils.h"

void resource_table_fprint(FILE *stream, resource_table_t *resource_table) {
	ppelib_reset_error();

	for (uint32_t i = 0; i < resource_table->size; ++i) {
		resource_t *resource = &resource_table->resources[i];
		fprintf(stream, "Type: ");
		if (resource->type) {
			fprintf(stream, "%s", resource->type);
		} else {
			const char *type = map_lookup(resource->type_id, ppelib_resource_types_map);
			if (type) {
				fprintf(stream, "%s", type);
			} else {
				fprintf(stream, "Ordinal(%i)", resource->type_id);
			}
		}

		fprintf(stream, " Name: ");
		if (resource->name) {
			fprintf(stream, "%s", resource->name);
		} else {
			fprintf(stream, "Ordinal(%i)", resource->name_id);
		}

		fprintf(stream, " Language: ");
		if (resource->language) {
			fprintf(stream, "%s", resource->language);
		} else {
			const char *language = map_lookup(resource->language_id, ppelib_language_map);
			if (language) {
				fprintf(stream, "%s", language);
			} else {
				fprintf(stream, "Ordinal(%i)", resource->language_id);
			}
		}

		fprintf(stream, " Codepage: ");
		const char *codepage = map_lookup(resource->codepage, ppelib_codepage_map);
		if (codepage) {
			fprintf(stream, "%s", codepage);
		} else {
			fprintf(stream, "Ordinal(%i)", resource->codepage);
		}

		fprintf(stream, "\n");
	}
}

void resource_table_print(resource_table_t *resource_table) {
	resource_table_fprint(stdout, resource_table);
}
