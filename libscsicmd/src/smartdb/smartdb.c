#include "smartdb.h"
#include <stdlib.h>

const smart_attr_t *smart_attr_for_id(const smart_table_t *table, uint8_t id)
{
	int i;

	for (i = 0; i < table->num_attrs; i++) {
		if (table->attrs[i].id == id)
			return &table->attrs[i];
	}

	return NULL;
}

const smart_attr_t *smart_attr_for_type(const smart_table_t *table, smart_attr_type_e attr_type)
{
	int i;

	for (i = 0; i < table->num_attrs; i++) {
		if (table->attrs[i].type == attr_type)
			return &table->attrs[i];
	}

	return NULL;
}
