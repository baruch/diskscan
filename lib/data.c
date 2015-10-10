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

#include "diskscan.h"
#include "data.h"
#include "compiler.h"
#include "system_id.h"

#include "hdrhistogram/src/hdr_histogram_log.h"

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
		case ERROR_UNKNOWN: return "error_unknown";
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

static inline void buf_to_hex(unsigned char *buf, unsigned buf_len, unsigned char *out_buf, unsigned out_buf_len)
{
	for (; buf_len > 0 && out_buf_len >= 3; buf_len--, out_buf_len -= 2) {
		*(out_buf++) = nibble_to_hex(*buf >> 4);
		*(out_buf++) = nibble_to_hex(*buf & 0x0F);
		buf++;
	}
	*out_buf = 0;
}

static void system_identifier_to_json(system_identifier_t *system_id, char *buf, int buf_len)
{
	int len = snprintf(buf, buf_len, "{ \"System\": \"%s\", \"Chassis\": \"%s\", \"BaseBoard\": \"%s\", \"Mac\": \"%s\", \"OS\": \"%s\" }",
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
	unsigned char sense_hex[sizeof(sense) * 2 + 1];

	buf_to_hex(sense, sense_len, sense_hex, sizeof(sense_hex));

	snprintf(buf, 2048, "{\"SenseKey\": %u, \"Asc\": %u, \"Ascq\": %u, \"FruCode\": %u, \"VendorCode\": %u, \"Hex\": \"%s\"}",
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
	add_indent(f, indent); fprintf(f, "\"Vendor\": \"%s\",\n", disk->vendor);
	add_indent(f, indent); fprintf(f, "\"Model\": \"%s\",\n", disk->model);
	add_indent(f, indent); fprintf(f, "\"FwRev\": \"%s\",\n", disk->fw_rev);
	add_indent(f, indent); fprintf(f, "\"Serial\": \"%s\",\n", disk->serial);
	add_indent(f, indent); fprintf(f, "\"NumSectors\": %"PRIu64",\n", disk->num_bytes / disk->sector_size);
	add_indent(f, indent); fprintf(f, "\"SectorSize\": %"PRIu64"\n", disk->sector_size);
	if (disk->is_ata && disk->ata_buf_len > 0) {
		unsigned char ata_hex[512*2+1];
		buf_to_hex(disk->ata_buf, disk->ata_buf_len, ata_hex, sizeof(ata_hex));
		add_indent(f, indent); fprintf(f, "\"AtaIdentifyRaw\": \"%s\",\n", ata_hex);
	}
	add_indent(f, indent); fprintf(f, "}");
}

static void data_log_event(FILE *f, int indent, uint64_t lba, uint32_t len, io_result_t *io_res, uint32_t t_nsec)
{
	add_indent(f, indent); fprintf(f, "{\"LBA\": %16"PRIu64", \"Len\": %8u, \"LatencyNSec\": %8u, ", lba, len, t_nsec);
	fprintf(f, "\"Data\": \"%s\", ", result_data_to_name(io_res->data));
	fprintf(f, "\"Error\": \"%s\", ", result_error_to_name(io_res->error));
	fprintf(f, "\"Sense\": %s", sense_info_to_json(&io_res->info, io_res->sense, io_res->sense_len));
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
	add_indent(log_raw->f, 1); fprintf(log_raw->f, "\"Disk\": ");
	disk_output(log_raw->f, disk, 2);
	fprintf(log_raw->f, ",\n");

	add_indent(log_raw->f, 1); fprintf(log_raw->f, "\"Raw\": [\n");
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
	add_indent(log->f, 1); fprintf(log->f, "\"Disk\": ");
	disk_output(log->f, disk, 2);
	fprintf(log->f, ",\n");

	add_indent(log->f, 1); fprintf(log->f, "\"Machine\": ");
	system_id_output(log->f);
	fprintf(log->f, ",\n");

	// TODO: Output Disk mode page info
	// TODO: Output Disk SATA configuration

	add_indent(log->f, 1); fprintf(log->f, "\"Scan\": {\n");
	add_indent(log->f, 2); time_output(log->f, "StartTime"); fprintf(log->f, ",\n");
	add_indent(log->f, 2); fprintf(log->f, "\"Events\": [\n");
}

static void histogram_output(FILE *f, struct hdr_histogram *histogram, int indent)
{
	char *encoded_histogram;

	hdr_log_encode(histogram, &encoded_histogram);

	add_indent(f, indent);
	fprintf(f, "\"Histogram\": \"%s\"\n", encoded_histogram);

	free(encoded_histogram);
}

static void latency_output(FILE *f, latency_t *latency, int latency_len, int indent)
{
	//unsigned latency_graph_len;
	//latency_t *latency_graph;
	add_indent(f, indent); fprintf(f, "\"Latencies\": [\n");

	int i;
	for (i = 0; i < latency_len; i++) {
		if (i != 0)
			fprintf(f, ",\n");
		add_indent(f, indent+1);
		fprintf(f, "{");
		fprintf(f, "\"StartSector\": %16"PRIu64, latency[i].start_sector);
		fprintf(f, ", \"EndSector\": %16"PRIu64, latency[i].end_sector);
		fprintf(f, ", \"LatencyMinMsec\": %8u", latency[i].latency_min_msec);
		fprintf(f, ", \"LatencyMaxMsec\": %8u", latency[i].latency_max_msec);
		fprintf(f, ", \"LatencyMedianMsec\": %8u", latency[i].latency_median_msec);
		fprintf(f, "}");
	}
	fprintf(f, "\n");

	add_indent(f, indent); fprintf(f, "],\n");
}

void data_log_end(data_log_t *log, disk_t *disk)
{
	if (log == NULL || log->f == NULL)
		return;

	fprintf(log->f, "\n");
	add_indent(log->f, 2); fprintf(log->f, "],\n");
	// TODO: Output SMART Information
	// TODO: Output Log Page information

	add_indent(log->f, 2); time_output(log->f, "EndTime"); fprintf(log->f, ",\n");

	histogram_output(log->f, disk->histogram, 2);
	latency_output(log->f, disk->latency_graph, disk->latency_graph_len, 2);
	add_indent(log->f, 2); fprintf(log->f, "\"Conclusion\": \"%s\"\n", conclusion_to_str(disk->conclusion));

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

		data_log_event(log->f, 3, lba, len, io_res, t_nsec);
	}
}
