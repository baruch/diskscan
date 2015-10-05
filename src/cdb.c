/* Copyright 2013 Baruch Even
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

#include "scsicmd.h"

#include <memory.h>

static inline void set_uint16(unsigned char *cdb, int start, uint16_t val)
{
        cdb[start] = (val >> 8) & 0xFF;
        cdb[start+1] = val & 0xFF;
}

static inline void set_uint32(unsigned char *cdb, int start, uint32_t val)
{
        cdb[start]   = (val >> 24) & 0xFF;
        cdb[start+1] = (val >> 16) & 0xFF;
        cdb[start+2] = (val >> 8) & 0xFF;
        cdb[start+3] = val & 0xFF;
}

static inline void set_uint64(unsigned char *cdb, int start, uint64_t val)
{
        cdb[start]   = (val >> 56) & 0xFF;
        cdb[start+1] = (val >> 48) & 0xFF;
        cdb[start+2] = (val >> 40) & 0xFF;
        cdb[start+3] = (val >> 32) & 0xFF;
        cdb[start+4] = (val >> 24) & 0xFF;
        cdb[start+5] = (val >> 16) & 0xFF;
        cdb[start+6] = (val >>  8) & 0xFF;
        cdb[start+7] = val & 0xFF;
}

int cdb_tur(unsigned char *cdb)
{
	const int TUR_LEN = 6;
	memset(cdb, 0, TUR_LEN);
	return TUR_LEN;
}

int cdb_inquiry(unsigned char *cdb, bool evpd, char page_code, uint16_t alloc_len)
{
	const int INQUIRY_LEN = 6;

	cdb[0] = 0x12;
	cdb[1] = evpd ? 1 : 0;
	cdb[2] = page_code;
	set_uint16(cdb, 3, alloc_len);
	cdb[5] = 0;

	return INQUIRY_LEN;
}

int cdb_read_capacity_10(unsigned char *cdb)
{
	const int LEN = 10;
	cdb[0] = 0x25;
	memset(cdb+1, 0, LEN-1);
	return LEN;
}

int cdb_read_capacity_16(unsigned char *cdb, uint32_t alloc_len)
{
	const int LEN = 16;
	cdb[0] = 0x9E;
	cdb[1] = 0x10;
	memset(cdb+2, 0, LEN-2);
	set_uint32(cdb, 10, alloc_len);
	return LEN;
}

int cdb_read_10(unsigned char *cdb, bool fua, uint64_t lba, uint16_t transfer_length_blocks)
{
	const int LEN = 10;
	cdb[0] = 0x28;
	cdb[1] = fua << 3;
	set_uint32(cdb, 2, lba);
	cdb[6] = 0;
	set_uint16(cdb, 7, transfer_length_blocks);
	cdb[9] = 0;
	return LEN;
}

int cdb_write_10(unsigned char *cdb, bool fua, uint64_t lba, uint16_t transfer_length_blocks)
{
	const int LEN = 10;
	cdb[0] = 0x2A;
	cdb[1] = fua << 3;
	set_uint32(cdb, 2, lba);
	cdb[6] = 0;
	set_uint16(cdb, 7, transfer_length_blocks);
	cdb[9] = 0;
	return LEN;
}

int cdb_read_16(unsigned char *cdb, bool fua, bool fua_nv, bool dpo, uint64_t lba, uint32_t transfer_length_blocks)
{
	const int LEN = 16;
	cdb[0] = 0x88;
	cdb[1] = (dpo<<4) | (fua<<3) | (fua_nv<<1);
	set_uint64(cdb, 2, lba);
	set_uint32(cdb, 10, transfer_length_blocks);
	cdb[14] = 0;
	cdb[15] = 0;
	return LEN;
}

int cdb_write_16(unsigned char *cdb, bool dpo, bool fua, bool fua_nv, uint64_t lba, uint32_t transfer_length_blocks)
{
	const int LEN = 16;
	cdb[0] = 0x8A;
	cdb[1] = (dpo<<4) | (fua<<3) | (fua_nv<<1);
	set_uint64(cdb, 2, lba);
	set_uint32(cdb, 10, transfer_length_blocks);
	cdb[14] = 0;
	cdb[15] = 0;
	return LEN;
}

int cdb_log_sense(unsigned char *cdb, uint8_t page_code, uint8_t subpage_code, uint16_t alloc_len)
{
	const int LEN = 10;
	memset(cdb, 0, LEN);
	cdb[0] = 0x4D;
	cdb[2] = (1 << 6) | (page_code & 0x3F); // The pc is always set to 1 for Cumulative values
	cdb[3] = subpage_code;
	set_uint16(cdb, 7, alloc_len);
	return LEN;
}
