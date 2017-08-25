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
#include "scsicmd_utils.h"

#include <stdio.h>
#include <string.h>

bool parse_read_capacity_10(unsigned char *buf, unsigned buf_len, uint32_t *max_lba, uint32_t *block_size)
{
	if (buf_len < 8)
		return false;

	if (max_lba)
		*max_lba = get_uint32(buf, 0);
	if (block_size)
		*block_size = get_uint32(buf, 4);
	return true;
}

bool parse_read_capacity_16(unsigned char *buf, unsigned buf_len, uint64_t *max_lba, uint32_t *block_size, bool *prot_enable,
		unsigned *p_type, unsigned *p_i_exponent, unsigned *logical_blocks_per_physical_block_exponent,
		bool *thin_provisioning_enabled, bool *thin_provisioning_zero, unsigned *lowest_aligned_lba)
{
	if (buf_len < 16)
		return false;

	if (max_lba)
		*max_lba = get_uint64(buf, 0);
	if (block_size)
		*block_size = get_uint32(buf, 8);
	if (prot_enable)
		*prot_enable = buf[12] & 1;
	if (p_type)
		*p_type = (buf[12] & 0xe) >> 1;
	if (p_i_exponent)
		*p_i_exponent = (buf[13] & 0xf0) >> 4;
	if (logical_blocks_per_physical_block_exponent)
		*logical_blocks_per_physical_block_exponent = buf[13] & 0xf;
	if (thin_provisioning_enabled)
		*thin_provisioning_enabled = buf[14] & 0x80;
	if (thin_provisioning_zero)
		*thin_provisioning_zero = buf[14] & 0x40;
	if (*lowest_aligned_lba)
		*lowest_aligned_lba = (buf[14] & 0x3f) << 8 | buf[15];
	return true;
}
