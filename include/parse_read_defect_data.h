/* Copyright 2015 Baruch Even
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef LIBSCSICMD_READ_DEFECT_DATA_H
#define LIBSCSICMD_READ_DEFECT_DATA_H

#include "scsicmd_utils.h"
#include "scsicmd.h"
#include <stdbool.h>
#include <stdint.h>

const char *read_defect_data_format_to_str(uint8_t fmt);

/* READ DEFECT DATA 10 */

#define READ_DEFECT_DATA_10_MIN_LEN 4

static inline bool read_defect_data_10_is_plist_valid(uint8_t *data)
{
	return data[1] & 0x10;
}

static inline bool read_defect_data_10_is_glist_valid(uint8_t *data)
{
	return data[1] & 0x08;
}

static inline uint8_t read_defect_data_10_list_format(uint8_t *data)
{
	return data[1] & 0x07;
}

static inline uint16_t read_defect_data_10_len(uint8_t *data)
{
	return get_uint16(data, 2);
}

/* For READ DEFECT DATA 10 the user may ask for only the header and the data is never really transfered, allow for this seperation in the checks. */
static inline bool read_defect_data_10_hdr_is_valid(uint8_t *data, unsigned data_len)
{
	if (data_len < READ_DEFECT_DATA_10_MIN_LEN)
		return false;
	if ((read_defect_data_10_len(data) % 4) != 0)
		return false;
	return true;
}

static inline bool read_defect_data_10_is_valid(uint8_t *data, unsigned data_len)
{
	if (!read_defect_data_10_hdr_is_valid(data, data_len))
		return false;
	if ((unsigned)read_defect_data_10_len(data) + READ_DEFECT_DATA_10_MIN_LEN < data_len)
		return false;
	return true;
}

static inline uint8_t *read_defect_data_10_data(uint8_t *data)
{
	return data + READ_DEFECT_DATA_10_MIN_LEN;
}

/* READ DEFECT DATA 10 */

#define READ_DEFECT_DATA_12_MIN_LEN 8

static inline bool read_defect_data_12_is_plist_valid(uint8_t *data)
{
	return data[1] & 0x10;
}

static inline bool read_defect_data_12_is_glist_valid(uint8_t *data)
{
	return data[1] & 0x08;
}

static inline uint8_t read_defect_data_12_list_format(uint8_t *data)
{
	return data[1] & 0x07;
}

static inline uint32_t read_defect_data_12_len(uint8_t *data)
{
	return get_uint32(data, 4);
}

/* For READ DEFECT DATA 10 the user may ask for only the header and the data is never really transfered, allow for this seperation in the checks. */
static inline bool read_defect_data_12_hdr_is_valid(uint8_t *data, unsigned data_len)
{
	if (data_len < READ_DEFECT_DATA_12_MIN_LEN)
		return false;
	if ((read_defect_data_12_len(data) % 4) != 0)
		return false;
	return true;
}

static inline bool read_defect_data_12_is_valid(uint8_t *data, unsigned data_len)
{
	if (!read_defect_data_12_hdr_is_valid(data, data_len))
		return false;
	if (read_defect_data_12_len(data) + READ_DEFECT_DATA_12_MIN_LEN < data_len)
		return false;
	return true;
}

static inline uint8_t *read_defect_data_12_data(uint8_t *data)
{
	return data + READ_DEFECT_DATA_12_MIN_LEN;
}

/* Formats */

/* Short format */
#define FORMAT_ADDRESS_SHORT_LEN 4
static inline uint32_t format_address_short_lba(uint8_t *data)
{
	return get_uint32(data, 0);
}

/* Long format */
#define FORMAT_ADDRESS_LONG_LEN 8
static inline uint32_t format_address_long_lba(uint8_t *data)
{
	return get_uint64(data, 0);
}

/* Byte from index */
#define FORMAT_ADDRESS_BYTE_FROM_INDEX_LEN 8
static inline uint32_t format_address_byte_from_index_cylinder(uint8_t *data)
{
	return get_uint24(data, 0);
}

static inline uint8_t format_address_byte_from_index_head(uint8_t *data)
{
	return data[3];
}

static inline uint32_t format_address_byte_from_index_bytes(uint8_t *data)
{
	return get_uint32(data, 4);
}
#define FORMAT_ADDRESS_BYTE_FROM_INDEX_ALL_TRACK 0xFFFFFFFF

/* Physical sector format */
#define FORMAT_ADDRESS_PHYSICAL_LEN 8
static inline uint32_t format_address_physical_cylinder(uint8_t *data)
{
	return get_uint24(data, 0);
}

static inline uint8_t format_address_physical_head(uint8_t *data)
{
	return data[3];
}

static inline uint32_t format_address_physical_sector(uint8_t *data)
{
	return get_uint32(data, 4);
}
#define FORMAT_ADDRESS_PHYSICAL_ALL_TRACK 0xFFFFFFFF

static inline unsigned read_defect_data_fmt_len(address_desc_format_e fmt)
{
	switch (fmt) {
	case ADDRESS_FORMAT_SHORT: return FORMAT_ADDRESS_SHORT_LEN;
	case ADDRESS_FORMAT_LONG: return FORMAT_ADDRESS_LONG_LEN;
	case ADDRESS_FORMAT_INDEX_OFFSET: return FORMAT_ADDRESS_BYTE_FROM_INDEX_LEN;
	case ADDRESS_FORMAT_PHYSICAL: return FORMAT_ADDRESS_PHYSICAL_LEN;
	case ADDRESS_FORMAT_VENDOR: return 4;
	default: return 0;
	}
}

#endif
