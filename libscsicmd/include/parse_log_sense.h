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

#ifndef LIBSCSICMD_LOG_SENSE_H
#define LIBSCSICMD_LOG_SENSE_H

#include "scsicmd_utils.h"
#include <stdint.h>
#include <stdbool.h>

/* Log Sense Header decode */

#define LOG_SENSE_MIN_LEN 4

static inline uint8_t log_sense_page_code(uint8_t *data)
{
	return data[0] & 0x3F;
}

static inline bool log_sense_subpage_format(uint8_t *data)
{
	return data[0] & 0x40;
}

static inline bool log_sense_data_saved(uint8_t *data)
{
	return data[0] & 0x80;
}

static inline uint8_t log_sense_subpage_code(uint8_t *data)
{
	return data[1];
}

static inline unsigned log_sense_data_len(uint8_t *data)
{
	return get_uint16(data, 2);
}

static inline uint8_t *log_sense_data(uint8_t *data)
{
	return data + LOG_SENSE_MIN_LEN;
}

static inline uint8_t *log_sense_data_end(uint8_t *data, unsigned data_len)
{
	return data + safe_len(data, data_len, log_sense_data(data), log_sense_data_len(data));
}

static inline bool log_sense_is_valid(uint8_t *data, unsigned data_len)
{
	if (data_len < LOG_SENSE_MIN_LEN)
		return false;
	if (!log_sense_subpage_format(data) && log_sense_subpage_code(data) != 0)
		return false;
	if (log_sense_data_len(data) + LOG_SENSE_MIN_LEN < data_len)
		return false;
	return true;
}

/* Log Sense Parameter decode */
#define LOG_SENSE_MIN_PARAM_LEN 4

#define LOG_PARAM_FLAG_DU 0x80
#define LOG_PARAM_FLAG_TSD 0x20
#define LOG_PARAM_FLAG_ETC 0x10
#define LOG_PARAM_FLAG_TMC_MASK 0x0C
#define LOG_PARAM_FLAG_FMT_MASK 0x03

#define LOG_PARAM_TMC_EVERY_UPDATE 0
#define LOG_PARAM_TMC_EQUAL 1
#define LOG_PARAM_TMC_NOT_EQUAL 2
#define LOG_PARAM_TMC_GREATER 3

#define LOG_PARAM_FMT_COUNTER_STOP 0
#define LOG_PARAM_FMT_ASCII 1
#define LOG_PARAM_FMT_COUNTER_ROLLOVER 2
#define LOG_PARAM_FMT_BINARY 2

static inline uint16_t log_sense_param_code(uint8_t *param)
{
	return get_uint16(param, 0);
}

inline static uint8_t log_sense_param_flags(uint8_t *param)
{
	return param[2];
}

inline static uint8_t log_sense_param_tmc(uint8_t *param)
{
	return (log_sense_param_flags(param) & LOG_PARAM_FLAG_TMC_MASK) >> 2;
}

inline static uint8_t log_sense_param_fmt(uint8_t *param)
{
	return log_sense_param_flags(param) & LOG_PARAM_FLAG_FMT_MASK;
}

static inline unsigned log_sense_param_len(uint8_t *param)
{
	return param[3];
}

static inline uint8_t *log_sense_param_data(uint8_t *param)
{
	return param + LOG_SENSE_MIN_PARAM_LEN;
}

static inline bool log_sense_param_is_valid(uint8_t *data, unsigned data_len, uint8_t *param)
{
	if (param < data)
		return false;

	const unsigned param_offset = param - data;
	if (param_offset > data_len)
		return false;

	if (param_offset + LOG_SENSE_MIN_PARAM_LEN > data_len)
		return false;

	if (param_offset + LOG_SENSE_MIN_PARAM_LEN + log_sense_param_len(param) > data_len)
		return false;

	return true;
}

#define for_all_log_sense_params(data, data_len, param) \
	for (param = log_sense_data(data); \
		 log_sense_param_is_valid(data, data_len, param); \
		 param = param + LOG_SENSE_MIN_PARAM_LEN + log_sense_param_len(param))

#define for_all_log_sense_pg_0_supported_pages(data, data_len, supported_page) \
	uint8_t *__tmp; \
	for (__tmp = log_sense_data(data), supported_page = __tmp[0]; __tmp < log_sense_data_end(data, data_len); __tmp++, supported_page = __tmp[0])

#define for_all_log_sense_pg_0_supported_subpages(data, data_len, supported_page, supported_subpage) \
	uint8_t *__tmp; \
	for (__tmp = log_sense_data(data), supported_page = __tmp[0], supported_subpage = __tmp[1]; __tmp + 1 < log_sense_data_end(data, data_len); __tmp+=2, supported_page = __tmp[0], supported_subpage = __tmp[1])

bool log_sense_page_informational_exceptions(uint8_t *page, unsigned page_len, uint8_t *asc, uint8_t *ascq, uint8_t *temperature);

#endif
