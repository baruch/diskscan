#include "sense_key_list.h"
#include "scsicmd.h"

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
