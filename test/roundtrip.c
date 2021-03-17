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

#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "ppe_error.h"

int main(int argc, char *argv[]) {
	int retval = 0;

	if (argc != 3) {
		printf("Usage: %s <infile> <outfile>\n", argv[0]);
		return 1;
	}

	ppelib_file_t *pe = ppelib_create_from_file(argv[1]);
	if (ppelib_error()) {
		printf("PPELib-Error: %s\n", ppelib_error());
		retval = 1;
		goto out;
	}

	//	if (!pe->resource_table.numb_versioninfo) {
	//		++pe->resource_table.size;
	//		pe->resource_table.resources = realloc(pe->resource_table.resources, sizeof(resource_t) * pe->resource_table.size);
	//		pe->resource_table.versioninfo = calloc(sizeof(version_info_t), 1);
	//		resource_t* resource = &pe->resource_table.resources[pe->resource_table.size - 1];
	//		pe->resource_table.versioninfo->resource = resource;
	//		memset(resource, 0, sizeof(resource_t));
	//		resource->type_id = RT_VERSION;
	//		++pe->resource_table.numb_versioninfo;
	//		versioninfo_set_value(&pe->resource_table.versioninfo[0], 0, 0, "This is a key", "This is a value");
	//	}

	update_resource_table(pe);
	ppelib_write_to_file(pe, argv[2]);
	if (ppelib_error()) {
		printf("PPELib-Error: %s\n", ppelib_error());
		goto out;
	}

out:
	ppelib_destroy(pe);
	return retval; // Non-zero return values are reserved for future use.
}
