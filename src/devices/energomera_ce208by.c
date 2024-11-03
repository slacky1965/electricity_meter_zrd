#include "tl_common.h"
#include "zcl_include.h"

#include "device.h"
#include "energomera_ce208by.h"
#include "se_custom_attr.h"
#include "app_uart.h"
#include "app_endpoint_cfg.h"
#include "app_dev_config.h"

#define START_STOP  0xc0
#define STUFF_DB    0xdb
#define STUFF_DC    0xdc
#define STUFF_DD    0xdd
#define GET_INFO    0x0006


static uint8_t package_buff[PKT_BUFF_MAX_LEN];

static pkt_t pkt_in = {0, {0}};
static pkt_t pkt_out = {0, {0}};

static const uint8_t crc8tab[] = {
        0x00, 0xb5, 0xdf, 0x6a, 0x0b, 0xbe, 0xd4, 0x61, 0x16, 0xa3, 0xc9, 0x7c, 0x1d, 0xa8, 0xc2, 0x77,
        0x2c, 0x99, 0xf3, 0x46, 0x27, 0x92, 0xf8, 0x4d, 0x3a, 0x8f, 0xe5, 0x50, 0x31, 0x84, 0xee, 0x5b,
        0x58, 0xed, 0x87, 0x32, 0x53, 0xe6, 0x8c, 0x39, 0x4e, 0xfb, 0x91, 0x24, 0x45, 0xf0, 0x9a, 0x2f,
        0x74, 0xc1, 0xab, 0x1e, 0x7f, 0xca, 0xa0, 0x15, 0x62, 0xd7, 0xbd, 0x08, 0x69, 0xdc, 0xb6, 0x03,
        0xb0, 0x05, 0x6f, 0xda, 0xbb, 0x0e, 0x64, 0xd1, 0xa6, 0x13, 0x79, 0xcc, 0xad, 0x18, 0x72, 0xc7,
        0x9c, 0x29, 0x43, 0xf6, 0x97, 0x22, 0x48, 0xfd, 0x8a, 0x3f, 0x55, 0xe0, 0x81, 0x34, 0x5e, 0xeb,
        0xe8, 0x5d, 0x37, 0x82, 0xe3, 0x56, 0x3c, 0x89, 0xfe, 0x4b, 0x21, 0x94, 0xf5, 0x40, 0x2a, 0x9f,
        0xc4, 0x71, 0x1b, 0xae, 0xcf, 0x7a, 0x10, 0xa5, 0xd2, 0x67, 0x0d, 0xb8, 0xd9, 0x6c, 0x06, 0xb3,
        0xd5, 0x60, 0x0a, 0xbf, 0xde, 0x6b, 0x01, 0xb4, 0xc3, 0x76, 0x1c, 0xa9, 0xc8, 0x7d, 0x17, 0xa2,
        0xf9, 0x4c, 0x26, 0x93, 0xf2, 0x47, 0x2d, 0x98, 0xef, 0x5a, 0x30, 0x85, 0xe4, 0x51, 0x3b, 0x8e,
        0x8d, 0x38, 0x52, 0xe7, 0x86, 0x33, 0x59, 0xec, 0x9b, 0x2e, 0x44, 0xf1, 0x90, 0x25, 0x4f, 0xfa,
        0xa1, 0x14, 0x7e, 0xcb, 0xaa, 0x1f, 0x75, 0xc0, 0xb7, 0x02, 0x68, 0xdd, 0xbc, 0x09, 0x63, 0xd6,
        0x65, 0xd0, 0xba, 0x0f, 0x6e, 0xdb, 0xb1, 0x04, 0x73, 0xc6, 0xac, 0x19, 0x78, 0xcd, 0xa7, 0x12,
        0x49, 0xfc, 0x96, 0x23, 0x42, 0xf7, 0x9d, 0x28, 0x5f, 0xea, 0x80, 0x35, 0x54, 0xe1, 0x8b, 0x3e,
        0x3d, 0x88, 0xe2, 0x57, 0x36, 0x83, 0xe9, 0x5c, 0x2b, 0x9e, 0xf4, 0x41, 0x20, 0x95, 0xff, 0x4a,
        0x11, 0xa4, 0xce, 0x7b, 0x1a, 0xaf, 0xc5, 0x70, 0x07, 0xb2, 0xd8, 0x6d, 0x0c, 0xb9, 0xd3, 0x66
};

static uint8_t checksum(const uint8_t *src_buffer, uint8_t len) {

    const uint8_t* table = &src_buffer[1];
    const uint8_t packet_len = len - 3;
    uint8_t crc;


    crc = 0xff;

    for (uint8_t i = 0; i < packet_len; i++) {
        crc = crc8tab[crc ^ table[i]];
    }

    return crc;
}


/* 0xc0 -> 0xdb, 0xdc
 * 0xdb -> 0xdb, 0xdd
 * 0xdb, 0xdd -> 0xdb
 * 0xdb, 0xdc -> 0xc0
 */

static size_t byte_stuffing() {

    uint8_t *source, *receiver;
    size_t len = 0;

    source = pkt_out.pkt_data;
    receiver = package_buff;

    *(receiver++) = *(source++);
    len++;

    for (int i = 0; i < (pkt_out.pkt_len-2); i++) {
        if (*source == STUFF_DB) {
            *(receiver++) = STUFF_DB;
            len++;
            *receiver = STUFF_DD;
        } else if (*source == START_STOP) {
            *(receiver++) = STUFF_DB;
            len++;
            *receiver = STUFF_DC;
        } else {
            *receiver = *source;
        }
        source++;
        receiver++;
        len++;
    }

    *(receiver) = *(source);
    len++;

    return len;
}

static size_t byte_unstuffing(uint8_t load_len) {

    size_t   len = 0;
    uint8_t *source = package_buff;
    uint8_t *receiver = pkt_in.pkt_data;

    *(receiver++) = *(source++);
    len++;

    for (int i = 0; i < (load_len-2); i++) {
        if (*source == STUFF_DB) {
            source++;
            len--;
            if (*source == STUFF_DD) {
                *receiver = STUFF_DB;
            } else if (*source == STUFF_DC) {
                *receiver = START_STOP;
            } else {
                /* error */
                return 0;
            }
        } else {
            *receiver = *source;
        }
        source++;
        receiver++;
        len++;
    }

    *(receiver) = *(source);
    len++;

    return len;
}

static uint8_t send_command(command_t command) {

    uint8_t buff_len, len = 0;

//    app_uart_rx_off();
    flush_buff_uart();

//    set_command(command);

    buff_len = byte_stuffing();

    /* three attempts to write to uart */
    for (uint8_t attempt = 0; attempt < 3; attempt++) {
        len = write_bytes_to_uart(package_buff, buff_len);
        if (len == buff_len) {
            break;
        } else {
            len = 0;
        }
#if UART_PRINTF_MODE
        printf("Attempt to send data to uart: %d\r\n", attempt+1);
#endif
        sleep_ms(250);
    }

    sleep_ms(100);
//    app_uart_rx_on(9600);

#if UART_PRINTF_MODE && DEBUG_PACKAGE
    if (len == 0) {
        uint8_t head[] = "write to uart error";
        print_package(head, package_buff, buff_len);
    } else {
        uint8_t head[] = "write to uart";
        print_package(head, package_buff, len);
    }
#endif

    return len;
}

static pkt_error_t response_meter(command_t command) {

    size_t len, load_size = 0;
    uint8_t ch, complete = false;

    pkt_error_no = PKT_ERR_TIMEOUT;

    memset(package_buff, 0, sizeof(package_buff));

    /* trying to read for 1 seconds */
    for(uint8_t i = 0; i < 100; i++ ) {
        load_size = 0;
        if (available_buff_uart()) {
            while (available_buff_uart() && load_size < PKT_BUFF_MAX_LEN) {

                ch = read_byte_from_buff_uart();

                if (load_size == 0) {
                    if (ch != START_STOP) {
                        pkt_error_no = PKT_ERR_NO_PKT;
                        continue;
                    }
                } else if (ch == START_STOP) {
                    complete = true;
                }

                package_buff[load_size++] = ch;

                if (complete) {
                    i = 100;
                    pkt_error_no = PKT_OK;
                    break;
                }
            }
        }
        sleep_ms(10);
    }

    if (load_size) {
#if UART_PRINTF_MODE && DEBUG_PACKAGE
        uint8_t head[] = "read from uart";
        print_package(head, package_buff, load_size);
#endif
        if (complete) {
            len = byte_unstuffing(load_size);
            if (len) {
                pkt_in.pkt_len = len;
                uint8_t crc = checksum(pkt_in.pkt_data, pkt_in.pkt_len);
                if (crc == pkt_in.pkt_data[(pkt_in.pkt_len-2)]) {
                    if (command == pkt_in.pkt_data[3]) {
                        pkt_error_no = PKT_OK;
                    } else {
                        pkt_error_no = PKT_ERR_DIFFERENT_COMMAND;
                    }
                } else {
                    pkt_error_no = PKT_ERR_CRC;
                }
            } else {
                pkt_error_no = PKT_ERR_UNSTUFFING;
            }
        } else {
            pkt_error_no = PKT_ERR_INCOMPLETE;
        }
    } else {
#if UART_PRINTF_MODE && DEBUG_PACKAGE
        uint8_t head[] = "read from uart error";
        print_package(head, package_buff, load_size);
#endif
    }

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    if (pkt_error_no != PKT_OK) print_error(pkt_error_no);
#endif

    return pkt_error_no;
}

static uint8_t open_session() {

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand open of session\r\n");
#endif

    pkt_out.pkt_len = 0;

    pkt_out.pkt_data[pkt_out.pkt_len++] = START_STOP;
    pkt_out.pkt_data[pkt_out.pkt_len++] = GET_INFO & 0xFF;
    pkt_out.pkt_data[pkt_out.pkt_len++] = (GET_INFO >> 8) & 0xFF;
    pkt_out.pkt_data[pkt_out.pkt_len++] = cmd_open_channel;
    pkt_out.pkt_data[pkt_out.pkt_len] = checksum(pkt_out.pkt_data, pkt_out.pkt_len+2);
    pkt_out.pkt_len++;
    pkt_out.pkt_data[pkt_out.pkt_len++] = START_STOP;

    if (send_command(cmd_open_channel)) {
        if (response_meter(cmd_open_channel) == PKT_OK) {
            pkt_out.pkt_len = 0;

            pkt_out.pkt_data[pkt_out.pkt_len++] = START_STOP;
            pkt_out.pkt_data[pkt_out.pkt_len++] = GET_INFO & 0xFF;
            pkt_out.pkt_data[pkt_out.pkt_len++] = (GET_INFO >> 8) & 0xFF;
            pkt_out.pkt_data[pkt_out.pkt_len++] = cmd_serial_number;
            pkt_out.pkt_data[pkt_out.pkt_len++] = cmd_open_channel;
            pkt_out.pkt_data[pkt_out.pkt_len] = checksum(pkt_out.pkt_data, pkt_out.pkt_len+2);
            pkt_out.pkt_len++;
            pkt_out.pkt_data[pkt_out.pkt_len++] = START_STOP;

            if (send_command(cmd_serial_number)) {
                if (response_meter(cmd_serial_number) == PKT_OK) {
                    return true;
                }
            }

        }
    }

    return false;
}


uint8_t measure_meter_energomera_ce208by() {

    uint8_t ret = open_session();

    printf("ret: %d\r\n", ret);

    return ret;
}
