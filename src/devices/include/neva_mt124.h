#ifndef SRC_DEVICES_INCLUDE_NEVA_MT124_H_
#define SRC_DEVICES_INCLUDE_NEVA_MT124_H_

#define TYPE_6102   6102
#define TYPE_7109   7109

typedef enum {
    HEBA_124_UNKNOWN = 0,
    HEBA_124_6102,
    HEBA_124_7109,
    HEBA_124_MAX
} neva_124_type_t;

typedef enum _command_t {
    cmd_open_channel = 0,
    cmd_ack_start,
    cmd_password_6102,
    cmd_password_7109,
    cmd_serial_number,
    cmd_sensors_data,
    cmd_tariffs_6102,
    cmd_tariffs_7109,
    cmd_power_data,
    cmd_volts_data,
    cmd_amps_data,
    cmd_close_channel,
    cmd_max
} command_t;

#endif /* SRC_DEVICES_INCLUDE_NEVA_MT124_H_ */
