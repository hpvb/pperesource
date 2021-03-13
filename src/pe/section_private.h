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

#ifndef PPELIB_SECTION_PRIVATE_H_
#define PPELIB_SECTION_PRIVATE_H_

#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>

#include "pe/constants.h"

typedef struct ppelib_file ppelib_file_t;

#define SECTION_SIZE 40

#include "platform.h"
#include "utils.h"

typedef struct section {
	char name[9];
	uint32_t virtual_size;
	uint32_t virtual_address;
	uint32_t size_of_raw_data;
	uint32_t pointer_to_raw_data;
	uint32_t pointer_to_relocations;
	uint32_t pointer_to_linenumbers;
	uint16_t number_of_relocations;
	uint16_t number_of_linenumbers;
	uint32_t characteristics;

	uint8_t *contents;
	size_t contents_size;
} section_t;

EXPORT_SYM size_t ppelib_section_serialize(const section_t *section, uint8_t *buffer, const size_t offset);
EXPORT_SYM size_t ppelib_section_deserialize(const uint8_t *buffer, const size_t size, const size_t offset, section_t *section);
EXPORT_SYM void ppelib_section_fprint(FILE *stream, const section_t *section);
EXPORT_SYM void ppelib_section_print(const section_t *section);

#endif /* PPELIB_SECTION_PRIVATE_H_  */
