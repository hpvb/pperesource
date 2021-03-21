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
#include <string.h>
#include <time.h>

#include "lodepng.h"

#include "pe/constants.h"

#include "platform.h"
#include "ppe_error.h"

#include "resources/icon_group.h"
#include "resources/resource.h"
#include "utils.h"

static const uint8_t png_header[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};

static int dimcmp(const void *a, const void *b) {
	icon_t *ia = (icon_t *)a;
	icon_t *ib = (icon_t *)b;

	return (ib->width * ib->height) - (ia->width * ia->height);
}

static int bppcmp(const void *a, const void *b) {
	icon_t *ia = (icon_t *)a;
	icon_t *ib = (icon_t *)b;

	return ib->bpp - ia->bpp;
}

static resource_t *find_icon(const resource_table_t *resource_table, uint16_t icon_id, uint32_t language_id) {
	resource_t *icon = NULL;

	for (size_t i = 0; i < resource_table->size; ++i) {
		resource_t *resource = resource_table->resources[i];
		if (resource->type_id == RT_ICON && resource->name_id == icon_id) {
			icon = resource;
			if (resource->language_id == language_id) {
				return icon;
			}
		}
	}

	return icon;
}

static char get_dib_mask(const uint8_t *mask, uint32_t width, uint32_t height, uint32_t x, uint32_t y) {
	size_t mask_bytes_per_line = TO_NEAREST(width / 8, 4);

	size_t mask_offset = ((height / 2) - y - 1) * mask_bytes_per_line + (x / 8);
	uint8_t bit_offset = 7 - (x % 8);

	return !!CHECK_BIT(mask[mask_offset], 1 << bit_offset);
}

static void decode_dib(const uint8_t *buffer, size_t size, resource_t *resource) {
	if (size < 4) {
		ppelib_set_error("DIB file too small");
		return;
	}

	uint32_t header_size = read_uint32_t(buffer);

	if (size < header_size) {
		ppelib_set_error("DIB file too small for header");
		return;
	}

	if (header_size != 40) {
		ppelib_set_error("Unknown DIB header size");
		return;
	}

	uint32_t width = read_uint32_t(buffer + 4);
	uint32_t height = read_uint32_t(buffer + 8);
	uint16_t planes = read_uint16_t(buffer + 12);
	uint16_t bpp = read_uint16_t(buffer + 14);
	uint32_t compression = read_uint32_t(buffer + 16);
	//uint32_t image_size = read_uint32_t(buffer + 20);
	//uint32_t horizontal_bpm = read_uint32_t(buffer + 24);
	//uint32_t vertical_bpm = read_uint32_t(buffer + 28);
	uint32_t palette_colors = read_uint32_t(buffer + 32);
	uint32_t important_colors = read_uint32_t(buffer + 36);

	printf("DIB: planes: %i, bpp: %i, compression: %i, palette_colors: %i, important_colors: %i\n", planes, bpp, compression, palette_colors, important_colors);
	size_t pixel_offset = header_size;

	uint16_t channels = bpp / 8;
	uint16_t divider = 1;

	switch (bpp) {
	case 1:
		channels = 1;
		divider = 8;
		palette_colors = palette_colors ? palette_colors : 2;
		break;
	case 4:
		channels = 1;
		divider = 2;
		palette_colors = palette_colors ? palette_colors : 16;
		break;
	case 8:
		palette_colors = palette_colors ? palette_colors : 256;
		break;
	case 24:
	case 32:
		palette_colors = 0;
		break;
	default:
		ppelib_set_error("Unknown BPP\n");
		return;
		break;
	}

	pixel_offset += (palette_colors * 4);

	size_t bytes_per_line = TO_NEAREST((width * channels) / divider, 4);
	size_t mask_bytes_per_line = TO_NEAREST(width / 8, 4);

	size_t mask_start = pixel_offset + (height / 2) * bytes_per_line;

	if (size < mask_start + ((height / 2) * mask_bytes_per_line)) {
		ppelib_set_error("Not enough space for DIB image data");
		return;
	}

	uint8_t *image = calloc(width * (height / 2) * 4, 1);

	size_t image_offset = 0;
	for (uint32_t y = 0; y < height / 2; y++) {
		for (uint32_t x = 0; x < width / divider; x++) {
			size_t bmp_offset = pixel_offset + ((height / 2) - y - 1) * bytes_per_line + channels * x;

			for (uint8_t i = 0; i < divider; ++i) {
				if (!get_dib_mask(buffer + mask_start, width, height, (x * divider) + i, y)) {
					if (bpp <= 8) {
						uint8_t pixel = 0;

						if (bpp == 1) {
							pixel = !!CHECK_BIT(buffer[bmp_offset], 1 << (7 - i));
						} else if (bpp == 4) {
							pixel = (buffer[bmp_offset] >> ((1 - i) * 4)) & 0x0F;
						} else if (bpp == 8) {
							pixel = buffer[bmp_offset];
						}

						if (pixel > palette_colors) {
							pixel = 0;
						}

						const uint8_t *palette = buffer + header_size + (pixel * 4);

						image[image_offset + 0] = palette[2]; // R
						image[image_offset + 1] = palette[1]; // G
						image[image_offset + 2] = palette[0]; // B

						if (palette[3]) {
							image[image_offset + 3] = palette[3]; // A
						} else {
							image[image_offset + 3] = 0xFF; // A
						}

					} else if (bpp >= 24) {
						const uint8_t *pixel = buffer + bmp_offset;

						image[image_offset + 0] = pixel[2]; // R
						image[image_offset + 1] = pixel[1]; // G
						image[image_offset + 2] = pixel[0]; // B

						if (bpp == 24) {
							image[image_offset + 3] = 0xff; // A
						} else {
							image[image_offset + 3] = pixel[3]; // A
						}
					}
				}

				image_offset += 4;
			}
		}
	}

#ifndef FUZZ
	unsigned char *png;
	size_t pngsize;
	unsigned error = lodepng_encode32(&png, &pngsize, image, width, height / 2);
	if (error) {
		ppelib_set_error("Failed to encode png");
	}

	free(resource->data);
	resource->data = png;
	resource->size = pngsize;
#else
	(void)resource;
#endif

	free(image);
}

static void parse_icon(const uint8_t *buffer, size_t size, size_t offset, resource_table_t *resource_table, icon_group_t *icon_group) {
	if (size < offset + 14) {
		ppelib_set_error("Too little room for icon entry");
		return;
	}

	uint8_t width = read_uint8_t(buffer + offset + 0);
	uint8_t height = read_uint8_t(buffer + offset + 1);
	uint8_t color_count = read_uint8_t(buffer + offset + 2);
	uint8_t reserved = read_uint8_t(buffer + offset + 3);
	uint16_t planes = read_uint16_t(buffer + offset + 4);
	uint16_t bpp = read_uint16_t(buffer + offset + 6);
	//uint32_t image_size = read_uint32_t(buffer + offset + 8);
	uint16_t icon_id = read_uint16_t(buffer + offset + 12);

	resource_t *icon_res = find_icon(resource_table, icon_id, icon_group->resource->language_id);
	if (!icon_res) {
		ppelib_set_error("Icon not in resource table");
		return;
	}

	++icon_group->numb_icons;
	icon_group->icons = realloc(icon_group->icons, icon_group->numb_icons * sizeof(icon_t));
	if (!icon_group->icons) {
		ppelib_set_error("Failed to allocate icon");
		return;
	}

	icon_t *icon = &icon_group->icons[icon_group->numb_icons - 1];
	memset(icon, 0, sizeof(icon_t));

	if (icon_res->size > sizeof(png_header)) {
		if (memcmp(icon_res->data, png_header, sizeof(png_header)) == 0) {
			icon->type = ICON_TYPE_PNG;
		} else {
			icon->type = ICON_TYPE_DIB;
		}
	}

	icon->width = width ? width : 256;
	icon->height = height ? height : 256;
	icon->color_count = color_count;
	icon->reserved = reserved;
	icon->planes = planes;
	icon->bpp = bpp;

	icon->size = icon_res->size;
	icon->data = malloc(icon_res->size);
	icon->resource = icon_res;
	memcpy(icon->data, icon_res->data, icon->size);

	if (icon->type == ICON_TYPE_PNG) {
#ifndef FUZZ
		uint32_t error;
		uint8_t *image = 0;
		uint32_t png_width, png_height;
		//size_t pngsize;
		LodePNGState state;

		lodepng_state_init(&state);
		state.decoder.zlibsettings.ignore_adler32 = 1;
		//state.decoder.ignore_crc = 1;
		error = lodepng_decode(&image, &png_width, &png_height, &state, icon->data, icon->size);
		if (error) {
			ppelib_set_error("Failed to decode png");
			//printf("%s\n", lodepng_error_text(error));
			return;
		}
		//uint32_t png_bpp = lodepng_get_bpp(&state.info_png.color);
		//printf("Size: %ix%i@%i png: %ix%i@%i\n", icon->width, icon->height, icon->bpp, png_width, png_height, png_bpp);

		lodepng_state_cleanup(&state);
		free(image);
#endif
	} else {
		//printf("Icon: %i\n", icon_id);
		decode_dib(icon->data, icon->size, icon_res);
	}

	char filename[100];
	int fs = snprintf(filename, 99, "dump/icon%i_%i.png", icon_res->name_id, icon_res->language_id);
	filename[fs] = 0;
	FILE *fp = fopen(filename, "w");
	fwrite(icon_res->data, icon_res->size, 1, fp);
	fclose(fp);
}

void icon_group_deserialize(resource_table_t *resource_table, resource_t *resource, icon_group_t *icon_group) {
	ppelib_reset_error();

	uint8_t *buffer = resource->data;
	size_t size = resource->size;

	icon_group->resource = resource;

	if (size < 6) {
		ppelib_set_error("Too little room for vsersioninfo");
		return;
	}

	//uint16_t reserved = read_uint16_t(buffer + 0);
	//uint16_t resource_type = read_uint16_t(buffer + 2);
	uint16_t resource_count = read_uint16_t(buffer + 4);

	if (size < (size_t)(6 + (resource_count * 14))) {
		ppelib_set_error("Too little room for icon entries");
		return;
	}

	for (size_t i = 0; i < resource_count; ++i) {
		parse_icon(buffer, size, 6 + (i * 14), resource_table, icon_group);
		if (ppelib_error_peek()) {
			return;
		}
	}

	if (icon_group->numb_icons) {
		qsort(icon_group->icons, icon_group->numb_icons, sizeof(icon_t), &dimcmp);
		qsort(icon_group->icons, icon_group->numb_icons, sizeof(icon_t), &bppcmp);
	}
}
