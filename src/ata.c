#include "ata.h"
#include <assert.h>

bool ata_inq_checksum(unsigned char *buf, int buf_len)
{
	char sum;
	int idx;

	if (buf[511] != 0xA5) {
		// Checksum isn't claimed to be valid, nothing to check here
		return 1;
	}

	for (idx = 0, sum = 0; idx < buf_len; idx++) {
		sum += buf[idx];
	}

	return sum == 0;
}

bool ata_status_from_scsi_sense(unsigned char *sense, int sense_len, ata_status_t *status)
{
	assert(status);
	assert(sense[0] == 0x72); // Descriptor based sense for current command
	assert(sense[8] == 9); // Descriptor type
	assert(sense[9] == 12); // Descriptor length
	assert(sense_len >= 22);
	// TODO: Proper parsing and return false on parse failure

	status->extend = sense[10] & 1;
	status->error = sense[11];

	if (status->extend)
		status->sector_count = sense[12]<<8 | sense[13];
	else
		status->sector_count = sense[13];

	if (status->extend)
		status->lba = sense[15] | (sense[14]<<8) | (sense[17]<<16) | (sense[16]<<24) | ((uint64_t)sense[19]<<32) | ((uint64_t)sense[18]<<40);
	else
		status->lba = sense[15] | (sense[17]<<8) | (sense[19]<<16);

	status->device = sense[20];
	status->status = sense[21];

	return true;
}
