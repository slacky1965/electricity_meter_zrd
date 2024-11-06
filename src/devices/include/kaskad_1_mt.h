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

typedef enum {
    BOARD_TYPE_UNKNOWN = 0,
    BOADR_TYPE1,
    BOADR_TYPE2,
    BOADR_TYPE3,
} board_type_t;

typedef enum {
    BOARD_ID_01 = 0x01,     // begin board id of type1  //
    BOARD_ID_02 = 0x02,
    BOARD_ID_03 = 0x03,
    BOARD_ID_04 = 0x04,
    BOARD_ID_0C = 0x0C,
    BOARD_ID_0D = 0x0D,
    BOARD_ID_11 = 0x11,
    BOARD_ID_12 = 0x12,     // end board id of type1    //
    BOARD_ID_07 = 0x07,     // begin board id of type2  //
    BOARD_ID_08 = 0x08,
    BOARD_ID_0A = 0x0A,
    BOARD_ID_0B = 0x0B,     // end board id of type2    //
    BOARD_ID_09 = 0x09,     // begin board id of type3  //
    BOARD_ID_0E = 0x0E,
    BOARD_ID_0F = 0x0F,
    BOARD_ID_10 = 0x10,
    BOARD_ID_20 = 0x20,
    BOARD_ID_21 = 0x21,
    BOARD_ID_22 = 0x22      // end board id of type3    //
} board_id_t;

typedef struct __attribute__((packed)) _package_header_t {
    uint8_t  data_len   :5;     /* 0-4 bits - data lenght                               */
    uint8_t  from_to    :1;     /* 1 request to the device, 0 response from the device  */
    uint8_t  cpu_power  :1;     /* 1 sufficient computing power, 0 weak computing power */
    uint8_t  crypted    :1;     /* 1 crypted, 0 non crypted                             */
    uint8_t  reserved;
    uint16_t address_to;
    uint16_t address_from;
    uint8_t  command;
    uint32_t password_status;
} package_header_t;

typedef struct __attribute__((packed)) _response_status_t {
    uint8_t  role;
    uint8_t  info1;
    uint8_t  info2;
    uint8_t  error;
} response_status_t;

typedef struct __attribute__((packed)) _package_t {
    uint8_t  start;
    uint8_t  boundary;
    package_header_t header;
    uint8_t  data[PKT_BUFF_MAX_LEN];
    uint8_t  pkt_len;
} package_t;

typedef struct __attribute__((packed)) _pkt_tariffs_t {
    uint32_t sum_tariffs;
    uint8_t  byte_cfg;
    uint8_t  division_factor;
    uint8_t  role;
    uint8_t  multiplication_factor[3];
    uint32_t tariff_1;
    uint32_t tariff_2;
    uint32_t tariff_3;
    uint32_t tariff_4;
} pkt_tariffs_t;

typedef struct __attribute__((packed)) _pkt_amps_t {
    uint8_t  phase_num; /* number of phase    */
    uint8_t  amps[3];   /* maybe 2 or 3 bytes */
} pkt_amps_t;

typedef struct __attribute__((packed)) _pkt_volts_t {
    uint8_t  phase_num;
    uint16_t volts;
} pkt_volts_t;

typedef struct __attribute__((packed)) _pkt_power_t {
    uint8_t  power[3];
    uint8_t  byte_cfg;
    uint8_t  division_factor;
} pkt_power_t;

typedef struct __attribute__((packed)) _pkt_read_cfg_t {
    uint8_t  divisor            :2; /* 0 - "00000000", 1 - "0000000.0", 2 - "000000.00", 3 - "00000.000"    */
    uint8_t  current_tariff     :2; /* 0 - first, 1 - second, 2 - third, 3 - fourth                         */
    uint8_t  char_num           :2; /* 0 - 6, 1 - 7, 2 - 8, 3 - 8                                           */
    uint8_t  tariffs            :2; /* 0 - 1, 1 - 1+2, 2 - 1+2+3, 3 - 1+2+3+4                               */
    uint8_t  summer_winter_time :1; /* automatic switching to daylight saving time or winter time 1 - on    */
    uint8_t  tariff_schedule    :2; /* 0 - work, 1 - Sat., 2 - Sun., 3 - special                            */
    uint8_t  power_limits       :1;
    uint8_t  power_limits_emerg :1;
    uint8_t  power_limits_cmd   :1;
    uint8_t  access_cust_info   :1;
    uint8_t  reserve            :1;
    uint8_t  display_time;
    uint8_t  counter_passowrd;
    uint8_t  months_worked;         /* battery */
    uint8_t  remaining_months_work; /* battery */
    uint8_t  role;
} pkt_read_cfg_t;

typedef struct __attribute__((packed)) _pkt_resbat_t {
    uint8_t  lifetime;
    uint8_t  worktime;
} pkt_resbat_t;

typedef struct __attribute__((packed)) _pkt_info_t {
    uint8_t  id;
    uint8_t  data[24];
    uint8_t  interface1;
    uint8_t  interface2;
    uint8_t  interface3;
    uint8_t  interface4;
    uint16_t battery_mv;
} pkt_info_t;

typedef struct __attribute__((packed)) _pkt_data31_t {
    uint8_t  start;
    uint8_t  boundary;
    package_header_t header;
    uint8_t  sub_command;
    uint8_t  data[DATA_MAX_LEN];    /* data31 -> data[30] + sub_command = 31 */
    uint8_t  crc;
    uint8_t  stop;
} pkt_data31_t;


#endif /* SRC_INCLUDE_KASKAD_1_MT_H_ */
