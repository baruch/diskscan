/*
 *  Copyright 2013 Baruch Even <baruch@ev-en.org>
 *
 *  This file is part of DiskScan.
 *
 *  DiskScan is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *  DiskScan is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with DiskScan.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */

#include "config.h"
#include "diskscan.h"
#include "data.h"
#include "compiler.h"
#include "system_id.h"

#include <inttypes.h>
#include <memory.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

static const char *result_data_to_name(enum result_data_e data)
{
	switch (data) {
		case DATA_FULL: return "data_full";
		case DATA_PARTIAL: return "data_partial";
		case DATA_NONE: return "data_none";
	}

	return "data_unknown";
}

static const char *result_error_to_name(enum result_error_e error)
{
	switch (error) {
		case ERROR_NONE: return "error_none";
		case ERROR_CORRECTED: return "error_corrected";
		case ERROR_UNCORRECTED: return "error_uncorrected";
		case ERROR_NEED_RETRY: return "error_need_retry";
		case ERROR_FATAL: return "error_fatal";
	}

	return "error_unknown";
}

static inline char nibble_to_hex(unsigned char nibble)
{
	if (nibble < 10)
		return '0' + nibble;
	else
		return 'A' + nibble - 10;
}

static void system_identifier_to_json(system_identifier_t *system_id, char *buf, int buf_len)
{
	int len = snprintf(buf, buf_len, "{ \"system\": \"%s\", \"chassis\": \"%s\", \"baseboard\": \"%s\", \"mac\": \"%s\", \"os\": \"%s\" }",
			system_id->system, system_id->chassis, system_id->baseboard, system_id->mac, system_id->os);
	// If we failed we should at least keep it a valid json
	if (len >= buf_len || len <= 0)
		snprintf(buf, buf_len, "{}");
}

static void system_id_output(FILE *f)
{
	char buf[2048];
	system_identifier_t system_id;

	memset(&system_id, 0, sizeof(system_id));
	if (system_identifier_read(&system_id)) {
		system_identifier_to_json(&system_id, buf, sizeof(buf));
		fputs(buf, f);
	} else {
		fputs("{}", f);
	}
}

static const char *sense_info_to_json(struct sense_info_t *info, unsigned char *sense, unsigned sense_len)
{
	static char buf[2048];
	char sense_hex[sizeof(sense) * 2 + 1];
	unsigned i, j;

	for (i = 0, j = 0; i < sense_len; i++) {
		sense_hex[j++] = nibble_to_hex(sense[i] >> 4);
		sense_hex[j++] = nibble_to_hex(sense[i] & 0x0F);
	}
	sense_hex[j] = 0;

	snprintf(buf, 2048, "{\"sense_key\": %u, \"asc\": %u, \"ascq\": %u, \"fru_code\": %u, \"vendor_code\": %u, \"raw\": \"%s\"}",
			info->sense_key, info->asc, info->ascq,
			info->fru_code_valid ? info->fru_code : 0,
			info->vendor_unique_error,
			sense_hex);

    //    bool ata_status_valid;
    //    ata_status_t ata_status;
	return buf;
}


static inline void add_indent(FILE *f, int indent)
{
	int i;
	for (i = 0; i < indent*4; i++)
		fprintf(f, " ");
}

static void disk_output(FILE *f, disk_t *disk, int indent)
{
	fprintf(f, "{\n");
	add_indent(f, indent); fprintf(f, "\"vendor\": \"%s\",\n", disk->vendor);
	add_indent(f, indent); fprintf(f, "\"model\": \"%s\",\n", disk->model);
	add_indent(f, indent); fprintf(f, "\"fw_rev\": \"%s\",\n", disk->fw_rev);
	add_indent(f, indent); fprintf(f, "\"serial\": \"%s\",\n", disk->serial);
	add_indent(f, indent); fprintf(f, "\"num_sectors\": \"%"PRIu64"\",\n", disk->num_bytes / disk->sector_size);
	add_indent(f, indent); fprintf(f, "\"sector_size\": \"%"PRIu64"\"\n", disk->sector_size);
	add_indent(f, indent); fprintf(f, "}");
}

static void data_log_event(FILE *f, int indent, uint64_t lba, uint32_t len, io_result_t *io_res, uint32_t t_nsec)
{
	add_indent(f, indent); fprintf(f, "{\"lba\": %16"PRIu64", \"len\": %8u, \"latency_nsec\": %8u, ", lba, len, t_nsec);
	fprintf(f, "\"data\": \"%s\", ", result_data_to_name(io_res->data));
	fprintf(f, "\"error\": \"%s\", ", result_error_to_name(io_res->error));
	fprintf(f, "\"sense\": %s", sense_info_to_json(&io_res->info, io_res->sense, io_res->sense_len));
	fprintf(f, "}");
}

void data_log_raw_start(data_log_raw_t *log_raw, const char *filename, disk_t *disk)
{
	log_raw->f = fopen(filename, "wt");
	if (log_raw->f == NULL)
		return;
	log_raw->is_first = true;

	fprintf(log_raw->f, "{\n");

	// Information about the disk itself
	add_indent(log_raw->f, 1); fprintf(log_raw->f, "\"disk\": ");
	disk_output(log_raw->f, disk, 2);
	fprintf(log_raw->f, ",\n");

	add_indent(log_raw->f, 1); fprintf(log_raw->f, "\"raw\": [\n");
}

void data_log_raw_end(data_log_raw_t *log_raw)
{
	fprintf(log_raw->f, "\n"); // End the line we left open from data_log_raw
	add_indent(log_raw->f, 1); fprintf(log_raw->f, "]\n"); // Close the raw log array
	fprintf(log_raw->f, "}\n"); // Close the entire struct
	fclose(log_raw->f);
}

void data_log_raw(data_log_raw_t *log_raw, uint64_t lba, uint32_t len, io_result_t *io_res, uint32_t t_nsec)
{
	if (log_raw == NULL || log_raw->f == NULL)
		return;

	if (!log_raw->is_first)
		fprintf(log_raw->f, ",\n");
	else
		log_raw->is_first = false;

	data_log_event(log_raw->f, 2, lba, len, io_res, t_nsec);
}

static void time_output(FILE *f, const char *name)
{
	char now[64];
	time_t t;
	struct tm *tmp;

	t = time(NULL);
	tmp = gmtime(&t);
	if (tmp != NULL)
		strftime(now, sizeof(now), "%Y-%m-%d %H:%M:%S", tmp);
	else
		snprintf(now, sizeof(now), "%"PRIu64, (uint64_t)t);
	fprintf(f, "\"%s\": \"%s\"", name, now);
}

void data_log_start(data_log_t *log, const char *filename, disk_t *disk)
{
	log->f = fopen(filename, "wt");
	if (!log->f)
		return;
	log->is_first = true;

	fprintf(log->f, "{\n");
	add_indent(log->f, 1); fprintf(log->f, "\"disk\": ");
	disk_output(log->f, disk, 2);
	fprintf(log->f, ",\n");

	add_indent(log->f, 1); fprintf(log->f, "\"machine\": ");
	system_id_output(log->f);
	fprintf(log->f, ",\n");

	// TODO: Output Disk mode page info
	// TODO: Output Disk SATA configuration

	add_indent(log->f, 1); fprintf(log->f, "\"scan\": {\n");
	add_indent(log->f, 2); time_output(log->f, "start_time"); fprintf(log->f, ",\n");
	add_indent(log->f, 2); fprintf(log->f, "\"events\": [\n");
}

static void histogram_output(FILE *f, uint64_t *histogram, int histogram_len, int indent)
{
	//uint64_t histogram[ARRAY_SIZE(histogram_time)];
	add_indent(f, indent); fprintf(f, "\"histogram\": [\n");

	int i;
	for (i = 0; i < histogram_len; i++) {
		if (i != 0)
			fprintf(f, ",\n");
		add_indent(f, indent+1);
		fprintf(f, "{");
		fprintf(f, "\"latency_msec\": %24"PRIu64, histogram_time[i]);
		fprintf(f, ", \"count\": %24"PRIu64, histogram[i]);
		fprintf(f, "}");
	}
	fprintf(f, "\n");

	add_indent(f, indent); fprintf(f, "],\n");
}

static void latency_output(FILE *f, latency_t *latency, int latency_len, int indent)
{
	//unsigned latency_graph_len;
	//latency_t *latency_graph;
	add_indent(f, indent); fprintf(f, "\"latencies\": [\n");

	int i;
	for (i = 0; i < latency_len; i++) {
		if (i != 0)
			fprintf(f, ",\n");
		add_indent(f, indent+1);
		fprintf(f, "{");
		fprintf(f, "\"start_sector\": %16"PRIu64, latency[i].start_sector);
		fprintf(f, ", \"end_sector\": %16"PRIu64, latency[i].end_sector);
		fprintf(f, ", \"latency_min_msec\": %8u", latency[i].latency_min_msec);
		fprintf(f, ", \"latency_max_msec\": %8u", latency[i].latency_max_msec);
		fprintf(f, ", \"latency_median_msec\": %8u", latency[i].latency_median_msec);
		fprintf(f, "}");
	}
	fprintf(f, "\n");

	add_indent(f, indent); fprintf(f, "]\n");
}

void data_log_end(data_log_t *log, disk_t *disk)
{
	if (log == NULL || log->f == NULL)
		return;

	fprintf(log->f, "\n");
	add_indent(log->f, 2); fprintf(log->f, "],\n");
	// TODO: Output the latency histogram data
	// TODO: Output SMART Information
	// TODO: Output Log Page information

	add_indent(log->f, 2); time_output(log->f, "end_time"); fprintf(log->f, ",\n");

	histogram_output(log->f, disk->histogram, ARRAY_SIZE(disk->histogram), 2);
	latency_output(log->f, disk->latency_graph, disk->latency_graph_len, 2);

	add_indent(log->f, 1); fprintf(log->f, "}\n");
	fprintf(log->f, "}\n");
}

void data_log(data_log_t *log, uint64_t lba, uint32_t len, io_result_t *io_res, uint32_t t_nsec)
{
	if (log == NULL || log->f == NULL)
		return;

	if (io_res->data != DATA_FULL || io_res->error != ERROR_NONE || t_nsec > 1000*1000*1000) {
		if (!log->is_first)
			fprintf(log->f, ",\n");
		else
			log->is_first = false;

		data_log_event(log->f, 2, lba, len, io_res, t_nsec);
	}
}
