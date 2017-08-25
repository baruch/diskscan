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

#ifndef LIBSCSICMD_RECEIVE_DIAGNOSTICS_H
#define LIBSCSICMD_RECEIVE_DIAGNOSTICS_H

#include "scsicmd_utils.h"
#include <stdint.h>
#include <memory.h>

#define RECV_DIAG_MIN_LEN 4

static inline uint8_t recv_diag_get_page_code(uint8_t *data)
{
	return data[0];
}

static inline uint8_t recv_diag_get_page_code_specific(uint8_t *data)
{
	return data[1];
}

static inline uint16_t recv_diag_get_len(uint8_t *data)
{
	return (data[2] << 8) | data[3];
}

static inline uint8_t *recv_diag_data(uint8_t *data)
{
	return data + RECV_DIAG_MIN_LEN;
}

static inline bool recv_diag_is_valid(uint8_t *data, unsigned data_len)
{
	if (data_len < RECV_DIAG_MIN_LEN)
		return false;
	if ((unsigned)recv_diag_get_len(data) + RECV_DIAG_MIN_LEN > data_len)
		return false;
	return true;
}

/* SES Page 1 Configuration */

static inline bool ses_config_is_valid(uint8_t *data, unsigned data_len)
{
	if (data_len < 8)
		return false;
	if (recv_diag_get_len(data) < 4)
		return false;
	return true;
}

static inline uint8_t ses_config_num_sub_enclosures(uint8_t *data)
{
	return data[1] + 1; // +1 for primary
}

static inline uint32_t ses_config_generation(uint8_t *data)
{
	return get_uint32(data, 4);
}

static inline uint8_t *ses_config_sub_enclosure(uint8_t *data)
{
	return data + 8;
}

/* Enclosure Descriptor */
static inline uint8_t ses_config_enclosure_descriptor_process_identifier(uint8_t *data)
{
	return (data[0] >> 4) & 0x7;
}

static inline uint8_t ses_config_enclosure_descriptor_num_processes(uint8_t *data)
{
	return data[0] & 0x7;
}

static inline uint8_t ses_config_enclosure_descriptor_subenclosure_identifier(uint8_t *data)
{
	return data[1];
}

static inline uint8_t ses_config_enclosure_descriptor_num_type_descriptors(uint8_t *data)
{
	return data[2];
}

static inline uint8_t ses_config_enclosure_descriptor_len(uint8_t *data)
{
	return data[3];
}

static inline uint64_t ses_config_enclosure_descriptor_logical_identifier(uint8_t *data)
{
	return get_uint64(data, 4);
}

static inline bool ses_config_enclosure_descriptor_is_valid(uint8_t *data, unsigned data_len)
{
	if (data_len < 12)
		return false;
	if (ses_config_enclosure_descriptor_len(data) < 36 ||
			ses_config_enclosure_descriptor_len(data) > 252 ||
			ses_config_enclosure_descriptor_len(data) > data_len)
	{
		return false;
	}
	return true;
}

static inline void _ses_str_cpy(uint8_t *src, unsigned src_len, char *s, unsigned slen)
{
	if (slen > src_len)
		slen = src_len;
	else
		slen--;

	memcpy(s, src, slen);
	s[slen] = 0;
}

static inline void ses_config_enclosure_descriptor_vendor_identifier(uint8_t *data, char *s, unsigned slen)
{
	_ses_str_cpy(data+12, 8, s, slen);
}

static inline void ses_config_enclosure_descriptor_product_identifier(uint8_t *data, char *s, unsigned slen)
{
	_ses_str_cpy(data+20, 16, s, slen);
}

static inline void ses_config_enclosure_descriptor_revision_level(uint8_t *data, char *s, unsigned slen)
{
	_ses_str_cpy(data+36, 4, s, slen);
}

static inline uint8_t *ses_config_enclosure_descriptor_vendor_info(uint8_t *data)
{
	return data + 40;
}

static inline uint8_t ses_config_enclosure_descriptor_vendor_len(uint8_t *data)
{
	return ses_config_enclosure_descriptor_len(data) + 4 - 40;
}

#endif
