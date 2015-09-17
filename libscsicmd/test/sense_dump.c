#include "sense_dump.h"
#include "scsicmd.h"
#include <stdio.h>
#include <inttypes.h>

void response_dump(unsigned char *buf, int buf_len)
{
	int i;
	for (i = 0; i < buf_len; i++) {
		if (i % 16 == 0)
			printf("\n%02x  ", i);
		printf("%02x ", buf[i]);
	}
	printf("\n");
}

static bool print_bool(const char *name, bool val)
{
        printf("%s: %s\n", name, val ? "yes" : "no");
        return val;
}

static bool print_code(const char *name, bool val, const char *true_code, const char *false_code)
{
        printf("%s: %s\n", name, val ? true_code : false_code);
        return val;
}

static void sense_dump_sense_info(sense_info_t *si)
{
        print_code("Type", si->is_fixed, "Fixed", "Descriptor");
        print_code("Time", si->is_current, "Current", "Deferred");
        printf("Code: %x/%02x/%02x\n", si->sense_key, si->asc, si->ascq);
        printf("Code: %s/%s\n", sense_key_to_name(si->sense_key), asc_num_to_name(si->asc, si->ascq));
        printf("Vendor Unique: 0x%04x\n", si->vendor_unique_error);
        if (si->information_valid)
                printf("Information: %"PRIx64"\n", si->information);
        if (si->cmd_specific_valid)
                printf("Command specific: %"PRIx64"\n", si->cmd_specific);
        if (si->sense_key_specific_valid) {

                switch (si->sense_key) {
                        case SENSE_KEY_ILLEGAL_REQUEST:
                                print_code("Illegal Request Type", si->sense_key_specific.illegal_request.command_error, "CDB", "Data");
                                if (si->sense_key_specific.illegal_request.bit_pointer_valid)
                                        printf("Illegal Request Bit: %d\n", si->sense_key_specific.illegal_request.bit_pointer);
                                printf("Illegal Request Field: %d\n", si->sense_key_specific.illegal_request.field_pointer);
                                break;
                        case SENSE_KEY_HARDWARE_ERROR:
                        case SENSE_KEY_MEDIUM_ERROR:
                        case SENSE_KEY_RECOVERED_ERROR:
                                printf("Actual Retry Count: %d\n", si->sense_key_specific.hardware_medium_recovered_error.actual_retry_count);
                                break;
                        case SENSE_KEY_NOT_READY:
                        case SENSE_KEY_NO_SENSE:
                                printf("Progress: %g%%\n", si->sense_key_specific.not_ready.progress);
                                break;
                        case SENSE_KEY_COPY_ABORTED:
                                print_code("Copy Aborted Type", si->sense_key_specific.copy_aborted.segment_descriptor, "Segment", "Descriptor");
                                if (si->sense_key_specific.copy_aborted.bit_pointer_valid)
                                        printf("Copy Aborted Bit: %d\n", si->sense_key_specific.copy_aborted.bit_pointer);
                                printf("Copy Aborted Field: %d\n", si->sense_key_specific.copy_aborted.field_pointer);
                                break;
                        case SENSE_KEY_UNIT_ATTENTION:
                                print_bool("Unit Attention Overflow", si->sense_key_specific.unit_attention.overflow);
                                break;
                }
        }

        if (si->fru_code_valid)
                printf("FRU Code: %02x\n", si->fru_code);

        if (si->ata_status_valid) {
                printf("ATA Status valid\n"); /*TODO: more details */
                printf("ATA\n");
                printf("    Extend: %02x\n", si->ata_status.extend);
                printf("    Error: %02x\n", si->ata_status.error);
                printf("    Device: %02x\n", si->ata_status.device);
                printf("    Status: %02x\n", si->ata_status.status);
                printf("    Sector Count: %u\n", si->ata_status.sector_count);
                printf("    LBA: %"PRIu64" / %"PRIx64"\n", si->ata_status.lba, si->ata_status.lba);
        }

        print_bool("Incorrect Length Indicator", si->incorrect_len_indicator);
}

void sense_dump(unsigned char *sense, int sense_len)
{
        printf("Raw sense buffer:\n");
        response_dump(sense, sense_len);
        printf("\n");

        sense_info_t si;
        bool success = scsi_parse_sense(sense, sense_len, &si);
        printf("Parsing succeeded: %s\n", success ? "yes" : "no");
        sense_dump_sense_info(&si);
}
