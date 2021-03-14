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

#include "data_directory_private.h"
#include "platform.h"
#include "ppe_error.h"

void ppelib_data_directory_fprint(FILE *stream, const data_directory_t *data_directory) {
	ppelib_reset_error();

	fprintf(stream, "Type: %s, ", map_lookup(data_directory->id, ppelib_data_directories_map));
	if (data_directory->section) {
		fprintf(stream, "Section: %s ", data_directory->section->name);
	} else if (data_directory->size) {
		fprintf(stream, "Section: After section data, ");
	} else {
		fprintf(stream, "Section: Empty, ");
	}

	fprintf(stream, "Offset: %zi, ", data_directory->offset);
	fprintf(stream, "Size: %zi\n", data_directory->size);
}

void ppelib_data_directory_print(const data_directory_t *data_directory) {
	ppelib_data_directory_fprint(stdout, data_directory);
}
