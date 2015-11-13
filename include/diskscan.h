#ifndef _DISKSCAN_H_
#define _DISKSCAN_H_

#include <stdio.h>
#include <stdint.h>
#include "arch.h"

#include "libscsicmd/include/ata.h"
#include "hdrhistogram/src/hdr_histogram.h"

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

enum scan_mode {
	SCAN_MODE_UNKNOWN,
	SCAN_MODE_SEQ,
	SCAN_MODE_RANDOM,
};

enum conclusion {
	CONCLUSION_SCAN_PROBLEM, /* Problem in the scan, no real conclusion */
	CONCLUSION_ABORTED, /* Scan aborted by used */
	CONCLUSION_PASSED,  /* Disk looks fine */
	/* Disk looks bad, and the reason it failed the test */
	CONCLUSION_FAILED_MAX_LATENCY,
	CONCLUSION_FAILED_LATENCY_PERCENTILE,
	CONCLUSION_FAILED_IO_ERRORS,
};

typedef struct latency_t {
	uint64_t start_sector;
	uint64_t end_sector;
	uint32_t latency_min_msec;
	uint32_t latency_max_msec;
	uint32_t latency_median_msec;
} latency_t;

typedef struct data_log_raw_t {
	FILE *f;
	bool is_first;
} data_log_raw_t;

typedef struct data_log_t {
	FILE *f;
	bool is_first;
} data_log_t;

typedef struct ata_state_t {
	bool is_smart_tripped;
	const struct smart_table *smart_table;
	ata_smart_attr_t smart[MAX_SMART_ATTRS];
	int smart_num;
	int last_temp;
	int last_reallocs;
	int last_pending_reallocs;
	int last_crc_errors;
} ata_state_t;

typedef struct scsi_state_t {
} scsi_state_t;

typedef struct disk_t {
	disk_dev_t dev;
	char path[128];
	char vendor[64];
	char model[64];
	char fw_rev[64];
	char serial[64];
	bool is_ata;
	union {
		ata_state_t ata;
		scsi_state_t scsi;
	} state;
	unsigned char ata_buf[512];
	unsigned ata_buf_len;
	uint64_t num_bytes;
	uint64_t sector_size;
	int run;
	int fix;

	uint64_t num_errors;
	struct hdr_histogram *histogram;
	unsigned latency_graph_len;
	latency_t *latency_graph;
	enum conclusion conclusion;

	data_log_raw_t data_raw;
	data_log_t data_log;
} disk_t;

int disk_open(disk_t *disk, const char *path, int fix, unsigned latency_graph_len, disk_mount_e allowed_mount);
int disk_scan(disk_t *disk, enum scan_mode mode, unsigned data_size);
int disk_close(disk_t *disk);
void disk_scan_stop(disk_t *disk);

enum scan_mode str_to_scan_mode(const char *s);
const char *conclusion_to_str(enum conclusion conclusion);

/* Implemented by the user (gui/cli) */
void report_progress(disk_t *disk, int percent_part, int percent_full);
void report_scan_success(disk_t *disk, uint64_t offset_bytes, uint64_t data_size, uint64_t time);
void report_scan_error(disk_t *disk, uint64_t offset_bytes, uint64_t data_size, uint64_t time);
void report_scan_done(disk_t *disk);

/* Used to log data to files */
void data_log_raw_start(data_log_raw_t *log_raw, const char *filename, disk_t *disk);
void data_log_raw_end(data_log_raw_t *log_raw);
void data_log_start(data_log_t *log, const char *filename, disk_t *disk);
void data_log_end(data_log_t *log, disk_t *disk);

#endif
