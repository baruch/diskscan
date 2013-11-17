#ifndef SYSTEM_ID_H
#define SYSTEM_ID_H

#include <stdbool.h>

typedef struct system_identifier_t {
	char os[64];
	char system[64];
	char chassis[64];
	char baseboard[64];
	char mac[64];
} system_identifier_t;

bool system_identifier_read(system_identifier_t *system_id);

#endif
