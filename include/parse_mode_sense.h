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

#ifndef LIBSCSICMD_MODE_SENSE_H
#define LIBSCSICMD_MODE_SENSE_H

#include "scsicmd_utils.h"

/* Mode parameter header for the MODE SENSE 6 */
#define MODE_SENSE_6_MIN_LEN 4u

static inline unsigned mode_sense_6_data_len(uint8_t *data)
{
	return data[0];
}

static inline uint8_t mode_sense_6_medium_type(uint8_t *data)
{
	return data[1];
}

static inline uint8_t mode_sense_6_device_specific_param(uint8_t *data)
{
	return data[2];
}

static inline uint8_t mode_sense_6_block_descriptor_length(uint8_t *data)
{
	return data[3];
}

static inline uint8_t *mode_sense_6_block_descriptor_data(uint8_t *data)
{
	return data + MODE_SENSE_6_MIN_LEN;
}

static inline uint8_t *mode_sense_6_mode_data(uint8_t *data)
{
	return data + MODE_SENSE_6_MIN_LEN + mode_sense_6_block_descriptor_length(data);
}

static inline unsigned mode_sense_6_expected_length(uint8_t *data)
{
	return 1 + mode_sense_6_data_len(data); // Add the first byte that is not part of the mode data length
}

static inline unsigned mode_sense_6_mode_data_len(uint8_t *data)
{
	return mode_sense_6_expected_length(data) - MODE_SENSE_6_MIN_LEN - mode_sense_6_block_descriptor_length(data);
}

static inline bool mode_sense_6_is_valid_header(uint8_t *data, unsigned data_len)
{
	if (mode_sense_6_data_len(data) < MODE_SENSE_6_MIN_LEN-1)
		return false;
	if (data_len < (unsigned)(mode_sense_6_data_len(data)) + 1)
		return false;
	if (mode_sense_6_block_descriptor_length(data) != 0 &&
		mode_sense_6_block_descriptor_length(data) != 8)
	{
		return false;
	}
	if (mode_sense_6_data_len(data) < mode_sense_6_block_descriptor_length(data))
		return false;
	return true;
}

/* Mode parameter header for the MODE SENSE 10 */
#define MODE_SENSE_10_MIN_LEN 8u

static inline unsigned mode_sense_10_data_len(uint8_t *data)
{
	return get_uint16(data, 0);
}

static inline uint8_t mode_sense_10_medium_type(uint8_t *data)
{
	return data[2];
}

static inline uint8_t mode_sense_10_device_specific_param(uint8_t *data)
{
	return data[3];
}

static inline bool mode_sense_10_long_lba(uint8_t *data)
{
	return data[4] & 1;
}

/* SPC4 says length is times 8 without long lba and times 16 with it but it seems the value is taken verbatim */
static inline unsigned mode_sense_10_block_descriptor_length(uint8_t *data)
{
	return get_uint16(data, 6);
}

static inline uint8_t *mode_sense_10_block_descriptor_data(uint8_t *data)
{
	if (mode_sense_10_block_descriptor_length(data) > 0)
		return data + MODE_SENSE_10_MIN_LEN;
	else
		return NULL;
}

static inline uint8_t *mode_sense_10_mode_data(uint8_t *data)
{
	return data + MODE_SENSE_10_MIN_LEN + mode_sense_10_block_descriptor_length(data);
}

static inline unsigned mode_sense_10_expected_length(uint8_t *data)
{
	return 2 + mode_sense_10_data_len(data); // Add the first two bytes that are not part of the mode data length
}

static inline unsigned mode_sense_10_mode_data_len(uint8_t *data)
{
	return mode_sense_10_expected_length(data) - MODE_SENSE_10_MIN_LEN - mode_sense_10_block_descriptor_length(data);
}

static inline bool mode_sense_10_is_valid_header(uint8_t *data, unsigned data_len)
{
	if (mode_sense_10_data_len(data) < MODE_SENSE_10_MIN_LEN-2)
		return false;
	if (data_len < (unsigned)(mode_sense_10_data_len(data)) + 2)
		return false;
	if (mode_sense_10_block_descriptor_length(data) != 0 &&
		mode_sense_10_block_descriptor_length(data) != 8)
	{
		return false;
	}
	return true;
}

/* Regular block descriptor (as opposed to long) */
#define BLOCK_DESCRIPTOR_LENGTH 8
#define BLOCK_DESCRIPTOR_NUM_BLOCKS_OVERFLOW 0xFFFFFF

static inline uint8_t block_descriptor_density_code(uint8_t *data)
{
	return data[0];
}

static inline uint32_t block_descriptor_num_blocks(uint8_t *data)
{
	return get_uint24(data, 1);
}

static inline uint32_t block_descriptor_block_length(uint8_t *data)
{
	return get_uint24(data, 5);
}

/* Mode Sense page data */
static inline uint8_t mode_sense_data_page_code(uint8_t *data)
{
	return data[0] & 0x3F;
}

static inline bool mode_sense_data_subpage_format(uint8_t *data)
{
	return data[0] & 0x40;
}

static inline bool mode_sense_data_parameter_saveable(uint8_t *data)
{
	return data[0] & 0x80;
}

/* Caller is required to know if this is a subpage format page or not */
static inline uint8_t mode_sense_data_subpage_code(uint8_t *data)
{
	return data[1];
}

static inline unsigned mode_sense_data_page_len(uint8_t *data)
{
	return mode_sense_data_subpage_format(data) ? 3 + (unsigned)(get_uint16(data, 2)) : 2 + (unsigned)(data[1]);
}

static inline unsigned mode_sense_data_param_len(uint8_t *data)
{
	return mode_sense_data_subpage_format(data) ? get_uint16(data, 2) : data[1];
}

static inline uint8_t *mode_sense_data_param(uint8_t *data)
{
	return mode_sense_data_subpage_format(data) ? data + 4 : data + 2;
}

static inline bool mode_sense_data_param_is_valid(uint8_t *data, unsigned data_len)
{
	if (data_len < 2)
		return false;
	if (mode_sense_data_page_code(data) == 0)
		return false;
	if (mode_sense_data_subpage_format(data) && data_len < 4)
		return false;
	if (data_len < mode_sense_data_page_len(data))
		return false;
	return true;
}


#define for_all_mode_sense_pages(data, data_len, mode_data, mode_data_len, page, remaining_len) \
	for (remaining_len = mode_data - data + safe_len(data, data_len, mode_data, mode_data_len), page = mode_data; \
		 remaining_len >= 3 && mode_sense_data_param_is_valid(page, remaining_len); \
		 remaining_len += mode_sense_data_param_len(page), page += mode_sense_data_page_len(page))

#define for_all_mode_sense_6_pages(data, data_len, page, remaining_len) \
	for_all_mode_sense_pages(data, data_len, mode_sense_6_mode_data(data), mode_sense_6_mode_data_len(data), page, remaining_len)

#define for_all_mode_sense_10_pages(data, data_len, page, remaining_len) \
	for_all_mode_sense_pages(data, data_len, mode_sense_10_mode_data(data), mode_sense_10_mode_data_len(data), page, remaining_len)

#endif
