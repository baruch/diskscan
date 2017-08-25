#include "parse_read_defect_data.h"

static const char *defect_data_format_str[] = {
	"Short",
	"Reserved (1)",
	"Reserved (2)",
	"Long",
	"Index",
	"Physical",
	"Vendor",
	"Reserved (7)",
};

const char *read_defect_data_format_to_str(uint8_t fmt)
{
	if (fmt > 7)
		return "Unknown";
	return defect_data_format_str[fmt];
}
