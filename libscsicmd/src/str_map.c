#include "scsicmd.h"

#include <stdio.h>

const char *sense_key_to_name(enum sense_key_e sense_key)
{
#undef SENSE_KEY_MAP

	switch (sense_key) {
#define SENSE_KEY_MAP(_name_, _val_) \
		case _val_: return #_name_ ;
		SENSE_KEY_LIST
	}
#undef SENSE_KEY_MAP

	return "Unknown sense key";
}

const char *asc_num_to_name(uint8_t asc, uint8_t ascq)
{
	static char msg[64];

	uint16_t asc_full = asc<<8 | ascq;

	switch (asc_full) {
#define SENSE_CODE_KEYED(_asc_, _fmt_)
#define SENSE_CODE(_asc_, _ascq_, _msg_) case _asc_<<8 | _ascq_: return _msg_;
	ASC_NUM_LIST
#undef SENSE_CODE
#undef SENSE_CODE_KEYED
	}

#define SENSE_CODE_KEYED(_asc_, _fmt_) if (asc == _asc_) { snprintf(msg, sizeof(msg), _fmt_, ascq); return msg; }
#define SENSE_CODE(_asc_, _ascq_, _msg_)
	ASC_NUM_LIST
#undef SENSE_CODE
#undef SENSE_CODE_KEYED

	snprintf(msg, sizeof(msg), "UNKNOWN ASC/ASCQ (%02Xh/%02Xh)", asc, ascq);
	return msg;
}
