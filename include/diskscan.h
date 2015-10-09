#ifndef _DISKSCAN_H_
#define _DISKSCAN_H_

#include <stdio.h>
#include <stdint.h>
#include "arch.h"

#include "libscsicmd/include/ata.h"

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

struct histogram_def_t {
	uint64_t top_val;
	double percent_bad;
};

static const struct histogram_def_t histogram_time[] = {
	{.top_val =    1, .percent_bad = 100.0},
	{.top_val =   10, .percent_bad = 100.0},
	{.top_val =  100, .percent_bad = 100.0},
	{.top_val =  500, .percent_bad = 100.0},
	{.top_val = 1000, .percent_bad = 10.0},
	{.top_val = 2000, .percent_bad = 1.0},
	{.top_val = 3000, .percent_bad = 1.0},
	{.top_val = 4000, .percent_bad = 1.0},
	{.top_val = 5000, .percent_bad = 1.0},
    {.top_val = 6000, .percent_bad = 0.5},
    {.top_val = 7000, .percent_bad = 0.5},
    {.top_val = 8000, .percent_bad = 0.5},
    {.top_val = 9000, .percent_bad = 0.5},
    {.top_val = 10000, .percent_bad = 0.1},
    {.top_val = 15000, .percent_bad = 0.01},
    {.top_val = 20000, .percent_bad = 0.0},
    {.top_val = 25000, .percent_bad = 0.0},
    {.top_val = 30000, .percent_bad = 0.0},
    {.top_val = UINT64_MAX, .percent_bad = 0.0}
};

enum scan_mode {
	SCAN_MODE_UNKNOWN,
	SCAN_MODE_SEQ,
	SCAN_MODE_RANDOM,
};

enum conclusion {
	CONCLUSION_SCAN_PROBLEM, /* Problem in the scan, no real conclusion */
	CONCLUSION_ABORTED, /* Scan aborted by used */
	CONCLUSION_PASSED,  /* Disk looks fine */
	CONCLUSION_FAILED,  /* Disk looks bad */
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
	uint64_t histogram[ARRAY_SIZE(histogram_time)];
	unsigned latency_graph_len;
	latency_t *latency_graph;
	enum conclusion conclusion;

	data_log_raw_t data_raw;
	data_log_t data_log;
} disk_t;

int disk_open(disk_t *disk, const char *path, int fix, unsigned latency_graph_len);
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
