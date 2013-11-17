#ifndef DISKSCAN_DATA_H
#define DISKSCAN_DATA_H

#include "arch.h"

void data_log(data_log_t *log, uint64_t lba, uint32_t len, io_result_t *io_res, uint32_t t_nsec);
void data_log_raw(data_log_raw_t *log_raw, uint64_t lba, uint32_t len, io_result_t *io_res, uint32_t t_nsec);

#endif
