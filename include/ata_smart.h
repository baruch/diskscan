#ifndef LIBSCSICMD_ATA_SMART_H
#define LIBSCSICMD_ATA_SMART_H

#include "smartdb.h"
#include "ata.h"

/* These functions are for common information that may show up differently in different drives (or unsupported) */
int ata_smart_get_temperature(const ata_smart_attr_t *attrs, int num_attrs, const smart_table_t *table, int *min_temp, int *max_temp);
int ata_smart_get_power_on_hours(const ata_smart_attr_t *attrs, int num_attrs, const smart_table_t *table, int *pminutes);
int ata_smart_get_num_reallocations(const ata_smart_attr_t *attrs, int num_attrs, const smart_table_t *table);
int ata_smart_get_num_pending_reallocations(const ata_smart_attr_t *attrs, int num_attrs, const smart_table_t *table);
int ata_smart_get_num_crc_errors(const ata_smart_attr_t *attrs, int num_attrs, const smart_table_t *table);

#endif
