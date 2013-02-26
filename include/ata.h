#ifndef LIBSCSICMD_ATA_H
#define LIBSCSICMD_ATA_H

#include <stdint.h>
#include <stdbool.h>

enum ata_passthrough_len_spec {
	ATA_PT_LEN_SPEC_NONE         = 0,
	ATA_PT_LEN_SPEC_FEATURES     = 1,
	ATA_PT_LEN_SPEC_SECTOR_COUNT = 2,
	ATA_PT_LEN_SPEC_TPSIU        = 3,
};

typedef uint16_t ata_word_t;
typedef uint32_t ata_longword_t;

static inline unsigned char ata_passthrough_flags_2(int offline, int ck_cond, int direction_in, int transfer_block, enum ata_passthrough_len_spec len_spec)
{
	return ((offline & 3) << 6) | (ck_cond&1) | ((direction_in & 1) << 3) | ((transfer_block & 1) << 2) | (len_spec & 3);
}

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

#endif
