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

size_t section_serialize(const section_t *section, uint8_t *buffer, const size_t offset) {
	ppelib_reset_error();

	if (!section) {
		ppelib_set_error("NULL pointer");
		return 0;
	}

	if (!buffer) {
		goto out;
	}

	memcpy(buffer + offset + 0, section->name, 8);
	write_uint32_t(buffer + offset + 8, section->virtual_size);
	write_uint32_t(buffer + offset + 12, section->virtual_address);
	write_uint32_t(buffer + offset + 16, section->size_of_raw_data);
	write_uint32_t(buffer + offset + 20, section->pointer_to_raw_data);
	write_uint32_t(buffer + offset + 24, section->pointer_to_relocations);
	write_uint32_t(buffer + offset + 28, section->pointer_to_linenumbers);
	write_uint16_t(buffer + offset + 32, section->number_of_relocations);
	write_uint16_t(buffer + offset + 34, section->number_of_linenumbers);
	write_uint32_t(buffer + offset + 36, section->characteristics);

out:
	return 40;
}
