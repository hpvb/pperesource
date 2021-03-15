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
#include <ppe_error.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pe/constants.h"
#include "resources/resource.h"

#include "main.h"
#include "ppelib_internal.h"

EXPORT_SYM ppelib_file_t *ppelib_create() {
	ppelib_reset_error();

	ppelib_file_t *pe = calloc(sizeof(ppelib_file_t), 1);
	if (!pe) {
		ppelib_set_error("Failed to allocate PE structure");
	}

	return pe;
}

EXPORT_SYM void ppelib_destroy(ppelib_file_t *pe) {
	if (!pe) {
		return;
	}

	if (pe->sections) {
		for (uint16_t i = 0; i < pe->header.number_of_sections; ++i) {
			free(pe->sections[i]->contents);
			free(pe->sections[i]);
		}
	}

	for (size_t i = 0; i < pe->resource_table.size; ++i) {
		free(pe->resource_table.resources[i]->type);
		free(pe->resource_table.resources[i]->name);
		free(pe->resource_table.resources[i]->language);
		free(pe->resource_table.resources[i]->data);
		free(pe->resource_table.resources[i]);
	}
	free(pe->resource_table.resources);

	free(pe->stub);
	free(pe->data_directories);
	free(pe->sections);
	free(pe->overlay);

	free(pe);
	pe = NULL;
}

EXPORT_SYM ppelib_file_t *ppelib_create_from_buffer(const uint8_t *buffer, size_t size) {
	ppelib_reset_error();

	if (size < 2) {
		ppelib_set_error("Not a PE file (too small for MZ signature)");
		return NULL;
	}

	uint16_t mz_signature = read_uint16_t(buffer);
	if (mz_signature != MZ_SIGNATURE) {
		ppelib_set_error("Not a PE file (MZ signature missing)");
		return NULL;
	}

	ppelib_file_t *pe = ppelib_create();
	if (ppelib_error_peek()) {
		return NULL;
	}

	if (size < 0x3c + sizeof(uint32_t)) {
		ppelib_set_error("File too small for PE header");
		goto out;
	}

	pe->pe_header_offset = read_uint32_t(buffer + 0x3C);

	if (size < pe->pe_header_offset + sizeof(uint32_t)) {
		ppelib_set_error("Not a PE file (file too small)");
		goto out;
	}

	pe->stub_size = pe->pe_header_offset;
	pe->stub = malloc(pe->stub_size);
	if (!pe->stub) {
		ppelib_set_error("Couldn't allocate DOS stub");
		goto out;
	}
	memcpy(pe->stub, buffer, pe->stub_size);

	uint32_t signature = read_uint32_t(buffer + pe->pe_header_offset);
	if (signature != PE_SIGNATURE) {
		ppelib_set_error("Not a PE file (PE00 signature missing)");
		goto out;
	}

	size_t header_offset = pe->pe_header_offset + 4;

	size_t header_size = header_deserialize(buffer, size, header_offset, &pe->header);
	if (ppelib_error_peek()) {
		goto out;
	}

	if (pe->header.number_of_rva_and_sizes > (UINT32_MAX / DATA_DIRECTORY_SIZE)) {
		//ppelib_set_error("File too small for directory entries (overflow)");
		//goto out;
		// Apparently this is what the Windows loader does for *any* value over 16?
		pe->header.number_of_rva_and_sizes = 16;
	}

	size_t data_directories_size = (pe->header.number_of_rva_and_sizes * DATA_DIRECTORY_SIZE);
	if (header_offset + header_size + data_directories_size > size) {
		pe->header.number_of_rva_and_sizes = MIN(pe->header.number_of_rva_and_sizes, 16);

		data_directories_size = (pe->header.number_of_rva_and_sizes * DATA_DIRECTORY_SIZE);
		if (header_offset + header_size + data_directories_size > size) {
			ppelib_set_error("File too small for directory entries");
			goto out;
		}
	}

	size_t section_offset = header_offset + COFF_HEADER_SIZE + pe->header.size_of_optional_header;
	pe->start_of_section_data = ((size_t)(pe->header.number_of_sections) * SECTION_SIZE) + section_offset;
	if (pe->start_of_section_data > size && pe->header.number_of_sections) {
		ppelib_set_error("File too small for section headers");
		goto out;
	}

	pe->sections = calloc(sizeof(void *) * pe->header.number_of_sections, 1);
	if (!pe->sections) {
		ppelib_set_error("Failed to allocate sections array");
		goto out;
	}

	for (uint16_t i = 0; i < pe->header.number_of_sections; ++i) {
		pe->sections[i] = calloc(sizeof(section_t), 1);
		if (!pe->sections[i]) {
			for (uint16_t s = 0; s < i; ++s) {
				free(pe->sections[s]);
			}
			memset(pe->sections, 0, sizeof(void *) * pe->header.number_of_sections);
			ppelib_set_error("Failed to allocate sections");
			goto out;
		}
	}

	size_t offset = section_offset;
	pe->start_of_section_va = 0;
	pe->end_of_section_data = pe->start_of_section_data;
	char first_section = 1;

	for (uint16_t i = 0; i < pe->header.number_of_sections; ++i) {
		size_t section_size = section_deserialize(buffer, size, offset, pe->sections[i]);
		if (ppelib_error_peek()) {
			goto out;
		}

		section_t *section = pe->sections[i];

		if (i == 0) {
			pe->start_of_section_va = section->virtual_address;
		} else {
			pe->start_of_section_va = MIN(pe->start_of_section_va, section->virtual_address);
		}

		if (section->size_of_raw_data > section->size_of_raw_data + section->virtual_size) {
			ppelib_set_error("Section data size out of range");
			goto out;
		}

		size_t data_size = MIN(section->virtual_size, section->size_of_raw_data);

		if (section->pointer_to_raw_data + data_size > size || section->pointer_to_raw_data > size || data_size > size || section->size_of_raw_data > size) {
			ppelib_set_error("Section data outside of file");
			goto out;
		}

		section->contents = malloc(data_size);
		if (!section->contents) {
			ppelib_set_error("Failed to allocate section data");
			goto out;
		}

		section->contents_size = data_size;
		memcpy(section->contents, buffer + section->pointer_to_raw_data, section->contents_size);

		if (section->pointer_to_raw_data) {
			if (first_section) {
				first_section = 0;
				pe->start_of_section_data = section->pointer_to_raw_data;
			} else {
				pe->start_of_section_data = MIN(pe->start_of_section_data, section->pointer_to_raw_data);
			}
		}

		pe->end_of_section_data = MAX(pe->end_of_section_data,
				section->pointer_to_raw_data + section->size_of_raw_data);

		offset += section_size;
	}

	pe->entrypoint_section = section_find_by_virtual_address(pe, pe->header.address_of_entry_point);

	if (pe->entrypoint_section) {
		pe->entrypoint_offset = pe->header.address_of_entry_point - pe->entrypoint_section->virtual_address;
	}

	pe->data_directories = calloc(sizeof(data_directory_t) * pe->header.number_of_rva_and_sizes, 1);
	if (!pe->data_directories) {
		ppelib_set_error("Failed to allocate data directories");
		goto out;
	}

	// Data directories don't have a dedicated deserialize function
	offset = header_offset + header_size;
	for (uint32_t i = 0; i < pe->header.number_of_rva_and_sizes; ++i) {
		uint32_t dir_va = read_uint32_t(buffer + offset + 0);
		uint32_t dir_size = read_uint32_t(buffer + offset + 4);

		section_t *section = section_find_by_virtual_address(pe, dir_va);
		if (i != DIR_CERTIFICATE_TABLE && section) {
			pe->data_directories[i].section = section;
			pe->data_directories[i].offset = dir_va - section->virtual_address;
		} else if (dir_size) {
			// Certificate tables' addresses aren't virtual. Despite the name.
			pe->data_directories[i].offset = dir_va - pe->end_of_section_data;
		}
		pe->data_directories[i].size = dir_size;
		pe->data_directories[i].id = i;

		offset += DATA_DIRECTORY_SIZE;
	}

	pe->end_of_section_data = MAX(pe->end_of_section_data, header_offset + header_size);
	if (size > pe->end_of_section_data) {
		pe->overlay_size = size - pe->end_of_section_data;
		pe->overlay = malloc(pe->overlay_size);
		if (!pe->overlay) {
			ppelib_set_error("Failed to allocate overlay data");
			goto out;
		}

		memcpy(pe->overlay, buffer + pe->end_of_section_data, pe->overlay_size);
	}

	if (pe->header.number_of_rva_and_sizes > DIR_RESOURCE_TABLE) {
		section_t *section = pe->data_directories[DIR_RESOURCE_TABLE].section;
		size_t offset = pe->data_directories[DIR_RESOURCE_TABLE].offset;

		if (section) {
			resource_table_deserialize(section, offset, &pe->resource_table);
			if (ppelib_error_peek()) {
				printf("Resource parse error: %s\n", ppelib_error());
			}
			ppelib_reset_error();
		}
	}

	resource_t *res = NULL;
	size_t nmb = get_resource_by_type_id(&pe->resource_table, RT_VERSION, &res);
	if (nmb == 1) {
		versioninfo_deserialize(res->data, res->size, 0);
	}
out:
	if (ppelib_error_peek()) {
		ppelib_destroy(pe);
		return NULL;
	}

	return pe;
}

EXPORT_SYM ppelib_file_t *ppelib_create_from_file(const char *filename) {
	ppelib_reset_error();
	size_t file_size;
	uint8_t *file_contents;

	FILE *f = fopen(filename, "rb");

	if (!f) {
		ppelib_set_error("Failed to open file");
		return NULL;
	}

	fseek(f, 0, SEEK_END);
	long ftell_size = ftell(f);
	rewind(f);

	if (ftell_size < 0) {
		fclose(f);
		ppelib_set_error("Unable to read file length");
		return NULL;
	}

	file_size = (size_t)ftell_size;

	if (!file_size) {
		fclose(f);
		ppelib_set_error("Empty file");
		return NULL;
	}

	file_contents = malloc(file_size);
	if (!file_size) {
		fclose(f);
		ppelib_set_error("Failed to allocate file data");
		return NULL;
	}

	size_t retsize = fread(file_contents, 1, file_size, f);
	if (retsize != file_size) {
		fclose(f);
		ppelib_set_error("Failed to read file data");
		return NULL;
	}

	fclose(f);

	ppelib_file_t *retval = ppelib_create_from_buffer(file_contents, file_size);
	free(file_contents);

	return retval;
}

EXPORT_SYM size_t ppelib_write_to_buffer(const ppelib_file_t *pe, uint8_t *buffer, size_t buf_size) {
	size_t size = 0;

	size_t header_size = header_serialize(&pe->header, NULL, 0);
	size_t data_tables_size = pe->header.number_of_rva_and_sizes * DATA_DIRECTORY_SIZE;
	size_t section_header_size = pe->header.number_of_sections * SECTION_SIZE;

	size_t section_size = 0;

	size_t pe_header_offset = pe->pe_header_offset + 4;
	size_t section_header_offset = pe_header_offset + COFF_HEADER_SIZE + pe->header.size_of_optional_header;

	uint32_t file_alignment = MAX(pe->header.file_alignment, 512);
	file_alignment = MIN(file_alignment, UINT16_MAX);

	for (uint16_t i = 0; i < pe->header.number_of_sections; ++i) {
		section_t *section = pe->sections[i];

		size_t this_section_size = section->pointer_to_raw_data;
		this_section_size += section->size_of_raw_data;
		section_size = MAX(section_size, this_section_size);
	}

	size += 2;
	size += pe->pe_header_offset;
	size += 4;
	size += pe->header.size_of_optional_header;
	size += section_header_size;

	// Some of this stuff may overlap so we need to ensure we have at least as much space
	// as the furthest out write
	size = MAX(size, section_size);
	size = MAX(size, pe_header_offset + header_size);
	size = MAX(size, pe_header_offset + header_size + data_tables_size);
	size = MAX(size, section_header_offset + section_header_size);

	size_t end_of_section_data = size;

	size += pe->overlay_size;

	//	printf("dos_header_size: %zi\n", pe->stub_size);
	//	//printf("dos_stub_size: %zi\n", dos_stub_size);
	//	printf("header_size: %zi\n", header_size);
	//	//printf("data_tables_size: %zi\n", data_tables_size);
	//	printf("section_header_size: %zi\n", section_header_size);
	//	printf("section_size: %zi\n", section_size);
	//	printf("overlay_size: %zi\n", pe->overlay_size);
	//	printf("total: %zi\n", size);

	if (!buffer) {
		return size;
	}

	if (buffer && size > buf_size) {
		ppelib_set_error("Target buffer too small.");
		return 0;
	}

	memset(buffer, 0, size);
	memcpy(buffer, pe->stub, pe->stub_size);
	write_uint32_t(buffer + pe->pe_header_offset, PE_SIGNATURE);
	header_serialize(&pe->header, buffer, pe_header_offset);

	size_t offset = pe_header_offset + header_size;
	for (uint32_t i = 0; i < pe->header.number_of_rva_and_sizes; ++i) {
		data_directory_t *dir = &pe->data_directories[i];
		section_t *section = dir->section;
		uint32_t dir_va = 0;
		uint32_t dir_size = (uint32_t)dir->size;

		if (section) {
			dir_va = (uint32_t)(section->virtual_address + dir->offset);
		} else if (dir->size) {
			dir_va = (uint32_t)(end_of_section_data + dir->offset);
		}

		write_uint32_t(buffer + offset + 0, dir_va);
		write_uint32_t(buffer + offset + 4, dir_size);

		offset += DATA_DIRECTORY_SIZE;
	}

	offset = section_header_offset;
	for (uint16_t i = 0; i < pe->header.number_of_sections; ++i) {
		section_t *section = pe->sections[i];
		section_serialize(section, buffer, offset);

		if (section->contents_size) {
			memcpy(buffer + section->pointer_to_raw_data, section->contents, section->contents_size);
		}

		offset += SECTION_SIZE;
	}

	if (pe->overlay_size) {
		memcpy(buffer + end_of_section_data, pe->overlay, pe->overlay_size);
	}

	return size;
}

EXPORT_SYM size_t ppelib_write_to_file(const ppelib_file_t *pe, const char *filename) {
	ppelib_reset_error();

	FILE *f = fopen(filename, "wb");
	if (!f) {
		ppelib_set_error("Failed to open file");
		return 0;
	}

	size_t bufsize = ppelib_write_to_buffer(pe, NULL, 0);
	if (ppelib_error_peek()) {
		fclose(f);
		return 0;
	}

	uint8_t *buffer = malloc(bufsize);
	if (!buffer) {
		ppelib_set_error("Failed to allocate buffer");
		fclose(f);
		return 0;
	}

	ppelib_write_to_buffer(pe, buffer, bufsize);
	if (ppelib_error_peek()) {
		fclose(f);
		free(buffer);
		return 0;
	}

	size_t written = fwrite(buffer, 1, bufsize, f);
	fclose(f);
	free(buffer);

	if (written != bufsize) {
		ppelib_set_error("Failed to write data");
	}

	return written;
}

void recalculate_sections(ppelib_file_t *pe) {
	uint32_t base_of_code = 0;
	uint32_t base_of_data = 0;
	uint32_t size_of_initialized_data = 0;
	uint32_t size_of_uninitialized_data = 0;
	uint32_t size_of_code = 0;

	uint32_t next_section_virtual = (uint32_t)pe->start_of_section_va;
	uint32_t next_section_physical = (uint32_t)pe->start_of_section_data;

	next_section_virtual = MAX(next_section_virtual, next_section_physical);

	section_t *resource_section = pe->data_directories[DIR_RESOURCE_TABLE].section;
	if (resource_section) {
		uint16_t section_index = section_find_index(pe, resource_section);
		size_t end_of_section_va = 0;

		for (uint16_t i = 0; i < pe->header.number_of_sections; ++i) {
			section_t *section = pe->sections[i];
			end_of_section_va = MAX(end_of_section_va, section->virtual_address + section->virtual_size);
		}

		if (section_index < pe->header.number_of_sections - 1) {
			section_t *next_section = pe->sections[section_index + 1];
			size_t end_offset = resource_section->virtual_address + resource_section->virtual_size;

			if (end_offset > next_section->virtual_address) {
				resource_section->virtual_address = (uint32_t)TO_NEAREST(end_of_section_va, pe->header.section_alignment);
			}
		}

		if (section_index == pe->header.number_of_sections - 1) {
			if (!resource_section->virtual_address) {
				resource_section->virtual_address = (uint32_t)TO_NEAREST(end_of_section_va, pe->header.section_alignment);
			}
		}

		resource_section->virtual_size = (uint32_t)resource_section->contents_size;
		resource_section->size_of_raw_data = TO_NEAREST((uint32_t)resource_section->contents_size, pe->header.file_alignment);
	}

	for (uint16_t i = 0; i < pe->header.number_of_sections; ++i) {
		section_t *section = pe->sections[i];

		// SizeOfRawData can't be more than the aligned amount of the data we actually have
		if (section->size_of_raw_data > TO_NEAREST(section->contents_size, pe->header.file_alignment)) {
			section->size_of_raw_data = TO_NEAREST((uint32_t)section->contents_size, pe->header.file_alignment);
		}

		if (section->size_of_raw_data) {
			if (section->pointer_to_raw_data != next_section_physical) {
				uint32_t next_section_physical_aligned = TO_NEAREST(next_section_physical,
						pe->header.section_alignment);

				if (section->pointer_to_raw_data != next_section_physical_aligned) {
					section->pointer_to_raw_data = next_section_physical_aligned;
				}
			}
		}

		if (CHECK_BIT(section->characteristics, IMAGE_SCN_CNT_CODE)) {
			if (!base_of_code) {
				base_of_code = section->virtual_address;
			}

			// This appears to hold empirically true.
			if (strcmp(".bind", section->name) != 0) {
				size_of_code += TO_NEAREST(section->virtual_size, pe->header.file_alignment);
			}
		}

		if (!base_of_data && !CHECK_BIT(section->characteristics, IMAGE_SCN_CNT_CODE)) {
			base_of_data = section->virtual_address;
		}

		if (CHECK_BIT(section->characteristics, IMAGE_SCN_CNT_INITIALIZED_DATA)) {
			// This appears to hold empirically true.
			if (pe->header.magic == PE32_MAGIC) {
				uint32_t vs = TO_NEAREST(section->virtual_size, pe->header.file_alignment);
				uint32_t rs = section->size_of_raw_data;
				size_of_initialized_data += MAX(vs, rs);
			} else if (pe->header.magic == PE32PLUS_MAGIC) {
				size_of_initialized_data += TO_NEAREST(section->size_of_raw_data, pe->header.file_alignment);
			}
		}

		if (CHECK_BIT(section->characteristics, IMAGE_SCN_CNT_UNINITIALIZED_DATA)) {
			size_of_uninitialized_data += TO_NEAREST(section->virtual_size, pe->header.file_alignment);
		}

		if (section->size_of_raw_data) {
			next_section_physical = TO_NEAREST(section->pointer_to_raw_data, pe->header.file_alignment);
			next_section_physical += TO_NEAREST(section->size_of_raw_data, pe->header.file_alignment);
		}

		if (section->virtual_size) {
			next_section_virtual = TO_NEAREST(section->virtual_address, pe->header.section_alignment);
			next_section_virtual += TO_NEAREST(section->virtual_size, pe->header.section_alignment);
		}

		pe->end_of_section_data = MAX(pe->end_of_section_data,
				section->pointer_to_raw_data + section->size_of_raw_data);
	}

	// PE files with only data can have this set to garbage. Might as well just keep it.
	if (size_of_code) {
		pe->header.base_of_code = base_of_code;
	}

	// The actual value of these of PE images in the wild varies a lot.
	// There doesn't appear to be an actual correct way of calculating these

	pe->header.base_of_data = base_of_data;
	pe->header.size_of_initialized_data = TO_NEAREST(size_of_initialized_data, pe->header.file_alignment);
	pe->header.size_of_uninitialized_data = TO_NEAREST(size_of_uninitialized_data, pe->header.file_alignment);
	pe->header.size_of_code = TO_NEAREST(size_of_code, pe->header.file_alignment);
	pe->header.size_of_image = next_section_virtual;
	if (pe->entrypoint_section) {
		pe->header.address_of_entry_point = pe->entrypoint_section->virtual_address + (uint32_t)pe->entrypoint_offset;
	}
}

section_t *create_rscs_section(ppelib_file_t *pe, size_t resource_table_size) {
	section_t *section = NULL;

	uint16_t section_index = section_create(pe, ".rscs", 0, (uint32_t)resource_table_size,
			IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ, NULL);
	section = pe->sections[section_index];

	pe->data_directories[DIR_RESOURCE_TABLE].section = section;
	pe->data_directories[DIR_RESOURCE_TABLE].offset = 0;
	pe->data_directories[DIR_RESOURCE_TABLE].size = resource_table_size;

	return section;
}

void recalculate_header(ppelib_file_t *pe) {
	size_t resource_table_size = resource_table_serialize(NULL, 0, &pe->resource_table);
	section_t *resource_section = NULL;
	size_t resource_offset = 0;
	size_t resource_size = 0;

	if (pe->header.number_of_rva_and_sizes > DIR_RESOURCE_TABLE) {
		resource_section = pe->data_directories[DIR_RESOURCE_TABLE].section;
		resource_offset = pe->data_directories[DIR_RESOURCE_TABLE].offset;
		resource_size = pe->data_directories[DIR_RESOURCE_TABLE].size;
	} else {
		pe->data_directories = realloc(pe->data_directories, sizeof(data_directory_t) * 16);
		memset(pe->data_directories + pe->header.number_of_rva_and_sizes, 0, sizeof(data_directory_t) * (16 - pe->header.number_of_rva_and_sizes));
		pe->header.number_of_rva_and_sizes = 16;
	}

	if (resource_table_size) {
		if (!resource_section) {
			resource_section = create_rscs_section(pe, resource_table_size);
		} else {
			uint16_t section_index = section_find_index(pe, resource_section);

			if (resource_section->contents_size == resource_size && !resource_offset) {
				// Old rscs only had our resources in it
				section_resize(pe, section_index, resource_table_size);
			} else {
				// Old rscs had more stuff in it just make a new one
				resource_section = create_rscs_section(pe, resource_table_size);
				resource_offset = 0;
			}
		}

		pe->data_directories[DIR_RESOURCE_TABLE].section = resource_section;
		pe->data_directories[DIR_RESOURCE_TABLE].offset = resource_offset;
		pe->data_directories[DIR_RESOURCE_TABLE].size = resource_table_size;
	}

	uint16_t header_size = (uint16_t)header_serialize(&pe->header, NULL, 0);
	size_t data_tables_size = pe->header.number_of_rva_and_sizes * DATA_DIRECTORY_SIZE;
	size_t section_header_size = pe->header.number_of_sections * SECTION_SIZE;

	size_t total_header_size = pe->pe_header_offset + 4 + header_size + data_tables_size + section_header_size;

	if (!pe->header.file_alignment || pe->header.file_alignment > UINT16_MAX) {
		pe->header.file_alignment = 512;
	}

	if (pe->header.file_alignment > 512) {
		pe->header.file_alignment = next_pow2(pe->header.file_alignment);
	}

	if (!pe->header.section_alignment || pe->header.section_alignment > UINT16_MAX || pe->header.section_alignment < pe->header.file_alignment) {
		pe->header.section_alignment = get_machine_page_size(pe->header.machine);
	}

	if (pe->header.section_alignment > get_machine_page_size(pe->header.machine)) {
		pe->header.section_alignment = next_pow2(pe->header.section_alignment);
	}

	if (TO_NEAREST(total_header_size, pe->header.file_alignment) > UINT32_MAX) {
		pe->header.size_of_headers = 0;
	} else {
		pe->header.size_of_headers = (uint32_t)(TO_NEAREST(total_header_size, pe->header.file_alignment));
	}

	pe->header.size_of_optional_header = (uint16_t)data_tables_size;

	if (pe->header.magic == PE32_MAGIC) {
		pe->header.size_of_optional_header += PE_OPTIONAL_HEADER_SIZE;
	}

	if (pe->header.magic == PE32PLUS_MAGIC) {
		pe->header.size_of_optional_header += PEPLUS_OPTIONAL_HEADER_SIZE;
	}

	pe->start_of_section_data = MAX(pe->start_of_section_data, pe->header.size_of_headers);
}

void ppelib_recalculate(ppelib_file_t *pe) {
	if (!pe) {
		return;
	}

	recalculate_header(pe);
	recalculate_sections(pe);
}
