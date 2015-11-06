#include "smartdb.h"
static const smart_table_t defaults = {
.num_attrs = 26,
.attrs = {
{.id=1, .type=SMART_ATTR_TYPE_NONE, .name="Raw Read Error Rate", .raw=SMART_ATTR_RAW_DEC48, .offset=-1},
{.id=2, .type=SMART_ATTR_TYPE_NONE, .name="Throughput Performance", .raw=SMART_ATTR_RAW_DEC48, .offset=-1},
{.id=3, .type=SMART_ATTR_TYPE_NONE, .name="Spin Up Time", .raw=SMART_ATTR_RAW_DEC48, .offset=-1},
{.id=4, .type=SMART_ATTR_TYPE_NONE, .name="Start/Stop Count", .raw=SMART_ATTR_RAW_DEC48, .offset=-1},
{.id=5, .type=SMART_ATTR_TYPE_REALLOC, .name="Reallocated Sectors Count", .raw=SMART_ATTR_RAW_DEC48, .offset=-1},
{.id=7, .type=SMART_ATTR_TYPE_NONE, .name="Seek Error Rate", .raw=SMART_ATTR_RAW_DEC48, .offset=-1},
{.id=8, .type=SMART_ATTR_TYPE_NONE, .name="Seek Time Performance", .raw=SMART_ATTR_RAW_DEC48, .offset=-1},
{.id=9, .type=SMART_ATTR_TYPE_POH, .name="Power On Hours", .raw=SMART_ATTR_RAW_DEC48, .offset=-1},
{.id=10, .type=SMART_ATTR_TYPE_NONE, .name="Spin Retry Count", .raw=SMART_ATTR_RAW_DEC48, .offset=-1},
{.id=11, .type=SMART_ATTR_TYPE_NONE, .name="Drive Calibration Retry Count", .raw=SMART_ATTR_RAW_DEC48, .offset=-1},
{.id=12, .type=SMART_ATTR_TYPE_NONE, .name="Device Power Cycle Count", .raw=SMART_ATTR_RAW_DEC48, .offset=-1},
{.id=13, .type=SMART_ATTR_TYPE_NONE, .name="Read Soft Error Rate", .raw=SMART_ATTR_RAW_DEC48, .offset=-1},
{.id=191, .type=SMART_ATTR_TYPE_NONE, .name="G Sense Error Rate", .raw=SMART_ATTR_RAW_DEC48, .offset=-1},
{.id=192, .type=SMART_ATTR_TYPE_NONE, .name="Power Off Retract Count", .raw=SMART_ATTR_RAW_DEC48, .offset=-1},
{.id=193, .type=SMART_ATTR_TYPE_NONE, .name="Load/Unload Cycle Count", .raw=SMART_ATTR_RAW_DEC48, .offset=-1},
{.id=194, .type=SMART_ATTR_TYPE_TEMP, .name="Temperature", .raw=SMART_ATTR_RAW_DEC48, .offset=150},
{.id=195, .type=SMART_ATTR_TYPE_NONE, .name="Hardware ECC Recovered", .raw=SMART_ATTR_RAW_DEC48, .offset=-1},
{.id=196, .type=SMART_ATTR_TYPE_NONE, .name="Reallocation Event Count", .raw=SMART_ATTR_RAW_DEC48, .offset=-1},
{.id=197, .type=SMART_ATTR_TYPE_REALLOC_PENDING, .name="Pending Sector Reallocation Count", .raw=SMART_ATTR_RAW_DEC48, .offset=-1},
{.id=198, .type=SMART_ATTR_TYPE_NONE, .name="Off-Line Scan Uncorrecable Sector Count", .raw=SMART_ATTR_RAW_DEC48, .offset=-1},
{.id=199, .type=SMART_ATTR_TYPE_CRC_ERRORS, .name="CRC Error Count", .raw=SMART_ATTR_RAW_DEC48, .offset=-1},
{.id=200, .type=SMART_ATTR_TYPE_NONE, .name="Multi-zone Error Rate", .raw=SMART_ATTR_RAW_DEC48, .offset=-1},
{.id=240, .type=SMART_ATTR_TYPE_NONE, .name="Head Flying Hours", .raw=SMART_ATTR_RAW_DEC48, .offset=-1},
{.id=241, .type=SMART_ATTR_TYPE_NONE, .name="Total LBAs Written", .raw=SMART_ATTR_RAW_DEC48, .offset=-1},
{.id=242, .type=SMART_ATTR_TYPE_NONE, .name="Total LBAs Read", .raw=SMART_ATTR_RAW_DEC48, .offset=-1},
{.id=254, .type=SMART_ATTR_TYPE_NONE, .name="Free Fall Sensor", .raw=SMART_ATTR_RAW_DEC48, .offset=-1},
}
};
const smart_table_t * smart_table_for_disk(const char *vendor, const char *model, const char *firmware)
{
(void)vendor;
(void)model;
(void)firmware;
return &defaults;
}
