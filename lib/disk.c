#include "disk.h"

#include "libscsicmd/include/ata.h"

int disk_smart_trip(disk_dev_t *dev)
{
	int cdb_len;
	unsigned char cdb[32];
	unsigned char buf[512];
	unsigned char sense[128];
	unsigned buf_read = 0;
	unsigned sense_read = 0;
	io_result_t io_res;
	bool smart_ok;

	cdb_len = cdb_ata_smart_return_status(cdb);
	disk_dev_cdb_in(dev, cdb, cdb_len, buf, sizeof(buf), &buf_read, sense, sizeof(sense), &sense_read, &io_res);
	if (!ata_smart_return_status_result(sense, sense_read, &smart_ok))
		return -1;

	return smart_ok ? 0 : 1;
}

int disk_smart_attributes(disk_dev_t *dev, ata_smart_attr_t *attrs, int max_attrs)
{
	int cdb_len;
	unsigned char cdb[32];
	unsigned char buf[512];
	unsigned char sense[128];
	unsigned buf_read = 0;
	unsigned sense_read = 0;
	io_result_t io_res;

	cdb_len = cdb_ata_smart_read_data(cdb);
	disk_dev_cdb_in(dev, cdb, cdb_len, buf, sizeof(buf), &buf_read, sense, sizeof(sense), &sense_read, &io_res);

	// TODO: Need to report about the SMART failure, once.
	if (io_res.data != DATA_FULL)
		return -1;

	return ata_parse_ata_smart_read_data(buf, attrs, max_attrs);
}
