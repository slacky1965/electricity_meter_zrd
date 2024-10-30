#ifndef SRC_DEVICES_INCLUDE_NEVA_MT124_H_
#define SRC_DEVICES_INCLUDE_NEVA_MT124_H_

typedef enum {
    HEBA_124_UNKNOWN = 0,
    HEBA_124_6102,
    HEBA_124_7109,
    HEBA_124_MAX
} neva_124_type_t;

typedef enum _command_t {
    cmd_open_channel = 0,
    cmd_ack_start,
    cmd_tariffs_data,
    cmd_power_data,
    cmd_serial_number,
    cmd_sensors_data
} command_t;

#endif /* SRC_DEVICES_INCLUDE_NEVA_MT124_H_ */
