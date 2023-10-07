#ifndef SRC_INCLUDE_KASKAD_1_MT_H_
#define SRC_INCLUDE_KASKAD_1_MT_H_

typedef enum _command_t {
    cmd_open_channel         = 0x01,
    cmd_tariffs_data         = 0x05,
    cmd_read_configure       = 0x10,
    cmd_resource_battery     = 0x1e,
    cmd_volts_data           = 0x0129,   /* command 0x29, sub command 0x01 */
    cmd_amps_data            = 0x012c,
    cmd_power_data           = 0x2d,
    cmd_serial_number        = 0x010a,
    cmd_date_release         = 0x020a,
    cmd_factory_manufacturer = 0x030a,
    cmd_name_device          = 0x040a,
    cmd_name_device2         = 0x050a,
    cmd_get_info             = 0x30,
    cmd_test_error           = 0x60
} command_t;

typedef struct __attribute__((packed)) _package_header_t {
    u8  data_len   :5;     /* 0-4 bits - data lenght                               */
    u8  from_to    :1;     /* 1 request to the device, 0 response from the device  */
    u8  cpu_power  :1;     /* 1 sufficient computing power, 0 weak computing power */
    u8  crypted    :1;     /* 1 crypted, 0 non crypted                             */
    u8  reserved;
    u16 address_to;
    u16 address_from;
    u8  command;
    u32 password_status;
} package_header_t;

typedef struct __attribute__((packed)) _response_status_t {
    u8  role;
    u8  info1;
    u8  info2;
    u8  error;
} response_status_t;

typedef struct __attribute__((packed)) _package_t {
    u8  start;
    u8  boundary;
    package_header_t header;
    u8  data[PKT_BUFF_MAX_LEN];
    u8  pkt_len;
} package_t;

typedef struct __attribute__((packed)) _pkt_tariffs_t {
    u32 sum_tariffs;
    u8  byte_cfg;
    u8  division_factor;
    u8  role;
    u8  multiplication_factor[3];
    u32 tariff_1;
    u32 tariff_2;
    u32 tariff_3;
    u32 tariff_4;
} pkt_tariffs_t;

typedef struct __attribute__((packed)) _pkt_amps_t {
    u8  phase_num; /* number of phase    */
    u8  amps[3];   /* maybe 2 or 3 bytes */
} pkt_amps_t;

typedef struct __attribute__((packed)) _pkt_volts_t {
    u8  phase_num;
    u16 volts;
} pkt_volts_t;

typedef struct __attribute__((packed)) _pkt_power_t {
    u8  power[3];
    u8  byte_cfg;
    u8  division_factor;
} pkt_power_t;

typedef struct __attribute__((packed)) _pkt_read_cfg_t {
    u8  divisor            :2; /* 0 - "00000000", 1 - "0000000.0", 2 - "000000.00", 3 - "00000.000"    */
    u8  current_tariff     :2; /* 0 - first, 1 - second, 2 - third, 3 - fourth                         */
    u8  char_num           :2; /* 0 - 6, 1 - 7, 2 - 8, 3 - 8                                           */
    u8  tariffs            :2; /* 0 - 1, 1 - 1+2, 2 - 1+2+3, 3 - 1+2+3+4                               */
    u8  summer_winter_time :1; /* automatic switching to daylight saving time or winter time 1 - on    */
    u8  tariff_schedule    :2; /* 0 - work, 1 - Sat., 2 - Sun., 3 - special                            */
    u8  power_limits       :1;
    u8  power_limits_emerg :1;
    u8  power_limits_cmd   :1;
    u8  access_cust_info   :1;
    u8  reserve            :1;
    u8  display_time;
    u8  counter_passowrd;
    u8  months_worked;         /* battery */
    u8  remaining_months_work; /* battery */
    u8  role;
} pkt_read_cfg_t;

typedef struct __attribute__((packed)) _pkt_resbat_t {
    u8  lifetime;
    u8  worktime;
} pkt_resbat_t;

typedef struct __attribute__((packed)) _pkt_info_t {
    u8  id;
    u8  data[24];
    u8  interface1;
    u8  interface2;
    u8  interface3;
    u8  interface4;
    u16 battery_mv;
} pkt_info_t;

typedef struct __attribute__((packed)) _pkt_data31_t {
    u8  start;
    u8  boundary;
    package_header_t header;
    u8  sub_command;
    u8  data[DATA_MAX_LEN];    /* data31 -> data[30] + sub_command = 31 */
    u8  crc;
    u8  stop;
} pkt_data31_t;


#endif /* SRC_INCLUDE_KASKAD_1_MT_H_ */
