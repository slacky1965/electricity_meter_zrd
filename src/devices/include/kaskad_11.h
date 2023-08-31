#ifndef SRC_INCLUDE_KASKAD_11_H_
#define SRC_INCLUDE_KASKAD_11_H_

typedef enum _command_t {
    cmd_open_channel    = 0x02,
    cmd_close_channel   = 0x03,
    cmd_datetime_device = 0x16,
    cmd_net_parameters  = 0x20,
    cmd_serial_number   = 0x25,
    cmd_tariffs_data    = 0x26,
    cmd_date_release    = 0x53,
    cmd_test_error      = 0xf0
} command_t;

typedef enum _net_parameters_t {
    net_voltage = 0,
    net_amps    = 1,
    net_power   = 5
} net_parameters_t;

typedef struct __attribute__((packed)) _package_t {
    u8  len;
    u8  cmd;
    u16 address;
    u8  data[DATA_MAX_LEN];
} package_t;

typedef struct __attribute__((packed)) _pkt_release_t {
    u8  len;
    u8  cmd;
    u16 address;
    u8  year;
    u8  month;
    u8  day;
    u8  status;
    u8  crc;
} pkt_release_t;

typedef struct __attribute__((packed)) _pkt_tariff_t {
    u8  len;
    u8  cmd;
    u16 address;
    u8  tariff_num;
    u32 value;
    u8  status;
    u8  crc;
} pkt_tariff_t;

typedef struct __attribute__((packed)) _pkt_voltage_t {
    u8  len;
    u8  cmd;
    u16 address;
    u8  param;
    u16 voltage;
    u8  status;
    u8  crc;
} pkt_voltage_t;

typedef struct __attribute__((packed)) _pkt_voltage3_t {
    u8  len;
    u8  cmd;
    u16 address;
    u8  param;
    u16 voltageA;
    u16 voltageB;
    u16 voltageC;
    u8  status;
    u8  crc;
} pkt_voltage3_t;

typedef struct __attribute__((packed)) _pkt_amps_t {
    u8  len;
    u8  cmd;
    u16 address;
    u8  param;
    u16 amps;
    u8  status;
    u8  crc;
} pkt_amps_t;

typedef struct __attribute__((packed)) _pkt_amps3_t {
    u8  len;
    u8  cmd;
    u16 address;
    u8  param;
    u8  ampsA[3];
    u8  ampsB[3];
    u8  ampsC[3];
    u8  status;
    u8  crc;
} pkt_amps3_t;

typedef struct __attribute__((packed)) _pkt_power_t {
    u8  len;
    u8  cmd;
    u16 address;
    u8  param;
    u8  power[3];
    u8  status;
    u8  crc;
} pkt_power_t;

typedef struct __attribute__((packed)) _pkt_power3_t {
    u8  len;
    u8  cmd;
    u16 address;
    u8  param;
    u8  powerA[3];
    u8  powerB[3];
    u8  powerC[3];
    u8  status;
    u8  crc;
} pkt_power3_t;

typedef struct __attribute__((packed)) _pkt_datetime_t {
    u8  len;
    u8  cmd;
    u16 address;
    u8  datetime[5];
    u8  status;
    u8  crc;
} pkt_datetime_t;


#endif /* SRC_INCLUDE_KASKAD_11_H_ */
