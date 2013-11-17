#ifndef _DISKSCAN_H_
#define _DISKSCAN_H_

#include <stdio.h>
#include <stdint.h>
#include "arch.h"

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

static const uint64_t histogram_time[] = {1, 10, 100, 500, 1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000, 15000, 20000, 25000, 30000, UINT64_MAX};

enum scan_mode {
	SCAN_MODE_UNKNOWN,
	SCAN_MODE_SEQ,
	SCAN_MODE_RANDOM,
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

typedef struct disk_t {
	disk_dev_t dev;
	char path[128];
	char vendor[32];
	char model[32];
	char fw_rev[32];
	char serial[32];
	bool is_ata;
	uint64_t num_bytes;
	uint64_t sector_size;
	int run;
	int fix;

	uint64_t num_errors;
	uint64_t histogram[ARRAY_SIZE(histogram_time)];
	unsigned latency_graph_len;
	latency_t *latency_graph;

	data_log_raw_t data_raw;
	data_log_t data_log;
} disk_t;

int disk_open(disk_t *disk, const char *path, int fix, unsigned latency_graph_len);
int disk_scan(disk_t *disk, enum scan_mode mode, unsigned data_size);
int disk_close(disk_t *disk);
void disk_scan_stop(disk_t *disk);

enum scan_mode str_to_scan_mode(const char *s);

/* Implemented by the user (gui/cli) */
void report_progress(disk_t *disk, int percent_part, int percent_full);
void report_scan_success(disk_t *disk, uint64_t offset_bytes, uint64_t data_size, uint64_t time);
void report_scan_error(disk_t *disk, uint64_t offset_bytes, uint64_t data_size, uint64_t time);
void report_scan_done(disk_t *disk);

/* Used to log data to files */
void data_log_raw_start(data_log_raw_t *log_raw, const char *filename, disk_t *disk);
void data_log_raw_end(data_log_raw_t *log_raw);
void data_log_start(data_log_t *log, const char *filename, disk_t *disk);
void data_log_end(data_log_t *log);

#endif
