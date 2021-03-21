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

#include <stdio.h>
#include <stdlib.h>

#include "main.h"
#include "ppe_error.h"

int main(int argc, char *argv[]) {
	int retval = 0;

	if (argc != 2) {
		printf("Usage: %s <infile>\n", argv[0]);
		return 1;
	}

	ppelib_file_t *pe = ppelib_create_from_file(argv[1]);
	if (ppelib_error()) {
		printf("PPELib-Error: %s\n", ppelib_error());
		retval = 1;
		goto out;
	}

	header_print(&pe->header);
	printf("\nDirectories\n");
	for (uint32_t i = 0; i < pe->header.number_of_rva_and_sizes; ++i) {
		data_directory_print(&pe->data_directories[i]);
	}
	printf("\n");

	printf("\nSections\n");
	for (uint16_t i = 0; i < pe->header.number_of_sections; ++i) {
		section_print(pe->sections[i]);
		printf("\n");
	}

	printf("\nResources\n");
	resource_table_print(&pe->resource_table);
	printf("\n");

	//	printf("%s\n", argv[1]);
	if (pe->resource_table.numb_versioninfo) {
		printf("\nVersion info\n");
	}

	for (size_t i = 0; i < pe->resource_table.numb_versioninfo; ++i) {
		versioninfo_print(&pe->resource_table.versioninfo[i]);
	}

	if (pe->resource_table.numb_icon_group) {
		printf("\nIcon groups:\n");
	}

	for (size_t i = 0; i < pe->resource_table.numb_icon_group; ++i) {
		icon_group_print(&pe->resource_table.icongroups[i]);
	}

	resource_t *res = resource_get_by_type_id(&pe->resource_table, RT_VERSION, 0);
	if (res) {
		FILE *fp = fopen("resource.dump", "w");
		fwrite(res->data, res->size, 1, fp);
		fclose(fp);
	}
out:
	ppelib_destroy(pe);
	return retval; // Non-zero return values are reserved for future use.
}
