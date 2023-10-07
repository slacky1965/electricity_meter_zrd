#ifndef SRC_INCLUDE_MERCURY_206_H_
#define SRC_INCLUDE_MERCURY_206_H_

typedef enum _command_t {
    cmd_serial_number   = 0x2f,
    cmd_tariffs_data    = 0x27,
    cmd_net_params      = 0x63,
    cmd_date_release    = 0x66,
    cmd_running_time    = 0x69,
    cmd_timeout         = 0x82,
    cmd_test_error      = 0xf0
} command_t;

typedef struct __attribute__((packed)) _package_t {
    u32 address;
    u8  cmd;
    u8  data[DATA_MAX_LEN];
    u8  pkt_len;
} package_t;

typedef struct __attribute__((packed)) _pkt_serial_num_t {
    u32 address;
    u8  cmd;
    u32 addr;
    u16 crc;
} pkt_serial_num_t;

typedef struct __attribute__((packed)) _pkt_release_t {
    u32 address;
    u8  cmd;
    u8  date[3];
    u16 crc;
} pkt_release_t;

typedef struct __attribute__((packed)) _pkt_net_params_t {
    u32 address;
    u8  cmd;
    u16 volts;
    u16 amps;
    u8  power[3];
    u16 crc;
} pkt_net_params_t;

typedef struct __attribute__((packed)) _pkt_tariffs_t {
    u32 address;
    u8  cmd;
    u32 tariff_1;
    u32 tariff_2;
    u32 tariff_3;
    u32 tariff_4;
    u16 crc;
} pkt_tariffs_t;

typedef struct __attribute__((packed)) _pkt_timeout_t {
    u32 address;
    u8  cmd;
    u8  timeout;
    u16 crc;
} pkt_timeout_t;

typedef struct __attribute__((packed)) _pkt_running_time_t {
    u32 address;
    u8  cmd;
    u8  tl[3];
    u8  tlb[3];
    u16 crc;
} pkt_running_time_t;

#endif /* SRC_INCLUDE_MERCURY_206_H_ */
