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

#ifndef LIBSCSICMD_ATA_H
#define LIBSCSICMD_ATA_H

#include <stdint.h>
#include <stdbool.h>

typedef uint16_t ata_word_t;
typedef uint32_t ata_longword_t;

typedef enum passthrough_protocol_e {
	PT_PROTO_HARDWARE_RESET = 0,
	PT_PROTO_SOFTWARE_RESET = 1,
	PT_PROTO_NON_DATA = 3,
	PT_PROTO_PIO_DATA_IN = 4,
	PT_PROTO_PIO_DATA_OUT = 5,
	PT_PROTO_DMA = 6,
	PT_PROTO_DMA_QUEUED = 7,
	PT_PROTO_EXECUTE_DEVICE_DIAGNOSTIC = 8,
	PT_PROTO_DEVICE_RESET = 9,
	PT_PROTO_UDMA_DATA_IN = 10,
	PT_PROTO_UDMA_DATA_OUT = 11,
	PT_PROTO_FPDMA = 12,
	PT_PROTO_RETURN_RESPONSE_INFO = 15,
} passthrough_protocol_e;

typedef enum ata_passthrough_len_spec_e {
	ATA_PT_LEN_SPEC_NONE         = 0,
	ATA_PT_LEN_SPEC_FEATURES     = 1,
	ATA_PT_LEN_SPEC_SECTOR_COUNT = 2,
	ATA_PT_LEN_SPEC_TPSIU        = 3,
} ata_passthrough_len_spec_e;

typedef struct ata_status_t {
	uint8_t extend;
	uint8_t error;
	uint8_t device;
	uint8_t status;
	uint16_t sector_count;
	uint64_t lba;
} ata_status_t;

static inline ata_word_t ata_get_word(const char *buf, int word)
{
	return (uint16_t)(buf[word*2+1])<<8 | buf[word*2];
}

static inline uint16_t ata_get_bits(const char *buf, int word, int start_bit, int end_bit)
{
	uint16_t val = ata_get_word(buf, word);
	uint16_t shift = start_bit;
	uint16_t mask = 0;

	switch (end_bit - start_bit + 1) {
		case 1: mask = 1; break;
		case 2: mask = 3; break;
		case 3: mask = 7; break;
		case 4: mask = 0xF; break;
		case 5: mask = 0x1F; break;
		case 6: mask = 0x3F; break;
		case 7: mask = 0x7F; break;
		case 8: mask = 0xFF; break;
		case 9: mask = 0x1FF; break;
		case 10: mask = 0x3FF; break;
		case 11: mask = 0x7FF; break;
		case 12: mask = 0xFFF; break;
		case 13: mask = 0x1FFF; break;
		case 14: mask = 0x3FFF; break;
		case 15: mask = 0x7FFF; break;
		case 16: mask = 0xFFFF; break;
	}

	return (val >> shift) & mask;
}

static inline bool ata_get_bit(char *buf, int word, int bit)
{
	return ata_get_bits(buf, word, bit, bit);
}

static inline char *ata_get_string(const char *buf, int word_start, int word_end, char *str)
{
	int word;
	int i;

	/* Need to reverse the characters in the string as per "ATA string conventions" of ATA/ATAPI command set */
	for (i = 0, word = word_start; word <= word_end; word++) {
		str[i++] = buf[word*2+1];
		str[i++] = buf[word*2];
	}
	str[i] = 0;

	return str;
}

static inline ata_longword_t ata_get_longword(const char *buf, int start_word)
{
	ata_longword_t high = ata_get_word(buf, start_word+1);
	ata_longword_t low = ata_get_word(buf, start_word);
	ata_longword_t longword = high << 16 | low;
	return longword;
}

bool ata_inquiry_checksum_verify(const char *buf, int buf_len);

static inline unsigned char ata_passthrough_flags_2(int offline, int ck_cond, int direction_in, int transfer_block, ata_passthrough_len_spec_e len_spec)
{
	return ((offline & 3) << 6) | ((ck_cond&1)<<5) | ((direction_in & 1) << 3) | ((transfer_block & 1) << 2) | (len_spec & 3);
}

static inline int cdb_ata_passthrough_12(unsigned char *cdb, uint8_t command, uint8_t feature, uint32_t lba, uint8_t sector_count, passthrough_protocol_e protocol, bool dir_in, int ck_cond)
{
	cdb[0] = 0xA1;
	cdb[1] = protocol<<1;
	cdb[2] = ata_passthrough_flags_2(0, ck_cond, dir_in, 1, ATA_PT_LEN_SPEC_SECTOR_COUNT);
	cdb[3] = feature;
	cdb[4] = sector_count;
	cdb[5] = lba & 0xFF;
	cdb[6] = (lba >> 8) & 0xFF;
	cdb[7] = (lba >> 16) & 0xFF;
	cdb[8] = (lba >> 24) & 0x0F; // 28-bit addressing
	cdb[9] = command;

	cdb[10] = cdb[11] = 0;

	return 12;
}

static inline int cdb_ata_identify(unsigned char *cdb)
{
	return cdb_ata_passthrough_12(cdb, 0xEC, 0x00, 0x0, 1, PT_PROTO_PIO_DATA_IN, true, 0);
}

static inline int cdb_ata_smart_return_status(unsigned char *cdb)
{
	return cdb_ata_passthrough_12(cdb, 0xB0, 0xDA, 0xC24F<<8, 1, PT_PROTO_DMA, true, 1);
}

bool ata_status_from_scsi_sense(unsigned char *sense, int sense_len, ata_status_t *status);

static inline bool ata_smart_return_status_result(unsigned char *sense, int sense_len, bool *smart_ok)
{
	ata_status_t status;
	if (!ata_status_from_scsi_sense(sense, sense_len, &status))
		return false;

	if (status.lba >> 8 == 0xC24F)
		*smart_ok = true;
	else if (status.lba >> 8 == 0x2CF4)
		*smart_ok = false;
	else
		return false;

	return true;
}

#endif
