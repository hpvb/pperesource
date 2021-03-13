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

#include "pe/constants.h"
#include "platform.h"
#include "ppe_error.h"

#include "utils.h"

#include "ppelib_internal.h"
#include "section_private.h"

EXPORT_SYM void ppelib_section_fprint(FILE *stream, const section_t *section) {
	ppelib_reset_error();

	if (!section) {
		ppelib_set_error("NULL pointer");
		return;
	}

	fprintf(stream, "Name: %s\n", section->name);
	fprintf(stream, "VirtualSize: %u\n", section->virtual_size);
	fprintf(stream, "VirtualAddress: 0x%08X\n", section->virtual_address);
	fprintf(stream, "SizeOfRawData: %u\n", section->size_of_raw_data);
	fprintf(stream, "PointerToRawData: 0x%08X\n", section->pointer_to_raw_data);
	fprintf(stream, "PointerToRelocations: 0x%08X\n", section->pointer_to_relocations);
	fprintf(stream, "PointerToLinenumbers: 0x%08X\n", section->pointer_to_linenumbers);
	fprintf(stream, "NumberOfRelocations: %u\n", section->number_of_relocations);
	fprintf(stream, "NumberOfLinenumbers: %u\n", section->number_of_linenumbers);
	fprintf(stream, "Characteristics: (0x%08X) ", section->characteristics);
	const ppelib_map_entry_t *characteristics_map_i = ppelib_section_flags_map;
	while (characteristics_map_i->string) {
		if (CHECK_BIT(section->characteristics, characteristics_map_i->value)) {
			fprintf(stream, "%s ", characteristics_map_i->string);
		}
		++characteristics_map_i;
	}
	fprintf(stream, "\n");
}

EXPORT_SYM void ppelib_section_print(const section_t *section) {
	ppelib_section_fprint(stdout, section);
}
