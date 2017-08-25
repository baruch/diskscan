#include "parse_log_sense.h"

bool log_sense_page_informational_exceptions(uint8_t *page, unsigned page_len, uint8_t *asc, uint8_t *ascq, uint8_t *temperature)
{
	if (!log_sense_is_valid(page, page_len))
		return false;
	if (log_sense_page_code(page) != 0x2F)
		return false;
	if (log_sense_subpage_format(page) && log_sense_subpage_code(page) != 0)
		return false;

	uint8_t *param;
	for_all_log_sense_params(page, page_len, param) {
		if (log_sense_param_code(param) == 0) {
			uint8_t *param_data = log_sense_param_data(param);
			*asc = param_data[0];
			*ascq = param_data[1];
			*temperature = param_data[2];
			return true;
		}
	}

	return false;
}


