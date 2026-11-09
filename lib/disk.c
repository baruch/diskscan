#include "disk.h"
#include "verbose.h"

#include "libscsicmd/include/ata.h"

static bool smart_command_ok(const io_result_t *result)
{
	return (result->error == ERROR_NONE || result->error == ERROR_CORRECTED) &&
		(!result->info.ata_status_valid || !(result->info.ata_status.status & 0x21));
}

int disk_smart_trip(disk_dev_t *dev)
{
	int cdb_len;
	unsigned char cdb[32];
	unsigned char sense[128];
	unsigned buf_read = 0;
	unsigned sense_read = 0;
	io_result_t io_res;
	bool smart_ok;

	cdb_len = cdb_ata_smart_return_status(cdb);
	disk_dev_cdb_in(dev, cdb, cdb_len, NULL, 0, &buf_read, sense, sizeof(sense), &sense_read, &io_res);
	if (!smart_command_ok(&io_res))
		return -1;
	if (!sense_read || !ata_smart_return_status_result(sense, sense_read, &smart_ok))
		return -1;

	return smart_ok ? 0 : 1;
}

int disk_smart_attributes(disk_dev_t *dev, ata_smart_attr_t *attrs, int max_attrs)
{
	int cdb_len;
	unsigned char cdb[32];
	unsigned char buf[512] = {0};
	unsigned char sense[128];
	unsigned buf_read = 0;
	unsigned sense_read = 0;
	io_result_t io_res;

	cdb_len = cdb_ata_smart_read_data(cdb);
	disk_dev_cdb_in(dev, cdb, cdb_len, buf, sizeof(buf), &buf_read, sense, sizeof(sense), &sense_read, &io_res);

	if (!smart_command_ok(&io_res)) {
		ERROR("SMART READ DATA failed: I/O error=%d, sense=%s (key=0x%02x ASC/ASCQ=%02x/%02x), received=%u/%u bytes",
			io_res.error, sense_key_to_name(io_res.info.sense_key), io_res.info.sense_key,
			io_res.info.asc, io_res.info.ascq, buf_read, (unsigned)sizeof(buf));
		if (io_res.info.ata_status_valid)
			ERROR("ATA status=0x%02x error=0x%02x", io_res.info.ata_status.status, io_res.info.ata_status.error);
		return -1;
	}
	if (io_res.data != DATA_FULL || buf_read != sizeof(buf)) {
		ERROR("SMART READ DATA returned a short response: %u/%u bytes", buf_read, (unsigned)sizeof(buf));
		return -1;
	}
	if (!ata_checksum_verify(buf)) {
		ERROR("SMART READ DATA returned an invalid checksum: got 0x%02x, expected 0x%02x",
			buf[511], ata_calc_checksum(buf));
		return -1;
	}

	int count = ata_parse_ata_smart_read_data(buf, attrs, max_attrs);
	if (count == 0)
		ERROR("SMART READ DATA returned no attributes (revision=0x%04x)", ata_get_ata_smart_read_data_version(buf));
	return count;
}
