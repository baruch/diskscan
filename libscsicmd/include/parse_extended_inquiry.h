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

#ifndef LIBSCSICMD_EXTENDED_INQUIRY_H
#define LIBSCSICMD_EXTENDED_INQUIRY_H

#include <stdint.h>

#define EVPD_MIN_LEN 4

static inline uint8_t evpd_peripheral_qualifier(uint8_t *data)
{
	return data[0] >> 5;
}

static inline uint8_t evpd_peripheral_device_type(uint8_t *data)
{
	return data[0] & 0x1F;
}

static inline uint8_t evpd_page_code(uint8_t *data)
{
	return data[1];
}

static inline uint16_t evpd_page_len(uint8_t *data)
{
	return (data[2] << 8) | data[3];
}

static inline uint8_t *evpd_page_data(uint8_t *data)
{
	return data + EVPD_MIN_LEN;
}

static inline bool evpd_is_ascii_page(uint8_t page_code)
{
	return 0x01 <= page_code && page_code <= 0x7F;
}

/* This should be called with the body of the EVPD data and not the full (i.e. with the output of evpd_page_data()) */
static inline uint16_t evpd_ascii_len(uint8_t *evpd_body)
{
	return (evpd_body[0] << 8) | evpd_body[1];
}

static inline uint8_t *evpd_ascii_data(uint8_t *evpd_body)
{
	return evpd_body + 2;
}

static inline uint8_t *evpd_ascii_post_data(uint8_t *evpd_body)
{
	return evpd_body + 2 + evpd_ascii_len(evpd_body);
}

static inline unsigned evpd_ascii_post_data_len(uint8_t *evpd_body, unsigned full_data_len)
{
	return full_data_len - EVPD_MIN_LEN - 2 - evpd_ascii_len(evpd_body);
}

static inline bool evpd_is_valid(uint8_t *data, unsigned data_len)
{
	if (data_len < EVPD_MIN_LEN)
		return false;

	if (evpd_page_len(data) > data_len)
		return false;

	return true;
}

#endif
