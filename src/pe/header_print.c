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

#include "header_private.h"
#include "ppelib_internal.h"

void header_fprint(FILE *stream, const header_t *header) {
	ppelib_reset_error();

	if (!header) {
		ppelib_set_error("NULL pointer");
		return;
	}
	fprintf(stream, "Machine: (0x%08X) %s\n", header->machine, map_lookup(header->machine, ppelib_machine_type_map));
	fprintf(stream, "NumberOfSections: %u\n", header->number_of_sections);
	char time_out_time_date_stamp[50];
	struct tm tm_time_date_stamp;
	time_t time_time_date_stamp = (time_t)header->time_date_stamp;
	gmtime_r(&time_time_date_stamp, &tm_time_date_stamp);
	strftime(time_out_time_date_stamp, sizeof(time_out_time_date_stamp), "%a, %d %b %Y %T %z", &tm_time_date_stamp);
	time_out_time_date_stamp[sizeof(time_out_time_date_stamp) - 1] = 0;
	fprintf(stream, "TimeDateStamp: %s\n", time_out_time_date_stamp);
	fprintf(stream, "PointerToSymbolTable: 0x%08X\n", header->pointer_to_symbol_table);
	fprintf(stream, "NumberOfSymbols: %u\n", header->number_of_symbols);
	fprintf(stream, "SizeOfOptionalHeader: %u\n", header->size_of_optional_header);
	fprintf(stream, "Characteristics: (0x%08X) ", header->characteristics);
	const ppelib_map_entry_t *characteristics_map_i = ppelib_characteristics_map;
	while (characteristics_map_i->string) {
		if (CHECK_BIT(header->characteristics, characteristics_map_i->value)) {
			fprintf(stream, "%s ", characteristics_map_i->string);
		}
		++characteristics_map_i;
	}
	fprintf(stream, "\n");
	fprintf(stream, "Magic: (0x%08X) %s\n", header->magic, map_lookup(header->magic, ppelib_magic_type_map));
	fprintf(stream, "MajorLinkerVersion: %u\n", header->major_linker_version);
	fprintf(stream, "MinorLinkerVersion: %u\n", header->minor_linker_version);
	fprintf(stream, "SizeOfCode: %u\n", header->size_of_code);
	fprintf(stream, "SizeOfInitializedData: %u\n", header->size_of_initialized_data);
	fprintf(stream, "SizeOfUninitializedData: %u\n", header->size_of_uninitialized_data);
	fprintf(stream, "AddressOfEntryPoint: 0x%08X\n", header->address_of_entry_point);
	fprintf(stream, "BaseOfCode: 0x%08X\n", header->base_of_code);
	if (header->magic == PE32_MAGIC) {
		fprintf(stream, "BaseOfData: 0x%08X\n", header->base_of_data);
	}
	fprintf(stream, "ImageBase: 0x%08" PRIx64 "\n", header->image_base);
	fprintf(stream, "SectionAlignment: %u\n", header->section_alignment);
	fprintf(stream, "FileAlignment: %u\n", header->file_alignment);
	fprintf(stream, "MajorOperatingSystemVersion: %u\n", header->major_operating_system_version);
	fprintf(stream, "MinorOperatingSystemVersion: %u\n", header->minor_operating_system_version);
	fprintf(stream, "MajorImageVersion: %u\n", header->major_image_version);
	fprintf(stream, "MinorImageVersion: %u\n", header->minor_image_version);
	fprintf(stream, "MajorSubsystemVersion: %u\n", header->major_subsystem_version);
	fprintf(stream, "MinorSubsystemVersion: %u\n", header->minor_subsystem_version);
	fprintf(stream, "Win32VersionValue: %u\n", header->win32_version_value);
	fprintf(stream, "SizeOfImage: %u\n", header->size_of_image);
	fprintf(stream, "SizeOfHeaders: %u\n", header->size_of_headers);
	fprintf(stream, "Checksum: %u\n", header->checksum);
	fprintf(stream, "Subsystem: (0x%08X) %s\n", header->subsystem, map_lookup(header->subsystem, ppelib_windows_subsystem_map));
	fprintf(stream, "DllCharacteristics: (0x%08X) ", header->dll_characteristics);
	const ppelib_map_entry_t *dll_characteristics_map_i = ppelib_dll_characteristics_map;
	while (dll_characteristics_map_i->string) {
		if (CHECK_BIT(header->dll_characteristics, dll_characteristics_map_i->value)) {
			fprintf(stream, "%s ", dll_characteristics_map_i->string);
		}
		++dll_characteristics_map_i;
	}
	fprintf(stream, "\n");
	fprintf(stream, "SizeOfStackReserve: %" PRIu64 "\n", header->size_of_stack_reserve);
	fprintf(stream, "SizeOfStackCommit: %" PRIu64 "\n", header->size_of_stack_commit);
	fprintf(stream, "SizeOfHeapReserve: %" PRIu64 "\n", header->size_of_heap_reserve);
	fprintf(stream, "SizeOfHeapCommit: %" PRIu64 "\n", header->size_of_heap_commit);
	fprintf(stream, "LoaderFlags: %u\n", header->loader_flags);
	fprintf(stream, "NumberOfRvaAndSizes: %u\n", header->number_of_rva_and_sizes);
}

void header_print(const header_t *header) {
	header_fprint(stdout, header);
}
