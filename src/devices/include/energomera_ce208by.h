#ifndef SRC_DEVICES_INCLUDE_ENERGOMERA_CE208BY_H_
#define SRC_DEVICES_INCLUDE_ENERGOMERA_CE208BY_H_

typedef enum {
    cmd_open_channel         = 0x03,
    cmd_serial_number        = 0x04,
    cmd_tariffs_data         = 0x01,
    cmd_resource_battery     = 0x20,
    cmd_volts_data           = 0x18,
    cmd_amps_data            = 0x16,
    cmd_power_data           = 0x0E,
} command_t;

typedef enum {
    get_tariff_summ = 0x01,
    get_tariff_1    = 0x02,
    get_tariff_2    = 0x04,
    get_tariff_3    = 0x08,
    get_tariff_4    = 0x10,
} get_tariff_t;

typedef struct {
    uint8_t     pkt_len;
    uint8_t     pkt_data[PKT_BUFF_MAX_LEN];
} pkt_t;

#endif /* SRC_DEVICES_INCLUDE_ENERGOMERA_CE208BY_H_ */
