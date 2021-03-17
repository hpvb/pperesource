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

#ifndef PPELIB_INTERNAL_H_
#define PPELIB_INTERNAL_H_

#include <inttypes.h>
#include <stddef.h>

#include "main.h"
#include "platform.h"

#include "pe/section_private.h"

section_t *section_find_by_physical_address(ppelib_file_t *pe, size_t address);
section_t *section_find_by_virtual_address(ppelib_file_t *pe, size_t va);
size_t section_rva_to_offset(const section_t *section, size_t rva);
void sort_sections(ppelib_file_t *pe);
void *section_rva_to_pointer(const section_t *section, size_t rva);
uint16_t section_create(ppelib_file_t *pe, char name[9], uint32_t virtual_size, uint32_t raw_size,
		uint32_t characteristics, uint8_t *data);
void section_resize(ppelib_file_t *pe, uint16_t section_index, size_t size);
uint16_t section_find_index(ppelib_file_t *pe, section_t *section);

#endif /* PPELIB_INTERNAL_H_ */
