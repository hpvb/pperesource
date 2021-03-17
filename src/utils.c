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

#include <iconv.h>

#ifdef LIBICONV_INTERNAL
#define iconv_t ppelib_iconv_t
#define iconv ppelib_iconv
#define iconv_open ppelib_iconv_open
#define iconv_close ppelib_iconv_close
#endif

#include "platform.h"
#include "ppe_error.h"
#include "utils.h"

uint8_t read_uint8_t(const uint8_t *buffer) {
	return *buffer;
}

void write_uint8_t(uint8_t *buffer, uint8_t val) {
	buffer[0] = val;
}

uint16_t read_uint16_t(const uint8_t *buffer) {
	uint16_t retval;
	memcpy(&retval, buffer, sizeof(uint16_t));
	return retval;
}

void write_uint16_t(uint8_t *buffer, uint16_t val) {
	memcpy(buffer, &val, sizeof(uint16_t));
}

uint32_t read_uint32_t(const uint8_t *buffer) {
	uint32_t retval;
	memcpy(&retval, buffer, sizeof(uint32_t));
	return retval;
}

void write_uint32_t(uint8_t *buffer, uint32_t val) {
	memcpy(buffer, &val, sizeof(uint32_t));
}

uint64_t read_uint64_t(const uint8_t *buffer) {
	uint64_t retval;
	memcpy(&retval, buffer, sizeof(uint64_t));
	return retval;
}

void write_uint64_t(uint8_t *buffer, uint64_t val) {
	memcpy(buffer, &val, sizeof(uint64_t));
}

uint16_t buffer_excise(uint8_t **buffer, size_t size, size_t start, size_t end) {
	if (start >= end) {
		return 0;
	}

	if (size - (end - start) == 0) {
		free(*buffer);
		*buffer = NULL;
	}

	if (end != size) {
		memmove((*buffer) + start, (*buffer) + end, size - end);
	}

	uint8_t *oldptr = *buffer;
	*buffer = realloc(*buffer, size - (end - start));
	if (!*buffer) {
		*buffer = oldptr;
		return 0;
	}

	return 1;
}

uint32_t next_pow2(uint32_t number) {
	number--;
	number |= number >> 1;
	number |= number >> 2;
	number |= number >> 4;
	number |= number >> 8;
	number |= number >> 16;
	number++;

	number = (number == 1) ? 2 : number;
	return number;
}

// TODO find actual hard information on this
uint32_t get_machine_page_size(enum ppelib_machine_type machine) {
	switch (machine) {
	case IMAGE_FILE_MACHINE_IA64:
	case IMAGE_FILE_MACHINE_ALPHA:
	case IMAGE_FILE_MACHINE_ALPHA64:
		return 0x2000;
	default:
		return 0x1000;
	}
}

const char *map_lookup(uint32_t value, const ppelib_map_entry_t *map) {
	const ppelib_map_entry_t *m = map;
	while (m->string) {
		if (m->value == value) {
			return m->string;
		}
		++m;
	}

	return NULL;
}

char *get_utf16_string(const uint8_t *buffer, size_t size, size_t offset, size_t string_size) {
	if (offset + string_size > size) {
		ppelib_set_error("Not enough space for string");
		return NULL;
	}

	size_t outstring_size = string_size * 2;
	if (!outstring_size) {
		outstring_size = 2;
	}
	char *string = calloc(outstring_size, 1);
	if (!string) {
		ppelib_set_error("Failed to allocate string");
		return NULL;
	}

	size_t insize = string_size;
	size_t outsize = outstring_size;
	char *instring = (char *)buffer + offset;
	char *outstring = string;

	iconv_t cd = iconv_open("UTF-8", "UTF-16LE");
	if (cd == (iconv_t)-1) {
		ppelib_set_error("iconv_open failed");
		free(string);
		return NULL;
	}
	size_t ret = iconv(cd, &instring, &insize, &outstring, &outsize);
	if (ret == (size_t)-1) {
		ppelib_set_error("string conversion failed");
		free(string);
		iconv_close(cd);
		return NULL;
	}

	iconv_close(cd);
	//
	//	printf("get_utf16_string: outsize: %zi, LENGTH: %zi, DEBUG: '%s'\n", outstring_size, outstring_size - outsize, string);
	//
	//	outstring = string;
	//	printf("outstring: ");
	//	for (size_t i = 0; i < outstring_size - outsize; ++i) {
	//		printf("0x%02X ", string[i]);
	//	}
	//	printf("\n");
	//	printf("instring: ");
	//	for (size_t i = 0; i < string_size; ++i) {
	//		printf("0x%02X ", *((char *)buffer + offset + i));
	//	}
	//	printf("\n");
	//printf("get_utf16_string: '%s', %zi\n", string, string_size);
	return string;
}

size_t convert_utf8_string(const char *string, char **outstring) {
	size_t string_size = strlen(string);
	size_t outstring_size = (string_size + 1) * 2;

	*outstring = calloc(outstring_size, 1);

	size_t insize = string_size;
	size_t outsize = outstring_size;
	char *instring = (char *)string;
	char *outstring_ptr = *outstring;

	iconv_t cd = iconv_open("UTF-16LE", "UTF-8");
	if (cd == (iconv_t)-1) {
		ppelib_set_error("iconv_open failed");
		free(*outstring);
		return 0;
	}
	size_t ret = iconv(cd, &instring, &insize, &outstring_ptr, &outsize);
	if (ret == (size_t)-1) {
		ppelib_set_error("string conversion failed");
		free(*outstring);
		iconv_close(cd);
		return 0;
	}

	iconv_close(cd);

	//	printf("Instring: %s\n", string);
	//	printf("convert_utf8_string: LENGTH: %zi, DEBUG: %s\n", outstring_size - outsize, string);
	//
	//	outstring_ptr = *outstring;
	//	for (size_t i = 0; i < outstring_size - outsize; ++i) {
	//		printf("0x%02X ", outstring_ptr[i]);
	//	}
	//	printf("\n");
	//	for (size_t i = 0; i < string_size; ++i) {
	//		printf("0x%02X ", string[i]);
	//	}
	//	printf("\n");
	return outstring_size - outsize;
}
