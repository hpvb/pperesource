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
#include <string.h>

#include "pe/constants.h"
#include "ppe_error.h"

#include "utils.h"

#include "section_private.h"

size_t section_deserialize(const uint8_t *buffer, const size_t size, const size_t offset, section_t *section) {
	ppelib_reset_error();

	if (offset + 40 > size) {
		ppelib_set_error("Not enough space for section headers");
		return 0;
	}

	memcpy(section->name, buffer + offset + 0, 8);
	section->name[8] = 0;
	section->virtual_size = read_uint32_t(buffer + offset + 8);
	section->virtual_address = read_uint32_t(buffer + offset + 12);
	section->size_of_raw_data = read_uint32_t(buffer + offset + 16);
	section->pointer_to_raw_data = read_uint32_t(buffer + offset + 20);
	section->pointer_to_relocations = read_uint32_t(buffer + offset + 24);
	section->pointer_to_linenumbers = read_uint32_t(buffer + offset + 28);
	section->number_of_relocations = read_uint16_t(buffer + offset + 32);
	section->number_of_linenumbers = read_uint16_t(buffer + offset + 34);
	section->characteristics = read_uint32_t(buffer + offset + 36);

	return 40;
}
