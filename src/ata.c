#include "ata.h"

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
