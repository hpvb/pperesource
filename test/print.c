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

	ppelib_header_print(&pe->header);

	//	size_t len = ppelib_write_to_buffer(pe, NULL, 0);
	//	if (ppelib_error()) {
	//		printf("PPELib-Error: %s\n", ppelib_error());
	//		goto out;
	//	}
	//
	//	uint8_t *b = malloc(len);
	//	ppelib_write_to_buffer(pe, b, len);
	//	pe2 = ppelib_create_from_buffer(b, len);
	//	free(b);
	//	if (ppelib_error()) {
	//		printf("PPELib-Error: %s\n", ppelib_error());
	//		goto out;
	//	}

out:
	ppelib_destroy(pe);
	return retval; // Non-zero return values are reserved for future use.
}
