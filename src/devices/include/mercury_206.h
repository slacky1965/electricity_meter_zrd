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
    uint32_t address;
    uint8_t  cmd;
    uint8_t  data[DATA_MAX_LEN];
    uint8_t  pkt_len;
} package_t;

typedef struct __attribute__((packed)) _pkt_serial_num_t {
    uint32_t address;
    uint8_t  cmd;
    uint32_t addr;
    uint16_t crc;
} pkt_serial_num_t;

typedef struct __attribute__((packed)) _pkt_release_t {
    uint32_t address;
    uint8_t  cmd;
    uint8_t  date[3];
    uint16_t crc;
} pkt_release_t;

typedef struct __attribute__((packed)) _pkt_net_params_t {
    uint32_t address;
    uint8_t  cmd;
    uint16_t volts;
    uint16_t amps;
    uint8_t  power[3];
    uint16_t crc;
} pkt_net_params_t;

typedef struct __attribute__((packed)) _pkt_tariffs_t {
    uint32_t address;
    uint8_t  cmd;
    uint32_t tariff_1;
    uint32_t tariff_2;
    uint32_t tariff_3;
    uint32_t tariff_4;
    uint16_t crc;
} pkt_tariffs_t;

typedef struct __attribute__((packed)) _pkt_timeout_t {
    uint32_t address;
    uint8_t  cmd;
    uint8_t  timeout;
    uint16_t crc;
} pkt_timeout_t;

typedef struct __attribute__((packed)) _pkt_running_time_t {
    uint32_t address;
    uint8_t  cmd;
    uint8_t  tl[3];
    uint8_t  tlb[3];
    uint16_t crc;
} pkt_running_time_t;

#endif /* SRC_INCLUDE_MERCURY_206_H_ */
