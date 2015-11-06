/* Generated file, do not edit */
#ifndef ATA_PARSE_H
#define ATA_PARSE_H
#include "ata.h"
static inline bool ata_get_ata_identify_smart_enabled(const char *buf) {
	ata_word_t val = ata_get_word(buf, 85);
	return val & (1 << 0);
}

static inline bool ata_get_ata_identify_cfast_supported(const char *buf) {
	ata_word_t val = ata_get_word(buf, 69);
	return val & (1 << 15);
}

static inline bool ata_get_ata_identify_sct_write_same_supported(const char *buf) {
	ata_word_t val = ata_get_word(buf, 206);
	return val & (1 << 2);
}

static inline bool ata_get_ata_identify_lps_misalignment_reporting_supported(const char *buf) {
	ata_word_t val = ata_get_word(buf, 69);
	return val & (1 << 13);
}

static inline bool ata_get_ata_identify_extended_number_of_user_addressable_sectors(const char *buf) {
	ata_word_t val = ata_get_word(buf, 69);
	return val & (1 << 3);
}

static inline bool ata_get_ata_identify_sense_data_supported(const char *buf) {
	ata_word_t val = ata_get_word(buf, 119);
	return val & (1 << 6);
}

static inline bool ata_get_ata_identify_fields_valid_words_64_70(const char *buf) {
	ata_word_t val = ata_get_word(buf, 53);
	return val & (1 << 1);
}

static inline bool ata_get_ata_identify_crypto_scramble_supported(const char *buf) {
	ata_word_t val = ata_get_word(buf, 59);
	return val & (1 << 13);
}

static inline bool ata_get_ata_identify_rzat_supported(const char *buf) {
	ata_word_t val = ata_get_word(buf, 69);
	return val & (1 << 5);
}

static inline bool ata_get_ata_identify_address_48bit_supported(const char *buf) {
	ata_word_t val = ata_get_word(buf, 83);
	return val & (1 << 10);
}

static inline bool ata_get_ata_identify_set_max_set_password_dma_and_set_max_unlock_dma_supported(const char *buf) {
	ata_word_t val = ata_get_word(buf, 69);
	return val & (1 << 9);
}

static inline bool ata_get_ata_identify_volatile_write_cache_supported(const char *buf) {
	ata_word_t val = ata_get_word(buf, 82);
	return val & (1 << 5);
}

static inline bool ata_get_ata_identify_standby_timer_values_settable(const char *buf) {
	ata_word_t val = ata_get_word(buf, 49);
	return val & (1 << 13);
}

static inline bool ata_get_ata_identify_sata_dma_setup_auto_activation_enabled(const char *buf) {
	ata_word_t val = ata_get_word(buf, 79);
	return val & (1 << 2);
}

static inline void ata_get_ata_identify_fw_rev(const char *buf, char *out) {
	ata_get_string(buf, 23, 26, out);
}

static inline bool ata_get_ata_identify_write_buffer_dma_supported(const char *buf) {
	ata_word_t val = ata_get_word(buf, 69);
	return val & (1 << 10);
}

static inline bool ata_get_ata_identify_major_version_ata_atapi_6(const char *buf) {
	ata_word_t val = ata_get_word(buf, 80);
	return val & (1 << 6);
}

static inline bool ata_get_ata_identify_major_version_ata_atapi_5(const char *buf) {
	ata_word_t val = ata_get_word(buf, 80);
	return val & (1 << 5);
}

static inline bool ata_get_ata_identify_sct_error_recovery_control_supported(const char *buf) {
	ata_word_t val = ata_get_word(buf, 206);
	return val & (1 << 3);
}

static inline bool ata_get_ata_identify_block_erase_supported(const char *buf) {
	ata_word_t val = ata_get_word(buf, 59);
	return val & (1 << 15);
}

static inline bool ata_get_ata_identify_sata_software_settings_preservation_enabled(const char *buf) {
	ata_word_t val = ata_get_word(buf, 79);
	return val & (1 << 6);
}

static inline bool ata_get_ata_identify_dma_supported(const char *buf) {
	ata_word_t val = ata_get_word(buf, 49);
	return val & (1 << 8);
}

static inline bool ata_get_ata_identify_write_uncorrectable_enabled(const char *buf) {
	ata_word_t val = ata_get_word(buf, 120);
	return val & (1 << 2);
}

static inline bool ata_get_ata_identify_sct_command_transport_supported(const char *buf) {
	ata_word_t val = ata_get_word(buf, 206);
	return val & (1 << 0);
}

static inline bool ata_get_ata_identify_not_ata_device(const char *buf) {
	ata_word_t val = ata_get_word(buf, 0);
	return val & (1 << 15);
}

static inline bool ata_get_ata_identify_read_buffer_supported(const char *buf) {
	ata_word_t val = ata_get_word(buf, 82);
	return val & (1 << 13);
}

static inline bool ata_get_ata_identify_read_buffer_dma_supported(const char *buf) {
	ata_word_t val = ata_get_word(buf, 69);
	return val & (1 << 11);
}

static inline bool ata_get_ata_identify_address_28bit_supported(const char *buf) {
	ata_word_t val = ata_get_word(buf, 69);
	return val & (1 << 6);
}

static inline bool ata_get_ata_identify_download_microcode_dma_supported(const char *buf) {
	ata_word_t val = ata_get_word(buf, 69);
	return val & (1 << 8);
}

static inline bool ata_get_ata_identify_mandatory_power_management_supported(const char *buf) {
	ata_word_t val = ata_get_word(buf, 82);
	return val & (1 << 3);
}

static inline bool ata_get_ata_identify_fields_valid_word_88(const char *buf) {
	ata_word_t val = ata_get_word(buf, 53);
	return val & (1 << 2);
}

static inline bool ata_get_ata_identify_packet_feature_set_supported(const char *buf) {
	ata_word_t val = ata_get_word(buf, 82);
	return val & (1 << 4);
}

static inline bool ata_get_ata_identify_smart_self_test_supported(const char *buf) {
	ata_word_t val = ata_get_word(buf, 84);
	return val & (1 << 1);
}

static inline bool ata_get_ata_identify_sata_in_order_data_delivery_enabled(const char *buf) {
	ata_word_t val = ata_get_word(buf, 79);
	return val & (1 << 4);
}

static inline bool ata_get_ata_identify_major_version_acs_2(const char *buf) {
	ata_word_t val = ata_get_word(buf, 80);
	return val & (1 << 9);
}

static inline bool ata_get_ata_identify_encrypt_all_supported(const char *buf) {
	ata_word_t val = ata_get_word(buf, 69);
	return val & (1 << 4);
}

static inline bool ata_get_ata_identify_iordy_supported(const char *buf) {
	ata_word_t val = ata_get_word(buf, 49);
	return val & (1 << 11);
}

static inline bool ata_get_ata_identify_nop_supported(const char *buf) {
	ata_word_t val = ata_get_word(buf, 82);
	return val & (1 << 14);
}

static inline bool ata_get_ata_identify_sata_device_initiated_power_management_enabled(const char *buf) {
	ata_word_t val = ata_get_word(buf, 79);
	return val & (1 << 3);
}

static inline bool ata_get_ata_identify_response_incomplete(const char *buf) {
	ata_word_t val = ata_get_word(buf, 0);
	return val & (1 << 2);
}

static inline bool ata_get_ata_identify_sata_automatic_partial_to_slumber_transitions_enabled(const char *buf) {
	ata_word_t val = ata_get_word(buf, 79);
	return val & (1 << 7);
}

static inline bool ata_get_ata_identify_sct_feature_control_supported(const char *buf) {
	ata_word_t val = ata_get_word(buf, 206);
	return val & (1 << 4);
}

static inline bool ata_get_ata_identify_major_version_ata_8_acs(const char *buf) {
	ata_word_t val = ata_get_word(buf, 80);
	return val & (1 << 8);
}

static inline bool ata_get_ata_identify_trusted_computing_supported(const char *buf) {
	ata_word_t val = ata_get_word(buf, 48);
	return val & (1 << 0);
}

static inline bool ata_get_ata_identify_sanitize_supported(const char *buf) {
	ata_word_t val = ata_get_word(buf, 59);
	return val & (1 << 12);
}

static inline bool ata_get_ata_identify_non_volatile_cache(const char *buf) {
	ata_word_t val = ata_get_word(buf, 69);
	return val & (1 << 2);
}

static inline void ata_get_ata_identify_serial_number(const char *buf, char *out) {
	ata_get_string(buf, 10, 19, out);
}

static inline ata_longword_t ata_get_ata_identify_total_addressable_sectors_28bit(const char *buf) {
	return ata_get_longword(buf, 60);
}

static inline void ata_get_ata_identify_model(const char *buf, char *out) {
	ata_get_string(buf, 27, 46, out);
}

static inline bool ata_get_ata_identify_overwrite_supported(const char *buf) {
	ata_word_t val = ata_get_word(buf, 59);
	return val & (1 << 14);
}

static inline bool ata_get_ata_identify_write_uncorrectable_supported(const char *buf) {
	ata_word_t val = ata_get_word(buf, 119);
	return val & (1 << 2);
}

static inline bool ata_get_ata_identify_smart_error_logging_supported(const char *buf) {
	ata_word_t val = ata_get_word(buf, 84);
	return val & (1 << 0);
}

static inline bool ata_get_ata_identify_sense_data_enabled(const char *buf) {
	ata_word_t val = ata_get_word(buf, 120);
	return val & (1 << 6);
}

static inline bool ata_get_ata_identify_smart_supported(const char *buf) {
	ata_word_t val = ata_get_word(buf, 82);
	return val & (1 << 0);
}

static inline bool ata_get_ata_identify_sct_data_tables_supported(const char *buf) {
	ata_word_t val = ata_get_word(buf, 206);
	return val & (1 << 5);
}

static inline bool ata_get_ata_identify_major_version_ata_atapi_7(const char *buf) {
	ata_word_t val = ata_get_word(buf, 80);
	return val & (1 << 7);
}

static inline bool ata_get_ata_identify_iordy_disable_supported(const char *buf) {
	ata_word_t val = ata_get_word(buf, 49);
	return val & (1 << 10);
}

static inline bool ata_get_ata_identify_sata_non_zero_buffer_offsets_enabled(const char *buf) {
	ata_word_t val = ata_get_word(buf, 79);
	return val & (1 << 1);
}

static inline bool ata_get_ata_identify_sata_hardware_feature_control_enabled(const char *buf) {
	ata_word_t val = ata_get_word(buf, 79);
	return val & (1 << 5);
}

static inline bool ata_get_ata_identify_write_buffer_supported(const char *buf) {
	ata_word_t val = ata_get_word(buf, 82);
	return val & (1 << 12);
}

static inline bool ata_get_ata_identify_drat_supported(const char *buf) {
	ata_word_t val = ata_get_word(buf, 69);
	return val & (1 << 14);
}

static inline bool ata_get_ata_identify_read_look_ahead_supported(const char *buf) {
	ata_word_t val = ata_get_word(buf, 82);
	return val & (1 << 6);
}

static inline bool ata_get_ata_identify_security_feature_set_supported(const char *buf) {
	ata_word_t val = ata_get_word(buf, 82);
	return val & (1 << 1);
}

#endif
