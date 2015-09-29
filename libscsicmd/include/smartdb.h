#ifndef LIBSCSICMD_SMARTDB_H
#define LIBSCSICMD_SMARTDB_H

#include <stdint.h>

typedef struct smart_table smart_table_t;
typedef struct smart_attr smart_attr_t;

typedef enum smart_attr_type {
	SMART_ATTR_TYPE_NONE,
	SMART_ATTR_TYPE_POH,
	SMART_ATTR_TYPE_TEMP,
	SMART_ATTR_TYPE_REALLOC,
	SMART_ATTR_TYPE_REALLOC_PENDING,
	SMART_ATTR_TYPE_CRC_ERRORS,
} smart_attr_type_e;

typedef enum smart_attr_raw {
	SMART_ATTR_RAW_HEX48,
	SMART_ATTR_RAW_DEC48,
} smart_attr_raw_e;

struct smart_attr {
	uint8_t id;
	smart_attr_type_e type;
	smart_attr_raw_e raw;
	int offset;
	const char *name;
};

struct smart_table {
	int num_attrs;
	smart_attr_t attrs[30];
};

const smart_table_t *smart_table_for_disk(const char *vendor, const char *model, const char *firmware);
const smart_attr_t *smart_attr_for_id(const smart_table_t *table, uint8_t id);
const smart_attr_t *smart_attr_for_type(const smart_table_t *table, smart_attr_type_e attr_type);

#endif
