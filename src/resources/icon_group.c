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

#include "pe/constants.h"

#include "platform.h"
#include "ppe_error.h"

#include "resources/icon_group.h"
#include "resources/resource.h"
#include "utils.h"

void icon_group_free(icon_group_t *icon_group) {
	for (size_t i = 0; i < icon_group->numb_icons; ++i) {
		free(icon_group->icons[i].data);
	}

	free(icon_group->icons);
}

void icon_group_print(icon_group_t *icon_group) {
	printf("Number of icons: %zi\n", icon_group->numb_icons);

	for (size_t i = 0; i < icon_group->numb_icons; ++i) {
		icon_t *icon = &icon_group->icons[i];
		if (icon->type == ICON_TYPE_PNG) {
			printf("  Type: PNG ");
		} else {
			printf("  Type: DIB ");
		}

		int name_id = -1;
		if (icon->resource) {
			name_id = (int)icon->resource->name_id;
		}
		if (name_id >= 0) {
			printf("ID: %i Dimensions: %ix%i@%i Size: %zi bytes\n", name_id, icon->width, icon->height, icon->bpp, icon->size);
		} else {
			printf("Dimensions: %ix%i@%i Size: %zi bytes\n", icon->width, icon->height, icon->bpp, icon->size);
		}
	}
}
