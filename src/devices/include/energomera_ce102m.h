#ifndef SRC_INCLUDE_ENERGOMERA_CE102M_H_
#define SRC_INCLUDE_ENERGOMERA_CE102M_H_

typedef enum _command_t {
    cmd_open_channel = 0,
    cmd_close_channel,
    cmd_ack_start,
    cmd_start_password,
    cmd_stat_data,
    cmd_tariffs_data,
    cmd_power_data,
    cmd_volts_data,
    cmd_amps_data,
    cmd_serial_number
} command_t;

typedef struct __attribute__((packed)) _ce102m_status_t {
    u32 current_tariff             :3;     /* 0-2      */
    u32 stat_battery               :1;     /* 3        */
    u32 reserve                    :3;     /* 4-6      */
    u32 energy_direction           :1;     /* 7        */
    u32 load_characteristic        :1;     /* 8        */
    u32 time_correction            :1;     /* 9        */
    u32 current_voltage            :2;     /* 10-11    */
    u32 stat_time                  :1;     /* 12       */
    u32 reserve2                   :1;     /* 13       */
    u32 seasonal_time              :1;     /* 14       */
    u32 reserve3                   :1;     /* 15       */
    u32 energy_params_error        :1;     /* 16       */
    u32 dvkz                       :1;     /* 17       */
    u32 reserve4                   :1;     /* 18       */
    u32 exp_date_battery           :1;     /* 19       */
    u32 progmem_crc_error          :1;     /* 20       */
    u32 metrolog_params_crc_error  :1;     /* 21       */
    u32 reserve5                   :2;     /* 22-23    */
    u32 schedule_tariffs           :4;     /* 24-27    */
    u32 schedule_tariffs_error     :1;     /* 28       */
    u32 reserve6                   :3;     /* 29-31    */
} ce102m_status_t;


#endif /* SRC_INCLUDE_ENERGOMERA_CE102M_H_ */
