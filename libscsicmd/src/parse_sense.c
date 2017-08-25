#include "scsicmd.h"
#include "scsicmd_utils.h"

#include <memory.h>

static void parse_sense_key_specific(unsigned char *sks, sense_info_t *info)
{
        info->sense_key_specific_valid = sks[0] & 0x80;
        if (info->sense_key_specific_valid)
        {
                uint32_t sense_key_specific = get_uint24(sks, 0) & 0x007FFFFF;
                switch (info->sense_key) {
                        case SENSE_KEY_ILLEGAL_REQUEST:
                                info->sense_key_specific.illegal_request.command_error = sense_key_specific & 0x400000;
                                info->sense_key_specific.illegal_request.bit_pointer_valid = sense_key_specific & 0x080000;
                                info->sense_key_specific.illegal_request.bit_pointer = (sense_key_specific & 0x070000) >> 16;
                                info->sense_key_specific.illegal_request.field_pointer = sense_key_specific & 0xFFFF;
                                break;
                        case SENSE_KEY_HARDWARE_ERROR:
                        case SENSE_KEY_MEDIUM_ERROR:
                        case SENSE_KEY_RECOVERED_ERROR:
                                info->sense_key_specific.hardware_medium_recovered_error.actual_retry_count = sense_key_specific & 0xFFFF;
                                break;
                        case SENSE_KEY_NOT_READY:
                        case SENSE_KEY_NO_SENSE:
                                info->sense_key_specific.not_ready.progress = ((double)(sense_key_specific & 0xFFFF))/65536.0;
                                break;
                        case SENSE_KEY_COPY_ABORTED:
                                info->sense_key_specific.copy_aborted.segment_descriptor = sense_key_specific & 0x200000;
                                info->sense_key_specific.copy_aborted.bit_pointer_valid = sense_key_specific & 0x080000;
                                info->sense_key_specific.copy_aborted.bit_pointer = (sense_key_specific & 0x070000) >> 16;
                                info->sense_key_specific.copy_aborted.field_pointer = sense_key_specific & 0xFFFF;
                                break;
                        case SENSE_KEY_UNIT_ATTENTION:
                                info->sense_key_specific.unit_attention.overflow = sense_key_specific & 0x010000;
                                break;
                        default:
                                info->sense_key_specific_valid = false;
                                break;
                }
        }
}

static bool parse_sense_fixed(unsigned char *sense, int sense_len, sense_info_t *info)
{
        if (sense_len < 18)
                return false;

        info->information_valid = sense[0] & 0x80;
        if (info->information_valid)
                info->information = get_uint32(sense, 3);

        info->incorrect_len_indicator = sense[2] & 0x20;
        info->sense_key = sense[2] & 0xF;
        info->asc = sense[12];
        info->ascq = sense[13];

        info->cmd_specific_valid = true;
        info->cmd_specific = get_uint32(sense, 8);

        info->fru_code_valid = true;
        info->fru_code = sense[14];

        parse_sense_key_specific(sense + 15, info);

        if (sense_len >= 21)
            info->vendor_unique_error = get_uint16(sense, 20);

        //uint8_t additional_sense_len = sense[7];

        return true;
}

static bool parse_sense_descriptor(unsigned char *sense, unsigned sense_len, sense_info_t *info)
{
        if (sense_len < 8)
                return false;

        info->sense_key = sense[1] & 0xF;
        info->asc = sense[2];
        info->ascq = sense[3];

        unsigned additional_sense_length = sense[7];
        if (sense_len > additional_sense_length + 8)
                sense_len = additional_sense_length + 8;

        unsigned idx;
        unsigned desc_len;

        for (idx = 8; idx < sense_len; idx += desc_len+2) {
                uint8_t desc_type = sense[idx];
                desc_len = sense[idx+1];

                if (idx + desc_len + 2 > sense_len)
                        break;

                switch (desc_type) {
                        case 0x00: // Information
                                if (desc_len == 0x0A) {
                                        info->information_valid = sense[idx+2] & 0x80;
                                        info->information = get_uint64(sense, idx+4);
                                }
                                break;
                        case 0x01: // Command specific information
                                if (desc_len == 0x0A) {
                                        info->cmd_specific_valid = true;
                                        info->cmd_specific = get_uint64(sense, idx+4);
                                }
                                break;
                        case 0x02: // Sense key specific
                                if (desc_len == 0x06) {
                                        parse_sense_key_specific(sense+4, info);
                                }
                                break;
                        case 0x03: // FRU
                                if (desc_len == 0x02) {
                                        info->fru_code_valid = true;
                                        info->fru_code = sense[idx+3];
                                }
                                break;
                        case 0x04: // Stream commands
                                break;
                        case 0x05: // Block commands
                                if (desc_len == 0x02) {
                                        info->incorrect_len_indicator = sense[idx+3] & 0x20;
                                }
                                break;
                        case 0x06: // OSD object identification
                                break;
                        case 0x07: // OSD response integrity check value
                                break;
                        case 0x08: // OSD attribute identification
                                break;
                        case 0x09: // ATA Status Return
                                if (desc_len == 0x0C) {
                                        info->ata_status_valid = true;
                                        info->ata_status.extend = sense[idx+2] & 1;
                                        info->ata_status.error = sense[idx+3];
                                        if (info->ata_status.extend) {
                                                info->ata_status.sector_count = (sense[idx+4] << 8) | sense[idx+5];
                                                info->ata_status.lba =
													((uint64_t)sense[idx+7]) | 
													((uint64_t)sense[idx+6]<<8) |
													((uint64_t)sense[idx+9]<<16) |
													((uint64_t)sense[idx+8]<<24) |
													((uint64_t)sense[idx+11]<<32) |
													((uint64_t)sense[idx+10]<<40);
                                        } else {
                                                info->ata_status.sector_count = sense[idx+4];
                                                info->ata_status.lba = sense[idx+7] | (sense[idx+9]<<8) | (sense[idx+11]<<16);
                                        }
                                        info->ata_status.device = sense[idx+12];
                                        info->ata_status.status = sense[idx+13];
                                }
                                break;
                        case 0x0A: // Progress indication
                                break;
                        case 0x0B: // User data segment referral
                                break;
                        case 0x80: // Vendor Unique Unit Error
                                if (desc_len == 0x02) {
                                    info->vendor_unique_error = get_uint16(sense, idx+2);
                                }
                                break;
                }
        }

        return true;
}

bool scsi_parse_sense(unsigned char *sense, int sense_len, sense_info_t *info)
{
        uint8_t response_code = sense[0] & 0x7F;

        memset(info, 0, sizeof(*info));

        if (response_code != 0x70 && response_code != 0x71 && response_code != 0x72 && response_code != 0x73)
                return false;

        if (response_code == 0x70 || response_code == 0x71)
                info->is_fixed = true;
        if (response_code == 0x70 || response_code == 0x72)
                info->is_current = true;

        if (info->is_fixed)
                return parse_sense_fixed(sense, sense_len, info);
        else
                return parse_sense_descriptor(sense, sense_len, info);
}
