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
    uint32_t current_tariff             :3;     /* 0-2      */
    uint32_t stat_battery               :1;     /* 3        */
    uint32_t reserve                    :3;     /* 4-6      */
    uint32_t energy_direction           :1;     /* 7        */
    uint32_t load_characteristic        :1;     /* 8        */
    uint32_t time_correction            :1;     /* 9        */
    uint32_t current_voltage            :2;     /* 10-11    */
    uint32_t stat_time                  :1;     /* 12       */
    uint32_t reserve2                   :1;     /* 13       */
    uint32_t seasonal_time              :1;     /* 14       */
    uint32_t reserve3                   :1;     /* 15       */
    uint32_t energy_params_error        :1;     /* 16       */
    uint32_t dvkz                       :1;     /* 17       */
    uint32_t reserve4                   :1;     /* 18       */
    uint32_t exp_date_battery           :1;     /* 19       */
    uint32_t progmem_crc_error          :1;     /* 20       */
    uint32_t metrolog_params_crc_error  :1;     /* 21       */
    uint32_t reserve5                   :2;     /* 22-23    */
    uint32_t schedule_tariffs           :4;     /* 24-27    */
    uint32_t schedule_tariffs_error     :1;     /* 28       */
    uint32_t reserve6                   :3;     /* 29-31    */
} ce102m_status_t;

#endif /* SRC_INCLUDE_ENERGOMERA_CE102M_H_ */
