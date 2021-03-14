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

#ifndef PPELIB_DATA_DIRECTORY_PRIVATE_H_
#define PPELIB_DATA_DIRECTORY_PRIVATE_H_

#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>

#include "pe/constants.h"

#include "section_private.h"

#include "platform.h"
#include "utils.h"

typedef struct ppelib_file ppelib_file_t;

typedef struct data_directory {
	section_t *section;

	size_t offset;
	size_t size;
	uint32_t id;
} data_directory_t;

void ppelib_data_directory_print(const data_directory_t *data_directory);
void ppelib_data_directory_fprint(FILE *stream, const data_directory_t *data_directory);

#endif /* PPELIB_DATA_DIRECTORY_PRIVATE_H_ */
