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

#ifndef SRC_RESOURCES_ICON_GROUP_H_
#define SRC_RESOURCES_ICON_GROUP_H_

#include <stddef.h>

typedef struct resource_table resource_table_t;
typedef struct resource resource_t;

typedef enum {
	BI_RGB = 0,
	BI_RLE8 = 1,
	BI_RLE4 = 2,
	BI_BITFIELDS = 3,
	BI_JPEG = 4,
	BI_PNG = 5,
	BI_ALPHABITFIELDS = 6,
	BI_CMYK = 11,
	BI_CMYKRLE8 = 12,
	BI_CMYKRLE4 = 13,
} dib_compression;

typedef enum {
	ICON_TYPE_PNG,
	ICON_TYPE_DIB,
} image_type;

typedef struct icon {
	image_type type;

	uint16_t width;
	uint16_t height;
	uint8_t color_count;
	uint8_t reserved;
	uint16_t planes;
	uint16_t bpp;

	size_t size;
	uint8_t *data;

	resource_t *resource;
} icon_t;

typedef struct icon_group {
	size_t numb_icons;
	icon_t *icons;

	resource_t *resource;
} icon_group_t;

void icon_group_free(icon_group_t *icon_group);
void icon_group_deserialize(resource_table_t *resource_table, resource_t *resource, icon_group_t *icon_group);
void icon_group_print(icon_group_t *icon_group);

#endif /* SRC_RESOURCES_ICON_GROUP_H_ */
