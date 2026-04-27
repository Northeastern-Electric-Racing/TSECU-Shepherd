#include "can_messages_rx.h"

void receive_ac_current_command(const can_msg_t *message, ac_current_command_t *ac_current_command) {
    
    uint16_t data_bigendian;
    memcpy(&data_bigendian, message->data, 2);
    uint16_t data = __builtin_bswap16(data_bigendian);
    uint64_t current_target_ac_mask = (1ULL << 16) - 1ULL;
    uint64_t current_target_ac_bits = (data >> 0) & current_target_ac_mask;
    int64_t current_target_ac_raw = (current_target_ac_bits & (1ULL << (16 - 1)))
        ? (int64_t)(current_target_ac_bits | ~current_target_ac_mask)
        : (int64_t)current_target_ac_bits;
    ac_current_command->current_target_ac = (float)(current_target_ac_raw / 10);
}

void receive_brake_current_command(const can_msg_t *message, brake_current_command_t *brake_current_command) {
    
    uint64_t data_bigendian;
    memcpy(&data_bigendian, message->data, 8);
    uint64_t data = __builtin_bswap64(data_bigendian);
    uint64_t brake_ac_current_mask = (1ULL << 16) - 1ULL;
    uint64_t brake_ac_current_bits = (data >> 48) & brake_ac_current_mask;
    int64_t brake_ac_current_raw = (brake_ac_current_bits & (1ULL << (16 - 1)))
        ? (int64_t)(brake_ac_current_bits | ~brake_ac_current_mask)
        : (int64_t)brake_ac_current_bits;
    brake_current_command->brake_ac_current = (float)(brake_ac_current_raw / 10);
}

void receive_drive_enable_command(const can_msg_t *message, drive_enable_command_t *drive_enable_command) {
    
    uint8_t data = message->data[0];
    uint64_t drive_enable_mask = (1ULL << 8) - 1ULL;
    uint64_t drive_enable_raw = (data >> 0) & drive_enable_mask;
    drive_enable_command->drive_enable = (uint8_t)drive_enable_raw;
}

void receive_shepherd_bms_fan_percent(const can_msg_t *message, shepherd_bms_fan_percent_t *shepherd_bms_fan_percent) {
    
    uint8_t data = message->data[0];
    uint64_t pwm_duty_mask = (1ULL << 8) - 1ULL;
    uint64_t pwm_duty_raw = (data >> 0) & pwm_duty_mask;
    shepherd_bms_fan_percent->pwm_duty = (uint8_t)pwm_duty_raw;
}

void receive_dashboard_efuse_state(const can_msg_t *message, dashboard_efuse_state_t *dashboard_efuse_state) {
    
    uint8_t data = message->data[0];
    uint64_t state_mask = (1ULL << 8) - 1ULL;
    uint64_t state_raw = (data >> 0) & state_mask;
    dashboard_efuse_state->state = (uint8_t)state_raw;
}

void receive_brake_efuse_state(const can_msg_t *message, brake_efuse_state_t *brake_efuse_state) {
    
    uint8_t data = message->data[0];
    uint64_t state_mask = (1ULL << 8) - 1ULL;
    uint64_t state_raw = (data >> 0) & state_mask;
    brake_efuse_state->state = (uint8_t)state_raw;
}

void receive_shutdown_efuse_state(const can_msg_t *message, shutdown_efuse_state_t *shutdown_efuse_state) {
    
    uint8_t data = message->data[0];
    uint64_t state_mask = (1ULL << 8) - 1ULL;
    uint64_t state_raw = (data >> 0) & state_mask;
    shutdown_efuse_state->state = (uint8_t)state_raw;
}

void receive_lv_efuse_state(const can_msg_t *message, lv_efuse_state_t *lv_efuse_state) {
    
    uint8_t data = message->data[0];
    uint64_t state_mask = (1ULL << 8) - 1ULL;
    uint64_t state_raw = (data >> 0) & state_mask;
    lv_efuse_state->state = (uint8_t)state_raw;
}

void receive_radfan_efuse_state(const can_msg_t *message, radfan_efuse_state_t *radfan_efuse_state) {
    
    uint8_t data = message->data[0];
    uint64_t state_mask = (1ULL << 8) - 1ULL;
    uint64_t state_raw = (data >> 0) & state_mask;
    radfan_efuse_state->state = (uint8_t)state_raw;
}

void receive_fanbatt_efuse_state(const can_msg_t *message, fanbatt_efuse_state_t *fanbatt_efuse_state) {
    
    uint8_t data = message->data[0];
    uint64_t state_mask = (1ULL << 8) - 1ULL;
    uint64_t state_raw = (data >> 0) & state_mask;
    fanbatt_efuse_state->state = (uint8_t)state_raw;
}

void receive_pumpone_efuse_state(const can_msg_t *message, pumpone_efuse_state_t *pumpone_efuse_state) {
    
    uint8_t data = message->data[0];
    uint64_t state_mask = (1ULL << 8) - 1ULL;
    uint64_t state_raw = (data >> 0) & state_mask;
    pumpone_efuse_state->state = (uint8_t)state_raw;
}

void receive_pumptwo_efuse_state(const can_msg_t *message, pumptwo_efuse_state_t *pumptwo_efuse_state) {
    
    uint8_t data = message->data[0];
    uint64_t state_mask = (1ULL << 8) - 1ULL;
    uint64_t state_raw = (data >> 0) & state_mask;
    pumptwo_efuse_state->state = (uint8_t)state_raw;
}

void receive_battbox_efuse_state(const can_msg_t *message, battbox_efuse_state_t *battbox_efuse_state) {
    
    uint8_t data = message->data[0];
    uint64_t state_mask = (1ULL << 8) - 1ULL;
    uint64_t state_raw = (data >> 0) & state_mask;
    battbox_efuse_state->state = (uint8_t)state_raw;
}

void receive_mc_efuse_state(const can_msg_t *message, mc_efuse_state_t *mc_efuse_state) {
    
    uint8_t data = message->data[0];
    uint64_t state_mask = (1ULL << 8) - 1ULL;
    uint64_t state_raw = (data >> 0) & state_mask;
    mc_efuse_state->state = (uint8_t)state_raw;
}

void receive_spare_efuse_state(const can_msg_t *message, spare_efuse_state_t *spare_efuse_state) {
    
    uint8_t data = message->data[0];
    uint64_t state_mask = (1ULL << 8) - 1ULL;
    uint64_t state_raw = (data >> 0) & state_mask;
    spare_efuse_state->state = (uint8_t)state_raw;
}

void receive_rtds_command_message(const can_msg_t *message, rtds_command_message_t *rtds_command_message) {
    
    uint8_t data = message->data[0];
    uint64_t command_mask = (1ULL << 8) - 1ULL;
    uint64_t command_raw = (data >> 0) & command_mask;
    rtds_command_message->command = (uint8_t)command_raw;
}

void receive_dashboard_efuse(const can_msg_t *message, dashboard_efuse_t *dashboard_efuse) {
    
    uint64_t data_bigendian;
    memcpy(&data_bigendian, message->data, 8);
    uint64_t data = __builtin_bswap64(data_bigendian);
    uint64_t ADC_mask = (1ULL << 16) - 1ULL;
    uint64_t ADC_raw = (data >> 48) & ADC_mask;
    dashboard_efuse->ADC = (uint16_t)ADC_raw;
    uint64_t voltage_mask = (1ULL << 16) - 1ULL;
    uint64_t voltage_raw = (data >> 32) & voltage_mask;
    dashboard_efuse->voltage = (float)(voltage_raw / 1000);
    uint64_t current_mask = (1ULL << 16) - 1ULL;
    uint64_t current_raw = (data >> 16) & current_mask;
    dashboard_efuse->current = (float)(current_raw / 1000);
    uint64_t is_faulted_mask = (1ULL << 4) - 1ULL;
    uint64_t is_faulted_raw = (data >> 12) & is_faulted_mask;
    dashboard_efuse->is_faulted = (bool)is_faulted_raw;
    uint64_t is_enabled_mask = (1ULL << 4) - 1ULL;
    uint64_t is_enabled_raw = (data >> 8) & is_enabled_mask;
    dashboard_efuse->is_enabled = (bool)is_enabled_raw;
    uint64_t control_state_mask = (1ULL << 8) - 1ULL;
    uint64_t control_state_raw = (data >> 0) & control_state_mask;
    dashboard_efuse->control_state = (uint8_t)control_state_raw;
}

void receive_brake_efuse(const can_msg_t *message, brake_efuse_t *brake_efuse) {
    
    uint64_t data_bigendian;
    memcpy(&data_bigendian, message->data, 8);
    uint64_t data = __builtin_bswap64(data_bigendian);
    uint64_t ADC_mask = (1ULL << 16) - 1ULL;
    uint64_t ADC_raw = (data >> 48) & ADC_mask;
    brake_efuse->ADC = (uint16_t)ADC_raw;
    uint64_t voltage_mask = (1ULL << 16) - 1ULL;
    uint64_t voltage_raw = (data >> 32) & voltage_mask;
    brake_efuse->voltage = (float)(voltage_raw / 1000);
    uint64_t current_mask = (1ULL << 16) - 1ULL;
    uint64_t current_raw = (data >> 16) & current_mask;
    brake_efuse->current = (float)(current_raw / 1000);
    uint64_t is_faulted_mask = (1ULL << 4) - 1ULL;
    uint64_t is_faulted_raw = (data >> 12) & is_faulted_mask;
    brake_efuse->is_faulted = (bool)is_faulted_raw;
    uint64_t is_enabled_mask = (1ULL << 4) - 1ULL;
    uint64_t is_enabled_raw = (data >> 8) & is_enabled_mask;
    brake_efuse->is_enabled = (bool)is_enabled_raw;
    uint64_t control_state_mask = (1ULL << 8) - 1ULL;
    uint64_t control_state_raw = (data >> 0) & control_state_mask;
    brake_efuse->control_state = (uint8_t)control_state_raw;
}

void receive_shutdown_efuse(const can_msg_t *message, shutdown_efuse_t *shutdown_efuse) {
    
    uint64_t data_bigendian;
    memcpy(&data_bigendian, message->data, 8);
    uint64_t data = __builtin_bswap64(data_bigendian);
    uint64_t ADC_mask = (1ULL << 16) - 1ULL;
    uint64_t ADC_raw = (data >> 48) & ADC_mask;
    shutdown_efuse->ADC = (uint16_t)ADC_raw;
    uint64_t voltage_mask = (1ULL << 16) - 1ULL;
    uint64_t voltage_raw = (data >> 32) & voltage_mask;
    shutdown_efuse->voltage = (float)(voltage_raw / 1000);
    uint64_t current_mask = (1ULL << 16) - 1ULL;
    uint64_t current_raw = (data >> 16) & current_mask;
    shutdown_efuse->current = (float)(current_raw / 1000);
    uint64_t is_faulted_mask = (1ULL << 4) - 1ULL;
    uint64_t is_faulted_raw = (data >> 12) & is_faulted_mask;
    shutdown_efuse->is_faulted = (bool)is_faulted_raw;
    uint64_t is_enabled_mask = (1ULL << 4) - 1ULL;
    uint64_t is_enabled_raw = (data >> 8) & is_enabled_mask;
    shutdown_efuse->is_enabled = (bool)is_enabled_raw;
    uint64_t control_state_mask = (1ULL << 8) - 1ULL;
    uint64_t control_state_raw = (data >> 0) & control_state_mask;
    shutdown_efuse->control_state = (uint8_t)control_state_raw;
}

void receive_lv_efuse(const can_msg_t *message, lv_efuse_t *lv_efuse) {
    
    uint64_t data_bigendian;
    memcpy(&data_bigendian, message->data, 8);
    uint64_t data = __builtin_bswap64(data_bigendian);
    uint64_t ADC_mask = (1ULL << 16) - 1ULL;
    uint64_t ADC_raw = (data >> 48) & ADC_mask;
    lv_efuse->ADC = (uint16_t)ADC_raw;
    uint64_t voltage_mask = (1ULL << 16) - 1ULL;
    uint64_t voltage_raw = (data >> 32) & voltage_mask;
    lv_efuse->voltage = (float)(voltage_raw / 1000);
    uint64_t current_mask = (1ULL << 16) - 1ULL;
    uint64_t current_raw = (data >> 16) & current_mask;
    lv_efuse->current = (float)(current_raw / 1000);
    uint64_t is_faulted_mask = (1ULL << 4) - 1ULL;
    uint64_t is_faulted_raw = (data >> 12) & is_faulted_mask;
    lv_efuse->is_faulted = (bool)is_faulted_raw;
    uint64_t is_enabled_mask = (1ULL << 4) - 1ULL;
    uint64_t is_enabled_raw = (data >> 8) & is_enabled_mask;
    lv_efuse->is_enabled = (bool)is_enabled_raw;
    uint64_t control_state_mask = (1ULL << 8) - 1ULL;
    uint64_t control_state_raw = (data >> 0) & control_state_mask;
    lv_efuse->control_state = (uint8_t)control_state_raw;
}

void receive_radfan_efuse(const can_msg_t *message, radfan_efuse_t *radfan_efuse) {
    
    uint64_t data_bigendian;
    memcpy(&data_bigendian, message->data, 8);
    uint64_t data = __builtin_bswap64(data_bigendian);
    uint64_t ADC_mask = (1ULL << 16) - 1ULL;
    uint64_t ADC_raw = (data >> 48) & ADC_mask;
    radfan_efuse->ADC = (uint16_t)ADC_raw;
    uint64_t voltage_mask = (1ULL << 16) - 1ULL;
    uint64_t voltage_raw = (data >> 32) & voltage_mask;
    radfan_efuse->voltage = (float)(voltage_raw / 1000);
    uint64_t current_mask = (1ULL << 16) - 1ULL;
    uint64_t current_raw = (data >> 16) & current_mask;
    radfan_efuse->current = (float)(current_raw / 1000);
    uint64_t is_faulted_mask = (1ULL << 4) - 1ULL;
    uint64_t is_faulted_raw = (data >> 12) & is_faulted_mask;
    radfan_efuse->is_faulted = (bool)is_faulted_raw;
    uint64_t is_enabled_mask = (1ULL << 4) - 1ULL;
    uint64_t is_enabled_raw = (data >> 8) & is_enabled_mask;
    radfan_efuse->is_enabled = (bool)is_enabled_raw;
    uint64_t control_state_mask = (1ULL << 8) - 1ULL;
    uint64_t control_state_raw = (data >> 0) & control_state_mask;
    radfan_efuse->control_state = (uint8_t)control_state_raw;
}

void receive_fanbatt_efuse(const can_msg_t *message, fanbatt_efuse_t *fanbatt_efuse) {
    
    uint64_t data_bigendian;
    memcpy(&data_bigendian, message->data, 8);
    uint64_t data = __builtin_bswap64(data_bigendian);
    uint64_t ADC_mask = (1ULL << 16) - 1ULL;
    uint64_t ADC_raw = (data >> 48) & ADC_mask;
    fanbatt_efuse->ADC = (uint16_t)ADC_raw;
    uint64_t voltage_mask = (1ULL << 16) - 1ULL;
    uint64_t voltage_raw = (data >> 32) & voltage_mask;
    fanbatt_efuse->voltage = (float)(voltage_raw / 1000);
    uint64_t current_mask = (1ULL << 16) - 1ULL;
    uint64_t current_raw = (data >> 16) & current_mask;
    fanbatt_efuse->current = (float)(current_raw / 1000);
    uint64_t is_faulted_mask = (1ULL << 4) - 1ULL;
    uint64_t is_faulted_raw = (data >> 12) & is_faulted_mask;
    fanbatt_efuse->is_faulted = (bool)is_faulted_raw;
    uint64_t is_enabled_mask = (1ULL << 4) - 1ULL;
    uint64_t is_enabled_raw = (data >> 8) & is_enabled_mask;
    fanbatt_efuse->is_enabled = (bool)is_enabled_raw;
    uint64_t control_state_mask = (1ULL << 8) - 1ULL;
    uint64_t control_state_raw = (data >> 0) & control_state_mask;
    fanbatt_efuse->control_state = (uint8_t)control_state_raw;
}

void receive_pumpone_efuse(const can_msg_t *message, pumpone_efuse_t *pumpone_efuse) {
    
    uint64_t data_bigendian;
    memcpy(&data_bigendian, message->data, 8);
    uint64_t data = __builtin_bswap64(data_bigendian);
    uint64_t ADC_mask = (1ULL << 16) - 1ULL;
    uint64_t ADC_raw = (data >> 48) & ADC_mask;
    pumpone_efuse->ADC = (uint16_t)ADC_raw;
    uint64_t voltage_mask = (1ULL << 16) - 1ULL;
    uint64_t voltage_raw = (data >> 32) & voltage_mask;
    pumpone_efuse->voltage = (float)(voltage_raw / 1000);
    uint64_t current_mask = (1ULL << 16) - 1ULL;
    uint64_t current_raw = (data >> 16) & current_mask;
    pumpone_efuse->current = (float)(current_raw / 1000);
    uint64_t is_faulted_mask = (1ULL << 4) - 1ULL;
    uint64_t is_faulted_raw = (data >> 12) & is_faulted_mask;
    pumpone_efuse->is_faulted = (bool)is_faulted_raw;
    uint64_t is_enabled_mask = (1ULL << 4) - 1ULL;
    uint64_t is_enabled_raw = (data >> 8) & is_enabled_mask;
    pumpone_efuse->is_enabled = (bool)is_enabled_raw;
    uint64_t control_state_mask = (1ULL << 8) - 1ULL;
    uint64_t control_state_raw = (data >> 0) & control_state_mask;
    pumpone_efuse->control_state = (uint8_t)control_state_raw;
}

void receive_pumptwo_efuse(const can_msg_t *message, pumptwo_efuse_t *pumptwo_efuse) {
    
    uint64_t data_bigendian;
    memcpy(&data_bigendian, message->data, 8);
    uint64_t data = __builtin_bswap64(data_bigendian);
    uint64_t ADC_mask = (1ULL << 16) - 1ULL;
    uint64_t ADC_raw = (data >> 48) & ADC_mask;
    pumptwo_efuse->ADC = (uint16_t)ADC_raw;
    uint64_t voltage_mask = (1ULL << 16) - 1ULL;
    uint64_t voltage_raw = (data >> 32) & voltage_mask;
    pumptwo_efuse->voltage = (float)(voltage_raw / 1000);
    uint64_t current_mask = (1ULL << 16) - 1ULL;
    uint64_t current_raw = (data >> 16) & current_mask;
    pumptwo_efuse->current = (float)(current_raw / 1000);
    uint64_t is_faulted_mask = (1ULL << 4) - 1ULL;
    uint64_t is_faulted_raw = (data >> 12) & is_faulted_mask;
    pumptwo_efuse->is_faulted = (bool)is_faulted_raw;
    uint64_t is_enabled_mask = (1ULL << 4) - 1ULL;
    uint64_t is_enabled_raw = (data >> 8) & is_enabled_mask;
    pumptwo_efuse->is_enabled = (bool)is_enabled_raw;
    uint64_t control_state_mask = (1ULL << 8) - 1ULL;
    uint64_t control_state_raw = (data >> 0) & control_state_mask;
    pumptwo_efuse->control_state = (uint8_t)control_state_raw;
}

void receive_battbox_efuse(const can_msg_t *message, battbox_efuse_t *battbox_efuse) {
    
    uint64_t data_bigendian;
    memcpy(&data_bigendian, message->data, 8);
    uint64_t data = __builtin_bswap64(data_bigendian);
    uint64_t ADC_mask = (1ULL << 16) - 1ULL;
    uint64_t ADC_raw = (data >> 48) & ADC_mask;
    battbox_efuse->ADC = (uint16_t)ADC_raw;
    uint64_t voltage_mask = (1ULL << 16) - 1ULL;
    uint64_t voltage_raw = (data >> 32) & voltage_mask;
    battbox_efuse->voltage = (float)(voltage_raw / 1000);
    uint64_t current_mask = (1ULL << 16) - 1ULL;
    uint64_t current_raw = (data >> 16) & current_mask;
    battbox_efuse->current = (float)(current_raw / 1000);
    uint64_t is_faulted_mask = (1ULL << 4) - 1ULL;
    uint64_t is_faulted_raw = (data >> 12) & is_faulted_mask;
    battbox_efuse->is_faulted = (bool)is_faulted_raw;
    uint64_t is_enabled_mask = (1ULL << 4) - 1ULL;
    uint64_t is_enabled_raw = (data >> 8) & is_enabled_mask;
    battbox_efuse->is_enabled = (bool)is_enabled_raw;
    uint64_t control_state_mask = (1ULL << 8) - 1ULL;
    uint64_t control_state_raw = (data >> 0) & control_state_mask;
    battbox_efuse->control_state = (uint8_t)control_state_raw;
}

void receive_mc_efuse(const can_msg_t *message, mc_efuse_t *mc_efuse) {
    
    uint64_t data_bigendian;
    memcpy(&data_bigendian, message->data, 8);
    uint64_t data = __builtin_bswap64(data_bigendian);
    uint64_t ADC_mask = (1ULL << 16) - 1ULL;
    uint64_t ADC_raw = (data >> 48) & ADC_mask;
    mc_efuse->ADC = (uint16_t)ADC_raw;
    uint64_t voltage_mask = (1ULL << 16) - 1ULL;
    uint64_t voltage_raw = (data >> 32) & voltage_mask;
    mc_efuse->voltage = (float)(voltage_raw / 1000);
    uint64_t current_mask = (1ULL << 16) - 1ULL;
    uint64_t current_raw = (data >> 16) & current_mask;
    mc_efuse->current = (float)(current_raw / 1000);
    uint64_t is_faulted_mask = (1ULL << 4) - 1ULL;
    uint64_t is_faulted_raw = (data >> 12) & is_faulted_mask;
    mc_efuse->is_faulted = (bool)is_faulted_raw;
    uint64_t is_enabled_mask = (1ULL << 4) - 1ULL;
    uint64_t is_enabled_raw = (data >> 8) & is_enabled_mask;
    mc_efuse->is_enabled = (bool)is_enabled_raw;
    uint64_t control_state_mask = (1ULL << 8) - 1ULL;
    uint64_t control_state_raw = (data >> 0) & control_state_mask;
    mc_efuse->control_state = (uint8_t)control_state_raw;
}

void receive_spare_efuse(const can_msg_t *message, spare_efuse_t *spare_efuse) {
    
    uint64_t data_bigendian;
    memcpy(&data_bigendian, message->data, 8);
    uint64_t data = __builtin_bswap64(data_bigendian);
    uint64_t ADC_mask = (1ULL << 16) - 1ULL;
    uint64_t ADC_raw = (data >> 48) & ADC_mask;
    spare_efuse->ADC = (uint16_t)ADC_raw;
    uint64_t voltage_mask = (1ULL << 16) - 1ULL;
    uint64_t voltage_raw = (data >> 32) & voltage_mask;
    spare_efuse->voltage = (float)(voltage_raw / 1000);
    uint64_t current_mask = (1ULL << 16) - 1ULL;
    uint64_t current_raw = (data >> 16) & current_mask;
    spare_efuse->current = (float)(current_raw / 1000);
    uint64_t is_faulted_mask = (1ULL << 4) - 1ULL;
    uint64_t is_faulted_raw = (data >> 12) & is_faulted_mask;
    spare_efuse->is_faulted = (bool)is_faulted_raw;
    uint64_t is_enabled_mask = (1ULL << 4) - 1ULL;
    uint64_t is_enabled_raw = (data >> 8) & is_enabled_mask;
    spare_efuse->is_enabled = (bool)is_enabled_raw;
    uint64_t control_state_mask = (1ULL << 8) - 1ULL;
    uint64_t control_state_raw = (data >> 0) & control_state_mask;
    spare_efuse->control_state = (uint8_t)control_state_raw;
}

void receive_shutdown_pins(const can_msg_t *message, shutdown_pins_t *shutdown_pins) {
    
    uint16_t data_bigendian;
    memcpy(&data_bigendian, message->data, 2);
    uint16_t data = __builtin_bswap16(data_bigendian);
    uint64_t bms_gpio_mask = (1ULL << 1) - 1ULL;
    uint64_t bms_gpio_raw = (data >> 15) & bms_gpio_mask;
    shutdown_pins->bms_gpio = (bool)bms_gpio_raw;
    uint64_t bots_gpio_mask = (1ULL << 1) - 1ULL;
    uint64_t bots_gpio_raw = (data >> 14) & bots_gpio_mask;
    shutdown_pins->bots_gpio = (bool)bots_gpio_raw;
    uint64_t spare_gpio_mask = (1ULL << 1) - 1ULL;
    uint64_t spare_gpio_raw = (data >> 13) & spare_gpio_mask;
    shutdown_pins->spare_gpio = (bool)spare_gpio_raw;
    uint64_t bspd_gpio_mask = (1ULL << 1) - 1ULL;
    uint64_t bspd_gpio_raw = (data >> 12) & bspd_gpio_mask;
    shutdown_pins->bspd_gpio = (bool)bspd_gpio_raw;
    uint64_t hv_c_mask = (1ULL << 1) - 1ULL;
    uint64_t hv_c_raw = (data >> 11) & hv_c_mask;
    shutdown_pins->hv_c = (bool)hv_c_raw;
    uint64_t hvd_gpio_mask = (1ULL << 1) - 1ULL;
    uint64_t hvd_gpio_raw = (data >> 10) & hvd_gpio_mask;
    shutdown_pins->hvd_gpio = (bool)hvd_gpio_raw;
    uint64_t imd_gpio_mask = (1ULL << 1) - 1ULL;
    uint64_t imd_gpio_raw = (data >> 9) & imd_gpio_mask;
    shutdown_pins->imd_gpio = (bool)imd_gpio_raw;
    uint64_t ckpt_gpio_mask = (1ULL << 1) - 1ULL;
    uint64_t ckpt_gpio_raw = (data >> 8) & ckpt_gpio_mask;
    shutdown_pins->ckpt_gpio = (bool)ckpt_gpio_raw;
    uint64_t inertia_sw_gpio_mask = (1ULL << 1) - 1ULL;
    uint64_t inertia_sw_gpio_raw = (data >> 7) & inertia_sw_gpio_mask;
    shutdown_pins->inertia_sw_gpio = (bool)inertia_sw_gpio_raw;
    uint64_t tsms_gpio_mask = (1ULL << 1) - 1ULL;
    uint64_t tsms_gpio_raw = (data >> 6) & tsms_gpio_mask;
    shutdown_pins->tsms_gpio = (bool)tsms_gpio_raw;
}

void receive_car_state(const can_msg_t *message, car_state_t *car_state) {
    
    uint64_t data_bigendian;
    memcpy(&data_bigendian, message->data, 8);
    uint64_t data = __builtin_bswap64(data_bigendian);
    uint64_t home_mode_mask = (1ULL << 4) - 1ULL;
    uint64_t home_mode_raw = (data >> 60) & home_mode_mask;
    car_state->home_mode = (bool)home_mode_raw;
    uint64_t nero_index_mask = (1ULL << 4) - 1ULL;
    uint64_t nero_index_raw = (data >> 56) & nero_index_mask;
    car_state->nero_index = (uint8_t)nero_index_raw;
    uint64_t car_speed_mask = (1ULL << 16) - 1ULL;
    uint64_t car_speed_bits = (data >> 40) & car_speed_mask;
    int64_t car_speed_raw = (car_speed_bits & (1ULL << (16 - 1)))
        ? (int64_t)(car_speed_bits | ~car_speed_mask)
        : (int64_t)car_speed_bits;
    car_state->car_speed = (float)(car_speed_raw / 10);
    uint64_t tsms_mask = (1ULL << 1) - 1ULL;
    uint64_t tsms_raw = (data >> 39) & tsms_mask;
    car_state->tsms = (bool)tsms_raw;
    uint64_t torque_limit_percentage_mask = (1ULL << 7) - 1ULL;
    uint64_t torque_limit_percentage_raw = (data >> 32) & torque_limit_percentage_mask;
    car_state->torque_limit_percentage = (float)(torque_limit_percentage_raw / 100);
    uint64_t reverse_mask = (1ULL << 1) - 1ULL;
    uint64_t reverse_raw = (data >> 31) & reverse_mask;
    car_state->reverse = (bool)reverse_raw;
    uint64_t regen_limit_mask = (1ULL << 10) - 1ULL;
    uint64_t regen_limit_raw = (data >> 21) & regen_limit_mask;
    car_state->regen_limit = (uint16_t)regen_limit_raw;
    uint64_t launch_control_mask = (1ULL << 1) - 1ULL;
    uint64_t launch_control_raw = (data >> 20) & launch_control_mask;
    car_state->launch_control = (bool)launch_control_raw;
    uint64_t functional_state_mask = (1ULL << 3) - 1ULL;
    uint64_t functional_state_raw = (data >> 17) & functional_state_mask;
    car_state->functional_state = (uint8_t)functional_state_raw;
    uint64_t traction_control_mask = (1ULL << 1) - 1ULL;
    uint64_t traction_control_raw = (data >> 16) & traction_control_mask;
    car_state->traction_control = (bool)traction_control_raw;
}

void receive_pedal_percent_pressed_values(const can_msg_t *message, pedal_percent_pressed_values_t *pedal_percent_pressed_values) {
    
    uint64_t data_bigendian;
    memcpy(&data_bigendian, message->data, 8);
    uint64_t data = __builtin_bswap64(data_bigendian);
    uint64_t accel_norm_mask = (1ULL << 16) - 1ULL;
    uint64_t accel_norm_raw = (data >> 48) & accel_norm_mask;
    pedal_percent_pressed_values->accel_norm = (float)(accel_norm_raw / 100);
    uint64_t brake_norm_mask = (1ULL << 16) - 1ULL;
    uint64_t brake_norm_raw = (data >> 32) & brake_norm_mask;
    pedal_percent_pressed_values->brake_norm = (float)(brake_norm_raw / 100);
    uint64_t brake_psi_brake1_mask = (1ULL << 16) - 1ULL;
    uint64_t brake_psi_brake1_bits = (data >> 16) & brake_psi_brake1_mask;
    int64_t brake_psi_brake1_raw = (brake_psi_brake1_bits & (1ULL << (16 - 1)))
        ? (int64_t)(brake_psi_brake1_bits | ~brake_psi_brake1_mask)
        : (int64_t)brake_psi_brake1_bits;
    pedal_percent_pressed_values->brake_psi_brake1 = (float)(brake_psi_brake1_raw / 10);
    uint64_t brake_psi_brake2_mask = (1ULL << 16) - 1ULL;
    uint64_t brake_psi_brake2_bits = (data >> 0) & brake_psi_brake2_mask;
    int64_t brake_psi_brake2_raw = (brake_psi_brake2_bits & (1ULL << (16 - 1)))
        ? (int64_t)(brake_psi_brake2_bits | ~brake_psi_brake2_mask)
        : (int64_t)brake_psi_brake2_bits;
    pedal_percent_pressed_values->brake_psi_brake2 = (float)(brake_psi_brake2_raw / 10);
}

void receive_pedal_sensor_voltages(const can_msg_t *message, pedal_sensor_voltages_t *pedal_sensor_voltages) {
    
    uint64_t data_bigendian;
    memcpy(&data_bigendian, message->data, 8);
    uint64_t data = __builtin_bswap64(data_bigendian);
    uint64_t accel1_volts_mask = (1ULL << 16) - 1ULL;
    uint64_t accel1_volts_raw = (data >> 48) & accel1_volts_mask;
    pedal_sensor_voltages->accel1_volts = (float)(accel1_volts_raw / 100);
    uint64_t accel2_volts_mask = (1ULL << 16) - 1ULL;
    uint64_t accel2_volts_raw = (data >> 32) & accel2_volts_mask;
    pedal_sensor_voltages->accel2_volts = (float)(accel2_volts_raw / 100);
    uint64_t brake1_volts_mask = (1ULL << 16) - 1ULL;
    uint64_t brake1_volts_raw = (data >> 16) & brake1_volts_mask;
    pedal_sensor_voltages->brake1_volts = (float)(brake1_volts_raw / 100);
    uint64_t brake2_volts_mask = (1ULL << 16) - 1ULL;
    uint64_t brake2_volts_raw = (data >> 0) & brake2_volts_mask;
    pedal_sensor_voltages->brake2_volts = (float)(brake2_volts_raw / 100);
}

void receive_lightning_board_light_status(const can_msg_t *message, lightning_board_light_status_t *lightning_board_light_status) {
    
    uint8_t data = message->data[0];
    uint64_t status_mask = (1ULL << 2) - 1ULL;
    uint64_t status_raw = (data >> 6) & status_mask;
    lightning_board_light_status->status = (uint8_t)status_raw;
}

void receive_temperature_sensor(const can_msg_t *message, temperature_sensor_t *temperature_sensor) {
    
    uint32_t data_bigendian;
    memcpy(&data_bigendian, message->data, 4);
    uint32_t data = __builtin_bswap32(data_bigendian);
    uint64_t vcu_temperature_mask = (1ULL << 16) - 1ULL;
    uint64_t vcu_temperature_bits = (data >> 16) & vcu_temperature_mask;
    int64_t vcu_temperature_raw = (vcu_temperature_bits & (1ULL << (16 - 1)))
        ? (int64_t)(vcu_temperature_bits | ~vcu_temperature_mask)
        : (int64_t)vcu_temperature_bits;
    temperature_sensor->vcu_temperature = (float)(vcu_temperature_raw / 100);
    uint64_t vcu_humidity_mask = (1ULL << 16) - 1ULL;
    uint64_t vcu_humidity_raw = (data >> 0) & vcu_humidity_mask;
    temperature_sensor->vcu_humidity = (float)(vcu_humidity_raw / 100);
}

void receive_imu_accelerometer(const can_msg_t *message, imu_accelerometer_t *imu_accelerometer) {
    
    uint64_t data_bigendian;
    memcpy(&data_bigendian, message->data, 8);
    uint64_t data = __builtin_bswap64(data_bigendian);
    uint64_t imu_accelerometer_x_mask = (1ULL << 16) - 1ULL;
    uint64_t imu_accelerometer_x_bits = (data >> 48) & imu_accelerometer_x_mask;
    int64_t imu_accelerometer_x_raw = (imu_accelerometer_x_bits & (1ULL << (16 - 1)))
        ? (int64_t)(imu_accelerometer_x_bits | ~imu_accelerometer_x_mask)
        : (int64_t)imu_accelerometer_x_bits;
    imu_accelerometer->imu_accelerometer_x = (float)(imu_accelerometer_x_raw / 4);
    uint64_t imu_accelerometer_y_mask = (1ULL << 16) - 1ULL;
    uint64_t imu_accelerometer_y_bits = (data >> 32) & imu_accelerometer_y_mask;
    int64_t imu_accelerometer_y_raw = (imu_accelerometer_y_bits & (1ULL << (16 - 1)))
        ? (int64_t)(imu_accelerometer_y_bits | ~imu_accelerometer_y_mask)
        : (int64_t)imu_accelerometer_y_bits;
    imu_accelerometer->imu_accelerometer_y = (float)(imu_accelerometer_y_raw / 4);
    uint64_t imu_accelerometer_z_mask = (1ULL << 16) - 1ULL;
    uint64_t imu_accelerometer_z_bits = (data >> 16) & imu_accelerometer_z_mask;
    int64_t imu_accelerometer_z_raw = (imu_accelerometer_z_bits & (1ULL << (16 - 1)))
        ? (int64_t)(imu_accelerometer_z_bits | ~imu_accelerometer_z_mask)
        : (int64_t)imu_accelerometer_z_bits;
    imu_accelerometer->imu_accelerometer_z = (float)(imu_accelerometer_z_raw / 4);
}

void receive_imu_gyro(const can_msg_t *message, imu_gyro_t *imu_gyro) {
    
    uint64_t data_bigendian;
    memcpy(&data_bigendian, message->data, 8);
    uint64_t data = __builtin_bswap64(data_bigendian);
    uint64_t imu_gyro_x_mask = (1ULL << 16) - 1ULL;
    uint64_t imu_gyro_x_bits = (data >> 48) & imu_gyro_x_mask;
    int64_t imu_gyro_x_raw = (imu_gyro_x_bits & (1ULL << (16 - 1)))
        ? (int64_t)(imu_gyro_x_bits | ~imu_gyro_x_mask)
        : (int64_t)imu_gyro_x_bits;
    imu_gyro->imu_gyro_x = (float)(imu_gyro_x_raw / 4);
    uint64_t imu_gyro_y_mask = (1ULL << 16) - 1ULL;
    uint64_t imu_gyro_y_bits = (data >> 32) & imu_gyro_y_mask;
    int64_t imu_gyro_y_raw = (imu_gyro_y_bits & (1ULL << (16 - 1)))
        ? (int64_t)(imu_gyro_y_bits | ~imu_gyro_y_mask)
        : (int64_t)imu_gyro_y_bits;
    imu_gyro->imu_gyro_y = (float)(imu_gyro_y_raw / 4);
    uint64_t imu_gyro_z_mask = (1ULL << 16) - 1ULL;
    uint64_t imu_gyro_z_bits = (data >> 16) & imu_gyro_z_mask;
    int64_t imu_gyro_z_raw = (imu_gyro_z_bits & (1ULL << (16 - 1)))
        ? (int64_t)(imu_gyro_z_bits | ~imu_gyro_z_mask)
        : (int64_t)imu_gyro_z_bits;
    imu_gyro->imu_gyro_z = (float)(imu_gyro_z_raw / 4);
}

void receive_faults(const can_msg_t *message, faults_t *faults) {
    
    uint32_t data_bigendian;
    memcpy(&data_bigendian, message->data, 4);
    uint32_t data = __builtin_bswap32(data_bigendian);
    uint64_t CAN_OUTGOING_FAULT_mask = (1ULL << 1) - 1ULL;
    uint64_t CAN_OUTGOING_FAULT_raw = (data >> 31) & CAN_OUTGOING_FAULT_mask;
    faults->CAN_OUTGOING_FAULT = (bool)CAN_OUTGOING_FAULT_raw;
    uint64_t CAN_INCOMING_FAULT_mask = (1ULL << 1) - 1ULL;
    uint64_t CAN_INCOMING_FAULT_raw = (data >> 30) & CAN_INCOMING_FAULT_mask;
    faults->CAN_INCOMING_FAULT = (bool)CAN_INCOMING_FAULT_raw;
    uint64_t BMS_CAN_MONITOR_FAULT_mask = (1ULL << 1) - 1ULL;
    uint64_t BMS_CAN_MONITOR_FAULT_raw = (data >> 29) & BMS_CAN_MONITOR_FAULT_mask;
    faults->BMS_CAN_MONITOR_FAULT = (bool)BMS_CAN_MONITOR_FAULT_raw;
    uint64_t LIGHTNING_CAN_MONITOR_FAULT_mask = (1ULL << 1) - 1ULL;
    uint64_t LIGHTNING_CAN_MONITOR_FAULT_raw = (data >> 28) & LIGHTNING_CAN_MONITOR_FAULT_mask;
    faults->LIGHTNING_CAN_MONITOR_FAULT = (bool)LIGHTNING_CAN_MONITOR_FAULT_raw;
    uint64_t ONBOARD_TEMP_FAULT_mask = (1ULL << 1) - 1ULL;
    uint64_t ONBOARD_TEMP_FAULT_raw = (data >> 27) & ONBOARD_TEMP_FAULT_mask;
    faults->ONBOARD_TEMP_FAULT = (bool)ONBOARD_TEMP_FAULT_raw;
    uint64_t IMU_ACCEL_FAULT_mask = (1ULL << 1) - 1ULL;
    uint64_t IMU_ACCEL_FAULT_raw = (data >> 26) & IMU_ACCEL_FAULT_mask;
    faults->IMU_ACCEL_FAULT = (bool)IMU_ACCEL_FAULT_raw;
    uint64_t IMU_GYRO_FAULT_mask = (1ULL << 1) - 1ULL;
    uint64_t IMU_GYRO_FAULT_raw = (data >> 25) & IMU_GYRO_FAULT_mask;
    faults->IMU_GYRO_FAULT = (bool)IMU_GYRO_FAULT_raw;
    uint64_t BSPD_PREFAULT_mask = (1ULL << 1) - 1ULL;
    uint64_t BSPD_PREFAULT_raw = (data >> 24) & BSPD_PREFAULT_mask;
    faults->BSPD_PREFAULT = (bool)BSPD_PREFAULT_raw;
    uint64_t ONBOARD_BRAKE_OPEN_CIRCUIT_FAULT_mask = (1ULL << 1) - 1ULL;
    uint64_t ONBOARD_BRAKE_OPEN_CIRCUIT_FAULT_raw = (data >> 23) & ONBOARD_BRAKE_OPEN_CIRCUIT_FAULT_mask;
    faults->ONBOARD_BRAKE_OPEN_CIRCUIT_FAULT = (bool)ONBOARD_BRAKE_OPEN_CIRCUIT_FAULT_raw;
    uint64_t ONBOARD_ACCEL_OPEN_CIRCUIT_FAULT_mask = (1ULL << 1) - 1ULL;
    uint64_t ONBOARD_ACCEL_OPEN_CIRCUIT_FAULT_raw = (data >> 22) & ONBOARD_ACCEL_OPEN_CIRCUIT_FAULT_mask;
    faults->ONBOARD_ACCEL_OPEN_CIRCUIT_FAULT = (bool)ONBOARD_ACCEL_OPEN_CIRCUIT_FAULT_raw;
    uint64_t ONBOARD_BRAKE_SHORT_CIRCUIT_FAULT_mask = (1ULL << 1) - 1ULL;
    uint64_t ONBOARD_BRAKE_SHORT_CIRCUIT_FAULT_raw = (data >> 21) & ONBOARD_BRAKE_SHORT_CIRCUIT_FAULT_mask;
    faults->ONBOARD_BRAKE_SHORT_CIRCUIT_FAULT = (bool)ONBOARD_BRAKE_SHORT_CIRCUIT_FAULT_raw;
    uint64_t ONBOARD_ACCEL_SHORT_CIRCUIT_FAULT_mask = (1ULL << 1) - 1ULL;
    uint64_t ONBOARD_ACCEL_SHORT_CIRCUIT_FAULT_raw = (data >> 20) & ONBOARD_ACCEL_SHORT_CIRCUIT_FAULT_mask;
    faults->ONBOARD_ACCEL_SHORT_CIRCUIT_FAULT = (bool)ONBOARD_ACCEL_SHORT_CIRCUIT_FAULT_raw;
    uint64_t ONBOARD_PEDAL_DIFFERENCE_FAULT_mask = (1ULL << 1) - 1ULL;
    uint64_t ONBOARD_PEDAL_DIFFERENCE_FAULT_raw = (data >> 19) & ONBOARD_PEDAL_DIFFERENCE_FAULT_mask;
    faults->ONBOARD_PEDAL_DIFFERENCE_FAULT = (bool)ONBOARD_PEDAL_DIFFERENCE_FAULT_raw;
    uint64_t RTDS_FAULT_mask = (1ULL << 1) - 1ULL;
    uint64_t RTDS_FAULT_raw = (data >> 18) & RTDS_FAULT_mask;
    faults->RTDS_FAULT = (bool)RTDS_FAULT_raw;
    uint64_t LV_LOW_VOLTAGE_FAULT_mask = (1ULL << 1) - 1ULL;
    uint64_t LV_LOW_VOLTAGE_FAULT_raw = (data >> 17) & LV_LOW_VOLTAGE_FAULT_mask;
    faults->LV_LOW_VOLTAGE_FAULT = (bool)LV_LOW_VOLTAGE_FAULT_raw;
    uint64_t PRECHARGE_FLOATING_FAULT_mask = (1ULL << 1) - 1ULL;
    uint64_t PRECHARGE_FLOATING_FAULT_raw = (data >> 16) & PRECHARGE_FLOATING_FAULT_mask;
    faults->PRECHARGE_FLOATING_FAULT = (bool)PRECHARGE_FLOATING_FAULT_raw;
    uint64_t LATCHING_ACTIVE_FAULT_mask = (1ULL << 1) - 1ULL;
    uint64_t LATCHING_ACTIVE_FAULT_raw = (data >> 15) & LATCHING_ACTIVE_FAULT_mask;
    faults->LATCHING_ACTIVE_FAULT = (bool)LATCHING_ACTIVE_FAULT_raw;
}

void receive_lv_voltage(const can_msg_t *message, lv_voltage_t *lv_voltage) {
    
    uint64_t data_bigendian;
    memcpy(&data_bigendian, message->data, 8);
    uint64_t data = __builtin_bswap64(data_bigendian);
    uint64_t ADC_mask = (1ULL << 16) - 1ULL;
    uint64_t ADC_raw = (data >> 48) & ADC_mask;
    lv_voltage->ADC = (uint16_t)ADC_raw;
    uint64_t Voltage_mask = (1ULL << 32) - 1ULL;
    uint64_t Voltage_raw = (data >> 16) & Voltage_mask;
    lv_voltage->Voltage = (float)(Voltage_raw / 1000);
}

void receive_vcu_test_message(const can_msg_t *message, vcu_test_message_t *vcu_test_message) {
    
    uint64_t data_bigendian;
    memcpy(&data_bigendian, message->data, 8);
    uint64_t data = __builtin_bswap64(data_bigendian);
    uint64_t three_bits_mask = (1ULL << 3) - 1ULL;
    uint64_t three_bits_raw = (data >> 61) & three_bits_mask;
    vcu_test_message->three_bits = (uint8_t)three_bits_raw;
    uint64_t float_value_mask = (1ULL << 32) - 1ULL;
    uint64_t float_value_bits = (data >> 29) & float_value_mask;
    int64_t float_value_raw = (float_value_bits & (1ULL << (32 - 1)))
        ? (int64_t)(float_value_bits | ~float_value_mask)
        : (int64_t)float_value_bits;
    vcu_test_message->float_value = (float)(float_value_raw / 100);
    uint64_t five_bits_mask = (1ULL << 5) - 1ULL;
    uint64_t five_bits_raw = (data >> 24) & five_bits_mask;
    vcu_test_message->five_bits = (uint8_t)five_bits_raw;
    uint64_t sixteen_bits_mask = (1ULL << 16) - 1ULL;
    uint64_t sixteen_bits_raw = (data >> 8) & sixteen_bits_mask;
    vcu_test_message->sixteen_bits = (uint16_t)sixteen_bits_raw;
    uint64_t signed_8_bits_mask = (1ULL << 8) - 1ULL;
    uint64_t signed_8_bits_bits = (data >> 0) & signed_8_bits_mask;
    int64_t signed_8_bits_raw = (signed_8_bits_bits & (1ULL << (8 - 1)))
        ? (int64_t)(signed_8_bits_bits | ~signed_8_bits_mask)
        : (int64_t)signed_8_bits_bits;
    vcu_test_message->signed_8_bits = (int8_t)signed_8_bits_raw;
}

void receive_dti_motor_temp_as_reported_by_vcu(const can_msg_t *message, dti_motor_temp_as_reported_by_vcu_t *dti_motor_temp_as_reported_by_vcu) {
    
    uint16_t data_bigendian;
    memcpy(&data_bigendian, message->data, 2);
    uint16_t data = __builtin_bswap16(data_bigendian);
    uint64_t temp_mask = (1ULL << 16) - 1ULL;
    uint64_t temp_raw = (data >> 0) & temp_mask;
    dti_motor_temp_as_reported_by_vcu->temp = (uint16_t)temp_raw;
}

void receive_dti_controller_temp_as_reported_by_vcu(const can_msg_t *message, dti_controller_temp_as_reported_by_vcu_t *dti_controller_temp_as_reported_by_vcu) {
    
    uint16_t data_bigendian;
    memcpy(&data_bigendian, message->data, 2);
    uint16_t data = __builtin_bswap16(data_bigendian);
    uint64_t temp_mask = (1ULL << 16) - 1ULL;
    uint64_t temp_raw = (data >> 0) & temp_mask;
    dti_controller_temp_as_reported_by_vcu->temp = (uint16_t)temp_raw;
}

void receive_bms_battbox_temp_as_reported_by_vcu(const can_msg_t *message, bms_battbox_temp_as_reported_by_vcu_t *bms_battbox_temp_as_reported_by_vcu) {
    
    uint32_t data_bigendian;
    memcpy(&data_bigendian, message->data, 4);
    uint32_t data = __builtin_bswap32(data_bigendian);
    uint64_t temp_mask = (1ULL << 32) - 1ULL;
    uint64_t temp_bits = (data >> 0) & temp_mask;
    int64_t temp_raw = (temp_bits & (1ULL << (32 - 1)))
        ? (int64_t)(temp_bits | ~temp_mask)
        : (int64_t)temp_bits;
    bms_battbox_temp_as_reported_by_vcu->temp = (float)(temp_raw / 100);
}

void receive_brake_state_as_reported_by_vcu(const can_msg_t *message, brake_state_as_reported_by_vcu_t *brake_state_as_reported_by_vcu) {
    
    uint8_t data = message->data[0];
    uint64_t brake_state_mask = (1ULL << 8) - 1ULL;
    uint64_t brake_state_bits = (data >> 0) & brake_state_mask;
    int64_t brake_state_raw = (brake_state_bits & (1ULL << (8 - 1)))
        ? (int64_t)(brake_state_bits | ~brake_state_mask)
        : (int64_t)brake_state_bits;
    brake_state_as_reported_by_vcu->brake_state = (bool)(brake_state_raw / 100);
}

void receive_rtds_state_message(const can_msg_t *message, rtds_state_message_t *rtds_state_message) {
    
    uint32_t data_bigendian;
    memcpy(&data_bigendian, message->data, 4);
    uint32_t data = __builtin_bswap32(data_bigendian);
    uint64_t pin_state_mask = (1ULL << 8) - 1ULL;
    uint64_t pin_state_raw = (data >> 24) & pin_state_mask;
    rtds_state_message->pin_state = (bool)pin_state_raw;
    uint64_t sounding_state_mask = (1ULL << 8) - 1ULL;
    uint64_t sounding_state_raw = (data >> 16) & sounding_state_mask;
    rtds_state_message->sounding_state = (bool)sounding_state_raw;
    uint64_t reverse_state_mask = (1ULL << 8) - 1ULL;
    uint64_t reverse_state_raw = (data >> 8) & reverse_state_mask;
    rtds_state_message->reverse_state = (bool)reverse_state_raw;
    uint64_t error_mask = (1ULL << 8) - 1ULL;
    uint64_t error_raw = (data >> 0) & error_mask;
    rtds_state_message->error = (bool)error_raw;
}

void receive_lfiu_low_current_adc_readings(const can_msg_t *message, lfiu_low_current_adc_readings_t *lfiu_low_current_adc_readings) {
    
    uint64_t data_bigendian;
    memcpy(&data_bigendian, message->data, 8);
    uint64_t data = __builtin_bswap64(data_bigendian);
    uint64_t raw_mask = (1ULL << 16) - 1ULL;
    uint64_t raw_raw = (data >> 48) & raw_mask;
    lfiu_low_current_adc_readings->raw = (uint16_t)raw_raw;
    uint64_t voltage_mask = (1ULL << 16) - 1ULL;
    uint64_t voltage_raw = (data >> 32) & voltage_mask;
    lfiu_low_current_adc_readings->voltage = (float)(voltage_raw / 1000);
    uint64_t current_mask = (1ULL << 16) - 1ULL;
    uint64_t current_bits = (data >> 16) & current_mask;
    int64_t current_raw = (current_bits & (1ULL << (16 - 1)))
        ? (int64_t)(current_bits | ~current_mask)
        : (int64_t)current_bits;
    lfiu_low_current_adc_readings->current = (float)(current_raw / 1000);
}

void receive_lfiu_high_current_adc_readings(const can_msg_t *message, lfiu_high_current_adc_readings_t *lfiu_high_current_adc_readings) {
    
    uint64_t data_bigendian;
    memcpy(&data_bigendian, message->data, 8);
    uint64_t data = __builtin_bswap64(data_bigendian);
    uint64_t raw_mask = (1ULL << 16) - 1ULL;
    uint64_t raw_raw = (data >> 48) & raw_mask;
    lfiu_high_current_adc_readings->raw = (uint16_t)raw_raw;
    uint64_t voltage_mask = (1ULL << 16) - 1ULL;
    uint64_t voltage_raw = (data >> 32) & voltage_mask;
    lfiu_high_current_adc_readings->voltage = (float)(voltage_raw / 1000);
    uint64_t current_mask = (1ULL << 16) - 1ULL;
    uint64_t current_bits = (data >> 16) & current_mask;
    int64_t current_raw = (current_bits & (1ULL << (16 - 1)))
        ? (int64_t)(current_bits | ~current_mask)
        : (int64_t)current_bits;
    lfiu_high_current_adc_readings->current = (float)(current_raw / 100);
}

void receive_second_vcu_test_message(const can_msg_t *message, second_vcu_test_message_t *second_vcu_test_message) {
    
    uint64_t data_bigendian;
    memcpy(&data_bigendian, message->data, 8);
    uint64_t data = __builtin_bswap64(data_bigendian);
    uint64_t one_mask = (1ULL << 14) - 1ULL;
    uint64_t one_raw = (data >> 50) & one_mask;
    second_vcu_test_message->one = (uint16_t)one_raw;
    uint64_t two_mask = (1ULL << 2) - 1ULL;
    uint64_t two_raw = (data >> 48) & two_mask;
    second_vcu_test_message->two = (uint8_t)two_raw;
    uint64_t three_mask = (1ULL << 2) - 1ULL;
    uint64_t three_raw = (data >> 46) & three_mask;
    second_vcu_test_message->three = (uint8_t)three_raw;
    uint64_t four_mask = (1ULL << 1) - 1ULL;
    uint64_t four_raw = (data >> 45) & four_mask;
    second_vcu_test_message->four = (bool)four_raw;
    uint64_t five_mask = (1ULL << 6) - 1ULL;
    uint64_t five_raw = (data >> 39) & five_mask;
    second_vcu_test_message->five = (uint8_t)five_raw;
    uint64_t six_mask = (1ULL << 23) - 1ULL;
    uint64_t six_raw = (data >> 16) & six_mask;
    second_vcu_test_message->six = (uint32_t)six_raw;
}

void receive_lv_box_fan_pwm(const can_msg_t *message, lv_box_fan_pwm_t *lv_box_fan_pwm) {
    
    uint8_t data = message->data[0];
    uint64_t fan_pwm_percentage_mask = (1ULL << 8) - 1ULL;
    uint64_t fan_pwm_percentage_raw = (data >> 0) & fan_pwm_percentage_mask;
    lv_box_fan_pwm->fan_pwm_percentage = (uint8_t)fan_pwm_percentage_raw;
}

void receive_bms_shutdown_status_as_reported_by_vcu(const can_msg_t *message, bms_shutdown_status_as_reported_by_vcu_t *bms_shutdown_status_as_reported_by_vcu) {
    
    uint8_t data = message->data[0];
    uint64_t bms_shutdown_as_reported_by_vcu_mask = (1ULL << 8) - 1ULL;
    uint64_t bms_shutdown_as_reported_by_vcu_raw = (data >> 0) & bms_shutdown_as_reported_by_vcu_mask;
    bms_shutdown_status_as_reported_by_vcu->bms_shutdown_as_reported_by_vcu = (bool)bms_shutdown_as_reported_by_vcu_raw;
}

void receive_drive_lock_states(const can_msg_t *message, drive_lock_states_t *drive_lock_states) {
    
    uint8_t data = message->data[0];
    uint64_t BRAKE_OC_mask = (1ULL << 1) - 1ULL;
    uint64_t BRAKE_OC_raw = (data >> 7) & BRAKE_OC_mask;
    drive_lock_states->BRAKE_OC = (bool)BRAKE_OC_raw;
    uint64_t BRAKE_SC_mask = (1ULL << 1) - 1ULL;
    uint64_t BRAKE_SC_raw = (data >> 6) & BRAKE_SC_mask;
    drive_lock_states->BRAKE_SC = (bool)BRAKE_SC_raw;
    uint64_t ACCEL_OC_mask = (1ULL << 1) - 1ULL;
    uint64_t ACCEL_OC_raw = (data >> 5) & ACCEL_OC_mask;
    drive_lock_states->ACCEL_OC = (bool)ACCEL_OC_raw;
    uint64_t ACCEL_SC_mask = (1ULL << 1) - 1ULL;
    uint64_t ACCEL_SC_raw = (data >> 4) & ACCEL_SC_mask;
    drive_lock_states->ACCEL_SC = (bool)ACCEL_SC_raw;
    uint64_t ACCEL_DIFF_mask = (1ULL << 1) - 1ULL;
    uint64_t ACCEL_DIFF_raw = (data >> 3) & ACCEL_DIFF_mask;
    drive_lock_states->ACCEL_DIFF = (bool)ACCEL_DIFF_raw;
    uint64_t BSPD_PREF_mask = (1ULL << 1) - 1ULL;
    uint64_t BSPD_PREF_raw = (data >> 2) & BSPD_PREF_mask;
    drive_lock_states->BSPD_PREF = (bool)BSPD_PREF_raw;
    uint64_t BMS_NOT_PRECHARGED_YET_mask = (1ULL << 1) - 1ULL;
    uint64_t BMS_NOT_PRECHARGED_YET_raw = (data >> 1) & BMS_NOT_PRECHARGED_YET_mask;
    drive_lock_states->BMS_NOT_PRECHARGED_YET = (bool)BMS_NOT_PRECHARGED_YET_raw;
}

void receive_wheel_buttons(const can_msg_t *message, wheel_buttons_t *wheel_buttons) {
    
    uint8_t data = message->data[0];
    uint64_t button_id_mask = (1ULL << 8) - 1ULL;
    uint64_t button_id_raw = (data >> 0) & button_id_mask;
    wheel_buttons->button_id = (uint8_t)button_id_raw;
}

void receive_lightning_board_imu_acceleration_data(const can_msg_t *message, lightning_board_imu_acceleration_data_t *lightning_board_imu_acceleration_data) {
    
    uint64_t data_bigendian;
    memcpy(&data_bigendian, message->data, 8);
    uint64_t data = __builtin_bswap64(data_bigendian);
    uint64_t accel_x_mask = (1ULL << 16) - 1ULL;
    uint64_t accel_x_bits = (data >> 48) & accel_x_mask;
    int64_t accel_x_raw = (accel_x_bits & (1ULL << (16 - 1)))
        ? (int64_t)(accel_x_bits | ~accel_x_mask)
        : (int64_t)accel_x_bits;
    lightning_board_imu_acceleration_data->accel_x = (float)(accel_x_raw / 1000);
    uint64_t accel_y_mask = (1ULL << 16) - 1ULL;
    uint64_t accel_y_bits = (data >> 32) & accel_y_mask;
    int64_t accel_y_raw = (accel_y_bits & (1ULL << (16 - 1)))
        ? (int64_t)(accel_y_bits | ~accel_y_mask)
        : (int64_t)accel_y_bits;
    lightning_board_imu_acceleration_data->accel_y = (float)(accel_y_raw / 1000);
    uint64_t accel_z_mask = (1ULL << 16) - 1ULL;
    uint64_t accel_z_bits = (data >> 16) & accel_z_mask;
    int64_t accel_z_raw = (accel_z_bits & (1ULL << (16 - 1)))
        ? (int64_t)(accel_z_bits | ~accel_z_mask)
        : (int64_t)accel_z_bits;
    lightning_board_imu_acceleration_data->accel_z = (float)(accel_z_raw / 1000);
}

void receive_lightning_board_imu_gyro_data(const can_msg_t *message, lightning_board_imu_gyro_data_t *lightning_board_imu_gyro_data) {
    
    uint64_t data_bigendian;
    memcpy(&data_bigendian, message->data, 8);
    uint64_t data = __builtin_bswap64(data_bigendian);
    uint64_t gyro_x_mask = (1ULL << 16) - 1ULL;
    uint64_t gyro_x_bits = (data >> 48) & gyro_x_mask;
    int64_t gyro_x_raw = (gyro_x_bits & (1ULL << (16 - 1)))
        ? (int64_t)(gyro_x_bits | ~gyro_x_mask)
        : (int64_t)gyro_x_bits;
    lightning_board_imu_gyro_data->gyro_x = (float)(gyro_x_raw / 1000);
    uint64_t gyro_y_mask = (1ULL << 16) - 1ULL;
    uint64_t gyro_y_bits = (data >> 32) & gyro_y_mask;
    int64_t gyro_y_raw = (gyro_y_bits & (1ULL << (16 - 1)))
        ? (int64_t)(gyro_y_bits | ~gyro_y_mask)
        : (int64_t)gyro_y_bits;
    lightning_board_imu_gyro_data->gyro_y = (float)(gyro_y_raw / 1000);
    uint64_t gyro_z_mask = (1ULL << 16) - 1ULL;
    uint64_t gyro_z_bits = (data >> 16) & gyro_z_mask;
    int64_t gyro_z_raw = (gyro_z_bits & (1ULL << (16 - 1)))
        ? (int64_t)(gyro_z_bits | ~gyro_z_mask)
        : (int64_t)gyro_z_bits;
    lightning_board_imu_gyro_data->gyro_z = (float)(gyro_z_raw / 1000);
}

void receive_lightning_board_lightning_sensor_information(const can_msg_t *message, lightning_board_lightning_sensor_information_t *lightning_board_lightning_sensor_information) {
    
    uint64_t data_bigendian;
    memcpy(&data_bigendian, message->data, 8);
    uint64_t data = __builtin_bswap64(data_bigendian);
    uint64_t interrupt_mask = (1ULL << 8) - 1ULL;
    uint64_t interrupt_raw = (data >> 56) & interrupt_mask;
    lightning_board_lightning_sensor_information->interrupt = (uint8_t)interrupt_raw;
    uint64_t distance_mask = (1ULL << 8) - 1ULL;
    uint64_t distance_raw = (data >> 48) & distance_mask;
    lightning_board_lightning_sensor_information->distance = (uint8_t)distance_raw;
    uint64_t energy_mask = (1ULL << 32) - 1ULL;
    uint64_t energy_raw = (data >> 16) & energy_mask;
    lightning_board_lightning_sensor_information->energy = (uint32_t)energy_raw;
}

void receive_lightning_board_magnometer_sensor_information(const can_msg_t *message, lightning_board_magnometer_sensor_information_t *lightning_board_magnometer_sensor_information) {
    
    uint64_t data_bigendian;
    memcpy(&data_bigendian, message->data, 8);
    uint64_t data = __builtin_bswap64(data_bigendian);
    uint64_t mag_x_mask = (1ULL << 16) - 1ULL;
    uint64_t mag_x_bits = (data >> 48) & mag_x_mask;
    int64_t mag_x_raw = (mag_x_bits & (1ULL << (16 - 1)))
        ? (int64_t)(mag_x_bits | ~mag_x_mask)
        : (int64_t)mag_x_bits;
    lightning_board_magnometer_sensor_information->mag_x = (float)(mag_x_raw / 1000);
    uint64_t mag_y_mask = (1ULL << 16) - 1ULL;
    uint64_t mag_y_bits = (data >> 32) & mag_y_mask;
    int64_t mag_y_raw = (mag_y_bits & (1ULL << (16 - 1)))
        ? (int64_t)(mag_y_bits | ~mag_y_mask)
        : (int64_t)mag_y_bits;
    lightning_board_magnometer_sensor_information->mag_y = (float)(mag_y_raw / 1000);
    uint64_t mag_z_mask = (1ULL << 16) - 1ULL;
    uint64_t mag_z_bits = (data >> 16) & mag_z_mask;
    int64_t mag_z_raw = (mag_z_bits & (1ULL << (16 - 1)))
        ? (int64_t)(mag_z_bits | ~mag_z_mask)
        : (int64_t)mag_z_bits;
    lightning_board_magnometer_sensor_information->mag_z = (float)(mag_z_raw / 1000);
}

void receive_lightning_pulse_message(const can_msg_t *message, lightning_pulse_message_t *lightning_pulse_message) {
    
    uint32_t data_bigendian;
    memcpy(&data_bigendian, message->data, 4);
    uint32_t data = __builtin_bswap32(data_bigendian);
    uint64_t count_mask = (1ULL << 32) - 1ULL;
    uint64_t count_raw = (data >> 0) & count_mask;
    lightning_pulse_message->count = (uint32_t)count_raw;
}

void receive_front_msb_env(const can_msg_t *message, front_msb_env_t *front_msb_env) {
    
    uint32_t data_bigendian;
    memcpy(&data_bigendian, message->data, 4);
    uint32_t data = __builtin_bswap32(data_bigendian);
    uint64_t temp_mask = (1ULL << 16) - 1ULL;
    uint64_t temp_raw = (data >> 16) & temp_mask;
    front_msb_env->temp = (float)(temp_raw / 10);
    uint64_t humidity_mask = (1ULL << 16) - 1ULL;
    uint64_t humidity_raw = (data >> 0) & humidity_mask;
    front_msb_env->humidity = (float)(humidity_raw / 10);
}

void receive_front_msb_accel(const can_msg_t *message, front_msb_accel_t *front_msb_accel) {
    
    uint64_t data_bigendian;
    memcpy(&data_bigendian, message->data, 8);
    uint64_t data = __builtin_bswap64(data_bigendian);
    uint64_t x_force_mask = (1ULL << 16) - 1ULL;
    uint64_t x_force_bits = (data >> 48) & x_force_mask;
    int64_t x_force_raw = (x_force_bits & (1ULL << (16 - 1)))
        ? (int64_t)(x_force_bits | ~x_force_mask)
        : (int64_t)x_force_bits;
    front_msb_accel->x_force = (float)x_force_raw;
    uint64_t y_force_mask = (1ULL << 16) - 1ULL;
    uint64_t y_force_bits = (data >> 32) & y_force_mask;
    int64_t y_force_raw = (y_force_bits & (1ULL << (16 - 1)))
        ? (int64_t)(y_force_bits | ~y_force_mask)
        : (int64_t)y_force_bits;
    front_msb_accel->y_force = (float)y_force_raw;
    uint64_t z_force_mask = (1ULL << 16) - 1ULL;
    uint64_t z_force_bits = (data >> 16) & z_force_mask;
    int64_t z_force_raw = (z_force_bits & (1ULL << (16 - 1)))
        ? (int64_t)(z_force_bits | ~z_force_mask)
        : (int64_t)z_force_bits;
    front_msb_accel->z_force = (float)z_force_raw;
}

void receive_front_msb_gyro(const can_msg_t *message, front_msb_gyro_t *front_msb_gyro) {
    
    uint64_t data_bigendian;
    memcpy(&data_bigendian, message->data, 8);
    uint64_t data = __builtin_bswap64(data_bigendian);
    uint64_t x_deg_mask = (1ULL << 16) - 1ULL;
    uint64_t x_deg_bits = (data >> 48) & x_deg_mask;
    int64_t x_deg_raw = (x_deg_bits & (1ULL << (16 - 1)))
        ? (int64_t)(x_deg_bits | ~x_deg_mask)
        : (int64_t)x_deg_bits;
    front_msb_gyro->x_deg = (float)x_deg_raw;
    uint64_t y_deg_mask = (1ULL << 16) - 1ULL;
    uint64_t y_deg_bits = (data >> 32) & y_deg_mask;
    int64_t y_deg_raw = (y_deg_bits & (1ULL << (16 - 1)))
        ? (int64_t)(y_deg_bits | ~y_deg_mask)
        : (int64_t)y_deg_bits;
    front_msb_gyro->y_deg = (float)y_deg_raw;
    uint64_t z_deg_mask = (1ULL << 16) - 1ULL;
    uint64_t z_deg_bits = (data >> 16) & z_deg_mask;
    int64_t z_deg_raw = (z_deg_bits & (1ULL << (16 - 1)))
        ? (int64_t)(z_deg_bits | ~z_deg_mask)
        : (int64_t)z_deg_bits;
    front_msb_gyro->z_deg = (float)z_deg_raw;
}

void receive_front_msb_strain(const can_msg_t *message, front_msb_strain_t *front_msb_strain) {
    
    uint64_t data_bigendian;
    memcpy(&data_bigendian, message->data, 8);
    uint64_t data = __builtin_bswap64(data_bigendian);
    uint64_t strain1_mask = (1ULL << 32) - 1ULL;
    uint64_t strain1_raw = (data >> 32) & strain1_mask;
    front_msb_strain->strain1 = (uint32_t)strain1_raw;
    uint64_t strain2_mask = (1ULL << 32) - 1ULL;
    uint64_t strain2_raw = (data >> 0) & strain2_mask;
    front_msb_strain->strain2 = (uint32_t)strain2_raw;
}

void receive_front_shockpot(const can_msg_t *message, front_shockpot_t *front_shockpot) {
    
    struct __attribute__((__packed__)) {
        uint32_t shock1;
        uint16_t shock1_raw;
        
    } bitstream_data;

    memcpy(&bitstream_data, message->data, sizeof(bitstream_data));

    
    
    
    front_shockpot->shock1 = (float)bitstream_data.shock1;
    
    
    
    
    
    front_shockpot->shock1_raw = (uint16_t)bitstream_data.shock1_raw;
    
    
    
}

void receive_front_ride_height(const can_msg_t *message, front_ride_height_t *front_ride_height) {
    
    uint16_t data_bigendian;
    memcpy(&data_bigendian, message->data, 2);
    uint16_t data = __builtin_bswap16(data_bigendian);
    uint64_t rh_mask = (1ULL << 16) - 1ULL;
    uint64_t rh_bits = (data >> 0) & rh_mask;
    int64_t rh_raw = (rh_bits & (1ULL << (16 - 1)))
        ? (int64_t)(rh_bits | ~rh_mask)
        : (int64_t)rh_bits;
    front_ride_height->rh = (float)rh_raw;
}

void receive_front_wheel_temp(const can_msg_t *message, front_wheel_temp_t *front_wheel_temp) {
    
    uint16_t data_bigendian;
    memcpy(&data_bigendian, message->data, 2);
    uint16_t data = __builtin_bswap16(data_bigendian);
    uint64_t wheel_temp_mask = (1ULL << 16) - 1ULL;
    uint64_t wheel_temp_raw = (data >> 0) & wheel_temp_mask;
    front_wheel_temp->wheel_temp = (float)wheel_temp_raw;
}

void receive_front_msb_orientation(const can_msg_t *message, front_msb_orientation_t *front_msb_orientation) {
    
    uint64_t data_bigendian;
    memcpy(&data_bigendian, message->data, 8);
    uint64_t data = __builtin_bswap64(data_bigendian);
    uint64_t x_fdeg_mask = (1ULL << 16) - 1ULL;
    uint64_t x_fdeg_bits = (data >> 48) & x_fdeg_mask;
    int64_t x_fdeg_raw = (x_fdeg_bits & (1ULL << (16 - 1)))
        ? (int64_t)(x_fdeg_bits | ~x_fdeg_mask)
        : (int64_t)x_fdeg_bits;
    front_msb_orientation->x_fdeg = (float)x_fdeg_raw;
    uint64_t y_fdeg_mask = (1ULL << 16) - 1ULL;
    uint64_t y_fdeg_bits = (data >> 32) & y_fdeg_mask;
    int64_t y_fdeg_raw = (y_fdeg_bits & (1ULL << (16 - 1)))
        ? (int64_t)(y_fdeg_bits | ~y_fdeg_mask)
        : (int64_t)y_fdeg_bits;
    front_msb_orientation->y_fdeg = (float)y_fdeg_raw;
    uint64_t z_fdeg_mask = (1ULL << 16) - 1ULL;
    uint64_t z_fdeg_bits = (data >> 16) & z_fdeg_mask;
    int64_t z_fdeg_raw = (z_fdeg_bits & (1ULL << (16 - 1)))
        ? (int64_t)(z_fdeg_bits | ~z_fdeg_mask)
        : (int64_t)z_fdeg_bits;
    front_msb_orientation->z_fdeg = (float)z_fdeg_raw;
}

void receive_back_msb_env(const can_msg_t *message, back_msb_env_t *back_msb_env) {
    
    uint32_t data_bigendian;
    memcpy(&data_bigendian, message->data, 4);
    uint32_t data = __builtin_bswap32(data_bigendian);
    uint64_t temp_mask = (1ULL << 16) - 1ULL;
    uint64_t temp_raw = (data >> 16) & temp_mask;
    back_msb_env->temp = (float)(temp_raw / 10);
    uint64_t humidity_mask = (1ULL << 16) - 1ULL;
    uint64_t humidity_raw = (data >> 0) & humidity_mask;
    back_msb_env->humidity = (float)(humidity_raw / 10);
}

void receive_back_msb_accel(const can_msg_t *message, back_msb_accel_t *back_msb_accel) {
    
    uint64_t data_bigendian;
    memcpy(&data_bigendian, message->data, 8);
    uint64_t data = __builtin_bswap64(data_bigendian);
    uint64_t x_force_mask = (1ULL << 16) - 1ULL;
    uint64_t x_force_bits = (data >> 48) & x_force_mask;
    int64_t x_force_raw = (x_force_bits & (1ULL << (16 - 1)))
        ? (int64_t)(x_force_bits | ~x_force_mask)
        : (int64_t)x_force_bits;
    back_msb_accel->x_force = (float)x_force_raw;
    uint64_t y_force_mask = (1ULL << 16) - 1ULL;
    uint64_t y_force_bits = (data >> 32) & y_force_mask;
    int64_t y_force_raw = (y_force_bits & (1ULL << (16 - 1)))
        ? (int64_t)(y_force_bits | ~y_force_mask)
        : (int64_t)y_force_bits;
    back_msb_accel->y_force = (float)y_force_raw;
    uint64_t z_force_mask = (1ULL << 16) - 1ULL;
    uint64_t z_force_bits = (data >> 16) & z_force_mask;
    int64_t z_force_raw = (z_force_bits & (1ULL << (16 - 1)))
        ? (int64_t)(z_force_bits | ~z_force_mask)
        : (int64_t)z_force_bits;
    back_msb_accel->z_force = (float)z_force_raw;
}

void receive_back_msb_gyro(const can_msg_t *message, back_msb_gyro_t *back_msb_gyro) {
    
    uint64_t data_bigendian;
    memcpy(&data_bigendian, message->data, 8);
    uint64_t data = __builtin_bswap64(data_bigendian);
    uint64_t x_deg_mask = (1ULL << 16) - 1ULL;
    uint64_t x_deg_bits = (data >> 48) & x_deg_mask;
    int64_t x_deg_raw = (x_deg_bits & (1ULL << (16 - 1)))
        ? (int64_t)(x_deg_bits | ~x_deg_mask)
        : (int64_t)x_deg_bits;
    back_msb_gyro->x_deg = (float)x_deg_raw;
    uint64_t y_deg_mask = (1ULL << 16) - 1ULL;
    uint64_t y_deg_bits = (data >> 32) & y_deg_mask;
    int64_t y_deg_raw = (y_deg_bits & (1ULL << (16 - 1)))
        ? (int64_t)(y_deg_bits | ~y_deg_mask)
        : (int64_t)y_deg_bits;
    back_msb_gyro->y_deg = (float)y_deg_raw;
    uint64_t z_deg_mask = (1ULL << 16) - 1ULL;
    uint64_t z_deg_bits = (data >> 16) & z_deg_mask;
    int64_t z_deg_raw = (z_deg_bits & (1ULL << (16 - 1)))
        ? (int64_t)(z_deg_bits | ~z_deg_mask)
        : (int64_t)z_deg_bits;
    back_msb_gyro->z_deg = (float)z_deg_raw;
}

void receive_back_msb_strain(const can_msg_t *message, back_msb_strain_t *back_msb_strain) {
    
    uint64_t data_bigendian;
    memcpy(&data_bigendian, message->data, 8);
    uint64_t data = __builtin_bswap64(data_bigendian);
    uint64_t strain1_mask = (1ULL << 32) - 1ULL;
    uint64_t strain1_raw = (data >> 32) & strain1_mask;
    back_msb_strain->strain1 = (uint32_t)strain1_raw;
    uint64_t strain2_mask = (1ULL << 32) - 1ULL;
    uint64_t strain2_raw = (data >> 0) & strain2_mask;
    back_msb_strain->strain2 = (uint32_t)strain2_raw;
}

void receive_back_shockpot(const can_msg_t *message, back_shockpot_t *back_shockpot) {
    
    struct __attribute__((__packed__)) {
        uint32_t shock1;
        uint16_t shock1_raw;
        
    } bitstream_data;

    memcpy(&bitstream_data, message->data, sizeof(bitstream_data));

    
    
    
    back_shockpot->shock1 = (float)bitstream_data.shock1;
    
    
    
    
    
    back_shockpot->shock1_raw = (uint16_t)bitstream_data.shock1_raw;
    
    
    
}

void receive_back_ride_height(const can_msg_t *message, back_ride_height_t *back_ride_height) {
    
    uint16_t data_bigendian;
    memcpy(&data_bigendian, message->data, 2);
    uint16_t data = __builtin_bswap16(data_bigendian);
    uint64_t rh_mask = (1ULL << 16) - 1ULL;
    uint64_t rh_bits = (data >> 0) & rh_mask;
    int64_t rh_raw = (rh_bits & (1ULL << (16 - 1)))
        ? (int64_t)(rh_bits | ~rh_mask)
        : (int64_t)rh_bits;
    back_ride_height->rh = (float)rh_raw;
}

void receive_back_wheel_temp(const can_msg_t *message, back_wheel_temp_t *back_wheel_temp) {
    
    uint16_t data_bigendian;
    memcpy(&data_bigendian, message->data, 2);
    uint16_t data = __builtin_bswap16(data_bigendian);
    uint64_t wheel_temp_mask = (1ULL << 16) - 1ULL;
    uint64_t wheel_temp_raw = (data >> 0) & wheel_temp_mask;
    back_wheel_temp->wheel_temp = (float)wheel_temp_raw;
}

void receive_back_msb_orientation(const can_msg_t *message, back_msb_orientation_t *back_msb_orientation) {
    
    uint64_t data_bigendian;
    memcpy(&data_bigendian, message->data, 8);
    uint64_t data = __builtin_bswap64(data_bigendian);
    uint64_t x_fdeg_mask = (1ULL << 16) - 1ULL;
    uint64_t x_fdeg_bits = (data >> 48) & x_fdeg_mask;
    int64_t x_fdeg_raw = (x_fdeg_bits & (1ULL << (16 - 1)))
        ? (int64_t)(x_fdeg_bits | ~x_fdeg_mask)
        : (int64_t)x_fdeg_bits;
    back_msb_orientation->x_fdeg = (float)x_fdeg_raw;
    uint64_t y_fdeg_mask = (1ULL << 16) - 1ULL;
    uint64_t y_fdeg_bits = (data >> 32) & y_fdeg_mask;
    int64_t y_fdeg_raw = (y_fdeg_bits & (1ULL << (16 - 1)))
        ? (int64_t)(y_fdeg_bits | ~y_fdeg_mask)
        : (int64_t)y_fdeg_bits;
    back_msb_orientation->y_fdeg = (float)y_fdeg_raw;
    uint64_t z_fdeg_mask = (1ULL << 16) - 1ULL;
    uint64_t z_fdeg_bits = (data >> 16) & z_fdeg_mask;
    int64_t z_fdeg_raw = (z_fdeg_bits & (1ULL << (16 - 1)))
        ? (int64_t)(z_fdeg_bits | ~z_fdeg_mask)
        : (int64_t)z_fdeg_bits;
    back_msb_orientation->z_fdeg = (float)z_fdeg_raw;
}

void receive_imd_general_information(const can_msg_t *message, imd_general_information_t *imd_general_information) {
    
    struct __attribute__((__packed__)) {
        uint16_t R_iso_corrected;
        uint8_t R_iso_status;
        uint8_t Iso_measurement_counter;
        uint8_t device_error;
        uint8_t HV_pos_conn_fail;
        uint8_t HV_neg_conn_fail;
        uint8_t Earth_conn_fail;
        uint8_t Iso_alarm;
        uint8_t iso_warning;
        uint8_t iso_outdated;
        uint8_t Unbalance_alarm;
        uint8_t Undervoltage_alarm;
        uint8_t Unsafe_to_start;
        uint8_t Earthlift_Open;
        uint8_t warnings_and_alarms_unused_bits;
        uint8_t Device_Activity;
        uint8_t Not_Applicable;
        
    } bitstream_data;

    memcpy(&bitstream_data, message->data, sizeof(bitstream_data));

    
    
    
    imd_general_information->R_iso_corrected = (uint16_t)bitstream_data.R_iso_corrected;
    
    
    
    
    
    imd_general_information->R_iso_status = (uint8_t)bitstream_data.R_iso_status;
    
    
    
    
    
    imd_general_information->Iso_measurement_counter = (uint8_t)bitstream_data.Iso_measurement_counter;
    
    
    
    
    
    imd_general_information->device_error = (bool)bitstream_data.device_error;
    
    
    
    
    
    imd_general_information->HV_pos_conn_fail = (bool)bitstream_data.HV_pos_conn_fail;
    
    
    
    
    
    imd_general_information->HV_neg_conn_fail = (bool)bitstream_data.HV_neg_conn_fail;
    
    
    
    
    
    imd_general_information->Earth_conn_fail = (bool)bitstream_data.Earth_conn_fail;
    
    
    
    
    
    imd_general_information->Iso_alarm = (bool)bitstream_data.Iso_alarm;
    
    
    
    
    
    imd_general_information->iso_warning = (bool)bitstream_data.iso_warning;
    
    
    
    
    
    imd_general_information->iso_outdated = (bool)bitstream_data.iso_outdated;
    
    
    
    
    
    imd_general_information->Unbalance_alarm = (bool)bitstream_data.Unbalance_alarm;
    
    
    
    
    
    imd_general_information->Undervoltage_alarm = (bool)bitstream_data.Undervoltage_alarm;
    
    
    
    
    
    imd_general_information->Unsafe_to_start = (bool)bitstream_data.Unsafe_to_start;
    
    
    
    
    
    
    
}

