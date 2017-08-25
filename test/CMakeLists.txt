add_library(testlib STATIC main.c sense_dump.c)

add_executable(ata_check_power_mode ata_check_power_mode.c)
target_link_libraries(ata_check_power_mode testlib scsicmd)

add_custom_command(OUTPUT ata_identify_dump.h
                   COMMAND ../structs/ata_struct_2_h_dump.py ../structs/ata_identify.yaml > ata_identify_dump.h
                   DEPENDS ../structs/ata_struct_2_h_dump.py ../structs/ata_identify.yaml)
add_custom_command(OUTPUT ata_identify_dump.c
                   COMMAND ../structs/ata_struct_2_c_dump.py ../structs/ata_identify.yaml > ata_identify_dump.c
                   DEPENDS ../structs/ata_struct_2_c_dump.py ../structs/ata_identify.yaml)
add_executable(ata_identify ata_identify.c ata_identify_dump.c ata_identify_dump.h)
target_link_libraries(ata_identify testlib scsicmd)

add_executable(ata_smart_read_data ata_smart_read_data.c)
target_link_libraries(ata_smart_read_data testlib scsicmd)

add_executable(ata_smart_return_status ata_smart_return_status.c)
target_link_libraries(ata_smart_return_status testlib scsicmd)

add_executable(scsi_inquiry scsi_inquiry.c)
target_link_libraries(scsi_inquiry testlib scsicmd)

add_executable(scsi_log_sense scsi_log_sense.c)
target_link_libraries(scsi_log_sense testlib scsicmd)

add_executable(parse_scsi parse_scsi.c)
target_link_libraries(parse_scsi testlib scsicmd)

add_executable(scsi_mode_sense scsi_mode_sense.c)
target_link_libraries(scsi_mode_sense testlib scsicmd)

add_executable(scsi_receive_diagnostics scsi_receive_diagnostics.c)
target_link_libraries(scsi_receive_diagnostics testlib scsicmd)

add_executable(scsi_read_capacity_10 scsi_read_capacity_10.c)
target_link_libraries(scsi_read_capacity_10 testlib scsicmd)

add_executable(scsi_read_capacity_16 scsi_read_capacity_16.c)
target_link_libraries(scsi_read_capacity_16 testlib scsicmd)

add_executable(sense_decode sense_decode.c)
target_link_libraries(sense_decode testlib scsicmd)

add_executable(collect_raw_data collect_raw_data.c)
target_link_libraries(collect_raw_data testlib scsicmd)
