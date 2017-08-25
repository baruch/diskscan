/* Generated file, do not edit */
#ifndef ATA_PARSE_H
#define ATA_PARSE_H
#include "ata.h"
static inline bool ata_get_ata_identify_smart_enabled(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 85);
	return val & (1 << 0);
}

static inline bool ata_get_ata_identify_sct_write_same_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 206);
	return val & (1 << 2);
}

static inline bool ata_get_ata_identify_extended_number_of_user_addressable_sectors(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 69);
	return val & (1 << 3);
}

static inline bool ata_get_ata_identify_sense_data_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 119);
	return val & (1 << 6);
}

static inline bool ata_get_ata_identify_fields_valid_words_64_70(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 53);
	return val & (1 << 1);
}

static inline bool ata_get_ata_identify_crypto_scramble_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 59);
	return val & (1 << 13);
}

static inline bool ata_get_ata_identify_rzat_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 69);
	return val & (1 << 5);
}

static inline ata_qword_t ata_get_ata_identify_extended_num_user_addressable_sectors(const unsigned char *buf) {
	return ata_get_qword(buf, 230);
}

static inline bool ata_get_ata_identify_wwn_64bit_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 84);
	return val & (1 << 8);
}

static inline bool ata_get_ata_identify_standby_timer_values_settable(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 49);
	return val & (1 << 13);
}

static inline bool ata_get_ata_identify_write_buffer_dma_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 69);
	return val & (1 << 10);
}

static inline void ata_get_ata_identify_fw_rev(const unsigned char *buf, char *out) {
	ata_get_string(buf, 23, 26, out);
}

static inline bool ata_get_ata_identify_sata_software_settings_preservation_enabled(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 79);
	return val & (1 << 6);
}

static inline bool ata_get_ata_identify_sata_device_initiated_power_management_enabled(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 79);
	return val & (1 << 3);
}

static inline bool ata_get_ata_identify_write_uncorrectable_enabled(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 120);
	return val & (1 << 2);
}

static inline bool ata_get_ata_identify_sct_command_transport_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 206);
	return val & (1 << 0);
}

static inline ata_longword_t ata_get_ata_identify_wwn_high(const unsigned char *buf) {
	return ata_get_longword(buf, 108);
}

static inline bool ata_get_ata_identify_supports_sata_gen1_1_5gbps(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 76);
	return val & (1 << 1);
}

static inline bool ata_get_ata_identify_supports_receipt_of_host_initiated_power_management_requests(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 76);
	return val & (1 << 9);
}

static inline bool ata_get_ata_identify_supports_receive_fpdma_queued(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 77);
	return val & (1 << 6);
}

static inline bool ata_get_ata_identify_smart_self_test_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 84);
	return val & (1 << 1);
}

static inline bool ata_get_ata_identify_overwrite_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 59);
	return val & (1 << 14);
}

static inline unsigned ata_get_ata_identify_queue_depth(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 75);
	return (val >> 0) & ((1<<(4 - 0 + 1)) - 1);
}

static inline bool ata_get_ata_identify_encrypt_all_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 69);
	return val & (1 << 4);
}

static inline bool ata_get_ata_identify_write_buffer_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 82);
	return val & (1 << 12);
}

static inline bool ata_get_ata_identify_streaming_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 84);
	return val & (1 << 4);
}

static inline bool ata_get_ata_identify_download_microcode_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 83);
	return val & (1 << 0);
}

static inline bool ata_get_ata_identify_response_incomplete(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 0);
	return val & (1 << 2);
}

static inline bool ata_get_ata_identify_sct_feature_control_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 206);
	return val & (1 << 4);
}

static inline bool ata_get_ata_identify_puis_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 83);
	return val & (1 << 5);
}

static inline bool ata_get_ata_identify_supports_read_log_dma_ext_as_read_log_dma(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 76);
	return val & (1 << 15);
}

static inline void ata_get_ata_identify_serial_number(const unsigned char *buf, char *out) {
	ata_get_string(buf, 10, 19, out);
}

static inline bool ata_get_ata_identify_supports_host_automatic_partial_to_slumber(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 76);
	return val & (1 << 13);
}

static inline bool ata_get_ata_identify_cfa_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 83);
	return val & (1 << 2);
}

static inline bool ata_get_ata_identify_sata_in_order_data_delivery_enabled(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 79);
	return val & (1 << 4);
}

static inline bool ata_get_ata_identify_major_version_acs_2(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 80);
	return val & (1 << 9);
}

static inline void ata_get_ata_identify_additional_product_identifier(const unsigned char *buf, char *out) {
	ata_get_string(buf, 170, 173, out);
}

static inline bool ata_get_ata_identify_volatile_write_cache_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 82);
	return val & (1 << 5);
}

static inline bool ata_get_ata_identify_smart_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 82);
	return val & (1 << 0);
}

static inline bool ata_get_ata_identify_sct_data_tables_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 206);
	return val & (1 << 5);
}

static inline bool ata_get_ata_identify_supports_dev_automatic_partial_to_slumber(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 76);
	return val & (1 << 14);
}

static inline unsigned ata_get_ata_identify_current_negotiated_link_speed(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 77);
	return (val >> 1) & ((1<<(3 - 1 + 1)) - 1);
}

static inline bool ata_get_ata_identify_supports_sata_gen2_3gbps(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 76);
	return val & (1 << 2);
}

static inline bool ata_get_ata_identify_read_look_ahead_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 82);
	return val & (1 << 6);
}

static inline bool ata_get_ata_identify_read_buffer_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 82);
	return val & (1 << 13);
}

static inline bool ata_get_ata_identify_cfast_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 69);
	return val & (1 << 15);
}

static inline bool ata_get_ata_identify_lps_misalignment_reporting_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 69);
	return val & (1 << 13);
}

static inline bool ata_get_ata_identify_supports_ncq_priority(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 76);
	return val & (1 << 12);
}

static inline bool ata_get_ata_identify_address_48bit_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 83);
	return val & (1 << 10);
}

static inline bool ata_get_ata_identify_set_max_set_password_dma_and_set_max_unlock_dma_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 69);
	return val & (1 << 9);
}

static inline bool ata_get_ata_identify_apm_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 83);
	return val & (1 << 3);
}

static inline bool ata_get_ata_identify_sata_dma_setup_auto_activation_enabled(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 79);
	return val & (1 << 2);
}

static inline bool ata_get_ata_identify_major_version_ata_atapi_7(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 80);
	return val & (1 << 7);
}

static inline bool ata_get_ata_identify_major_version_ata_atapi_6(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 80);
	return val & (1 << 6);
}

static inline bool ata_get_ata_identify_major_version_ata_atapi_5(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 80);
	return val & (1 << 5);
}

static inline bool ata_get_ata_identify_sct_error_recovery_control_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 206);
	return val & (1 << 3);
}

static inline bool ata_get_ata_identify_block_erase_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 59);
	return val & (1 << 15);
}

static inline bool ata_get_ata_identify_gpl_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 84);
	return val & (1 << 5);
}

static inline bool ata_get_ata_identify_dma_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 49);
	return val & (1 << 8);
}

static inline unsigned ata_get_ata_identify_rotational_rate(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 216);
	return (val >> 0) & ((1<<(15 - 0 + 1)) - 1);
}

static inline bool ata_get_ata_identify_not_ata_device(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 0);
	return val & (1 << 15);
}

static inline bool ata_get_ata_identify_spin_up_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 83);
	return val & (1 << 6);
}

static inline bool ata_get_ata_identify_read_buffer_dma_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 69);
	return val & (1 << 11);
}

static inline bool ata_get_ata_identify_address_28bit_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 69);
	return val & (1 << 6);
}

static inline bool ata_get_ata_identify_supports_sata_gen3_6gbps(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 76);
	return val & (1 << 3);
}

static inline bool ata_get_ata_identify_download_microcode_dma_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 69);
	return val & (1 << 8);
}

static inline bool ata_get_ata_identify_mandatory_power_management_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 82);
	return val & (1 << 3);
}

static inline bool ata_get_ata_identify_fields_valid_word_88(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 53);
	return val & (1 << 2);
}

static inline bool ata_get_ata_identify_packet_feature_set_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 82);
	return val & (1 << 4);
}

static inline bool ata_get_ata_identify_nop_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 82);
	return val & (1 << 14);
}

static inline bool ata_get_ata_identify_supports_ncq_queue_management_commands(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 77);
	return val & (1 << 5);
}

static inline bool ata_get_ata_identify_write_uncorrectable_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 119);
	return val & (1 << 2);
}

static inline bool ata_get_ata_identify_supports_sata_phy_event_counters_log(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 76);
	return val & (1 << 10);
}

static inline bool ata_get_ata_identify_supports_ncq(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 76);
	return val & (1 << 8);
}

static inline bool ata_get_ata_identify_supports_ncq_streaming(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 77);
	return val & (1 << 4);
}

static inline bool ata_get_ata_identify_sata_automatic_partial_to_slumber_transitions_enabled(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 79);
	return val & (1 << 7);
}

static inline void ata_get_ata_identify_current_media_serial(const unsigned char *buf, char *out) {
	ata_get_string(buf, 176, 205, out);
}

static inline bool ata_get_ata_identify_major_version_ata_8_acs(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 80);
	return val & (1 << 8);
}

static inline ata_longword_t ata_get_ata_identify_wwn_low(const unsigned char *buf) {
	return ata_get_longword(buf, 110);
}

static inline bool ata_get_ata_identify_trusted_computing_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 48);
	return val & (1 << 0);
}

static inline bool ata_get_ata_identify_sata_non_zero_buffer_offsets_enabled(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 79);
	return val & (1 << 1);
}

static inline bool ata_get_ata_identify_non_volatile_cache(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 69);
	return val & (1 << 2);
}

static inline bool ata_get_ata_identify_iordy_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 49);
	return val & (1 << 11);
}

static inline bool ata_get_ata_identify_sata_hardware_feature_control_enabled(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 79);
	return val & (1 << 5);
}

static inline bool ata_get_ata_identify_supports_unload_while_ncq_outstanding(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 76);
	return val & (1 << 11);
}

static inline bool ata_get_ata_identify_security_feature_set_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 82);
	return val & (1 << 1);
}

static inline bool ata_get_ata_identify_smart_error_logging_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 84);
	return val & (1 << 0);
}

static inline bool ata_get_ata_identify_sense_data_enabled(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 120);
	return val & (1 << 6);
}

static inline bool ata_get_ata_identify_iordy_disable_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 49);
	return val & (1 << 10);
}

static inline bool ata_get_ata_identify_sanitize_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 59);
	return val & (1 << 12);
}

static inline void ata_get_ata_identify_model(const unsigned char *buf, char *out) {
	ata_get_string(buf, 27, 46, out);
}

static inline bool ata_get_ata_identify_drat_supported(const unsigned char *buf) {
	ata_word_t val = ata_get_word(buf, 69);
	return val & (1 << 14);
}

static inline ata_longword_t ata_get_ata_identify_total_addressable_sectors_28bit(const unsigned char *buf) {
	return ata_get_longword(buf, 60);
}

#endif
