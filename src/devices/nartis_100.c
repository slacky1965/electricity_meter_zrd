#include "tl_common.h"
#include "zcl_include.h"

#include "se_custom_attr.h"
#include "app_uart.h"
#include "app_endpoint_cfg.h"
#include "app_dev_config.h"
#include "device.h"
#include "app_reporting.h"
#include "nartis_100.h"

#define PASSWORD        "111"
//#define PASSWORD        "12345678"
#define CLIENT_ADDRESS  0x20
#define PHY_DEVICE      0x10
#define LOGICAL_DEVICE  0x01
#define FLAG            0x7E
#define TYPE3           0x0A

#define MAX_INFO_FIELD  0x80
#define MIN_FRAME_SIZE  10      /* flag 1 + format 2 + address 3 + control 1 + FCS 2 + flag = 10 byte */

#define SNRM            0x93
#define DISC            0x53
#define UA              0x73
#define LSAP            0xe6
#define CMD_LSAP        LSAP
#define RESP_LSAP       0xe7
#define AARQ            0x60
#define AARE            0x61
#define AUTH            0xac
#define GET_REQUEST     0xc0
#define GET_RESPONSE    0xc4

enum {
    TYPE_SIGNED_32    = 0x05,
    TYPE_UNSIGNED_32  = 0x06,
    TYPE_OCTET_STRING = 0x09
};

static meter_t meter;
static package_t raw_package;               /* in - out */
static result_package_t result_package;
static uint8_t release_month;
static uint16_t release_year;
static uint8_t present_month;
static uint16_t present_year;

static uint64_t tariff_summ;

static uint8_t serial_number[SE_ATTR_SN_SIZE+1] = {0};
static uint8_t date_release[DATA_MAX_LEN+2] = {0};


static request_t attr_descriptor_serial_number = {
        .class =     {0x00, 0x01},
        .obis =      {0x00, 0x00, 0x60, 0x01, 0x00, 0xff},   /* 0.0.96.01.00.255 */
        .attribute = {0x02, 0x00}
};

static request_t attr_descriptor_date_release = {
        .class =     {0x00, 0x01},
        .obis =      {0x00, 0x00, 0x60, 0x01, 0x04, 0xff},   /* 0.0.96.1.4.255   */
        .attribute = {0x02, 0x00}
};

static request_t attr_descriptor_tariff1AP = {
        .class  =    {0x00, 0x03},
        .obis   =    {0x01, 0x00, 0x01, 0x08, 0x01, 0xff},   /* 1.0.1.8.1.255   */
        .attribute = {0x02, 0x00}
};

static request_t attr_descriptor_tariff1AM = {
        .class  =    {0x00, 0x03},
        .obis   =    {0x01, 0x00, 0x02, 0x08, 0x01, 0xff},   /* 1.0.2.8.1.255   */
        .attribute = {0x02, 0x00}
};

static request_t attr_descriptor_tariff2AP = {
        .class  =    {0x00, 0x03},
        .obis   =    {0x01, 0x00, 0x01, 0x08, 0x02, 0xff},   /* 1.0.1.8.2.255   */
        .attribute = {0x02, 0x00}
};

static request_t attr_descriptor_tariff2AM = {
        .class  =    {0x00, 0x03},
        .obis   =    {0x01, 0x00, 0x02, 0x08, 0x02, 0xff},   /* 1.0.2.8.2.255   */
        .attribute = {0x02, 0x00}
};

static request_t attr_descriptor_tariff3AP = {
        .class  =    {0x00, 0x03},
        .obis   =    {0x01, 0x00, 0x01, 0x08, 0x03, 0xff},   /* 1.0.1.8.3.255   */
        .attribute = {0x02, 0x00}
};

static request_t attr_descriptor_tariff3AM = {
        .class  =    {0x00, 0x03},
        .obis   =    {0x01, 0x00, 0x02, 0x08, 0x03, 0xff},   /* 1.0.2.8.3.255   */
        .attribute = {0x02, 0x00}
};

static request_t attr_descriptor_tariff4AP = {
        .class  =    {0x00, 0x03},
        .obis   =    {0x01, 0x00, 0x01, 0x08, 0x04, 0xff},   /* 1.0.1.8.4.255   */
        .attribute = {0x02, 0x00}
};

static request_t attr_descriptor_tariff4AM = {
        .class  =    {0x00, 0x03},
        .obis   =    {0x01, 0x00, 0x02, 0x08, 0x04, 0xff},   /* 1.0.2.8.4.255   */
        .attribute = {0x02, 0x00}
};

static request_t attr_descriptor_voltage = {
        .class  =    {0x00, 0x03},
        .obis   =    {0x01, 0x00, 0x0c, 0x07, 0x00, 0xff},   /* 1.0.12.7.0.255  */
        .attribute = {0x02, 0x00}
};

static request_t attr_descriptor_power = {
        .class  =    {0x00, 0x03},
        .obis   =    {0x01, 0x00, 0x09, 0x07, 0x00, 0xff},   /* 1.0.9.7.0.255  */
        .attribute = {0x02, 0x00}
};

static request_t attr_descriptor_current = {
        .class  =    {0x00, 0x03},
        .obis   =    {0x01, 0x00, 0x0b, 0x07, 0x00, 0xff},   /* 1.0.11.7.0.255  */
        .attribute = {0x02, 0x00}
};

static request_t attr_descriptor_time = {
        .class  =    {0x00, 0x08},
        .obis   =    {0x00, 0x00, 0x01, 0x00, 0x00, 0xff},   /* 0.0.1.0.0.255  */
        .attribute = {0x02, 0x00}
};



static void send_notification();

static const uint16_t fcstab[256] = {
     0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
     0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
     0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
     0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
     0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
     0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
     0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
     0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
     0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
     0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
     0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
     0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
     0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
     0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
     0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
     0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
     0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
     0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
     0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
     0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
     0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
     0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
     0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
     0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
     0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
     0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
     0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
     0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
     0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
     0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
     0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
     0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};

static uint16_t checksum(const uint8_t *src_buffer, size_t len) {

    uint16_t crc = 0xffff;

    while(len--) {
        crc = (crc >> 8) ^ fcstab[(crc ^ *src_buffer++) & 0xff];
    }

    crc ^= 0xffff;

    return crc;
}


uint8_t get_address_size(uint8_t *buff) {

    uint8_t size = 0;


    if (*(buff++) & 1) {
        size++;
    } else {
        size++;
        if (*(buff++) & 1) {
            size++;
        } else {
            size++;
            if (*(buff++) & 1) {
                size++;
            } else {
                size++;
                if (*(buff++) & 1) {
                    size++;
                } else {
                    size = 0;
                }
            }
        }
    }

    if (size != 0 && size != 1 && size != 2 && size != 4) size = 0;

    return size;

}

static uint8_t set_address(uint8_t *buff, uint8_t len, uint16_t lower, uint16_t upper) {

    uint8_t ret = true;
    switch(len) {
        case 1:
            *buff = ((uint8_t)(upper << 1)) | 0x01;
            break;
        case 2:
            *buff =     ((uint8_t)(upper << 1)) & 0x7E;
            *(buff+1) = ((uint8_t)(lower << 1)) | 0x01;
            break;
        case 4:
            *buff =     ((uint8_t)(upper >> 7)) & 0x7E;
            *(buff+1) = ((uint8_t)(upper << 1)) & 0x7E;
            *(buff+2) = ((uint8_t)(lower >> 7)) & 0x7E;
            *(buff+3) = ((uint8_t)(lower << 1)) | 0x01;
            break;
        default:
            ret = false;
            break;
    }

    return ret;
}

static uint8_t get_address(uint8_t *buff, uint8_t len, uint16_t *lower, uint16_t *upper) {

    uint8_t ret = true;

    *lower = 0;
    *upper = 0;

    switch(len) {
        case 1:
            *upper = (uint16_t)((*buff >> 1) & 0x7f);
            break;
        case 2:
            *upper = (uint16_t)((*buff >> 1) & 0x7f);
            *lower = (uint16_t)((*(buff+1) >> 1) & 0x7f);
            break;
        case 4:
            *lower = (uint16_t)((*buff >> 1) & 0x7f);
            *lower = *lower << 8;
            *lower = *lower + ((*(buff+1) >> 1) & 0x7f);
            *upper = (uint16_t)((*(buff+2) >> 1) & 0x7f);
            *upper = *upper << 8;
            *upper = *upper + ((*(buff+3) >> 1) & 0x7f);
            break;
        default:
            ret = false;
            break;
    }

    return ret;
}

static size_t send_command(uint8_t *buff, size_t size) {

    size_t len = 0;

//    app_uart_rx_off();
    flush_buff_uart();

    for (uint8_t attempt = 0; attempt < 3; attempt++) {
        len = write_bytes_to_uart(buff, size);
        if (len == size) {
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
//    app_uart_rx_on();


#if UART_PRINTF_MODE && DEBUG_PACKAGE
    if (len == 0) {
        uint8_t head[] = "write to uart error";
        print_package(head, buff, size);
    } else {
        uint8_t head[] = "write to uart";
        print_package(head, buff, len);
    }
#endif

    return len;
}




uint8_t response_meter() {

    size_t load_size, len;
    uint16_t crc, check_crc, lower, upper;
    uint8_t ch = 0;
    uint8_t *ptr_format;
    format_t format;
    pkt_error_no = PKT_ERR_TIMEOUT;

    memset(&raw_package, 0, sizeof(package_t));

    uint8_t *pkt_buff = (uint8_t*)&raw_package;

    /* trying to read for 1 seconds */
    for(uint8_t i = 0; i < 100; i++ ) {
        load_size = 0;
        if (available_buff_uart()) {

            if (get_queue_len_buff_uart() < MIN_FRAME_SIZE) {
                pkt_error_no = PKT_ERR_NO_PKT;
                flush_buff_uart();
                break;
            }

            while (available_buff_uart()) {
                ch = read_byte_from_buff_uart();

                if (ch == FLAG) {
                    pkt_buff[load_size++] = ch;
                    break;
                } else {
                    pkt_error_no = PKT_ERR_UNKNOWN_FORMAT;
                }
            }

            if (ch != FLAG) continue;

            if (get_queue_len_buff_uart() < 2) {
                pkt_error_no = PKT_ERR_NO_PKT;
                flush_buff_uart();
                break;
            }

            ptr_format = (uint8_t*)&format;

            *(ptr_format+1) = pkt_buff[load_size++] = read_byte_from_buff_uart();
            *ptr_format = pkt_buff[load_size++] = read_byte_from_buff_uart();

            if (format.type != TYPE3) {
                pkt_error_no = PKT_ERR_TYPE;
                flush_buff_uart();
                break;
            }

            if (format.length > load_size+get_queue_len_buff_uart()-2) {
                pkt_error_no = PKT_ERR_INCOMPLETE;
                flush_buff_uart();
                break;
            }

            meter.format = format;

            if (format.segmentation) {
                len = format.length + 1 - load_size;
            } else {
                len = format.length + 2 - load_size;
            }

            for(size_t i = 0; i < len; i++) {
                pkt_buff[load_size++] = read_byte_from_buff_uart();

            }

            meter.rrr = (raw_package.header.control >> 5) & 0x07;
            meter.sss = (raw_package.header.control >> 1) & 0x07;


            if (!format.segmentation) {
                if (pkt_buff[load_size-1] != FLAG) {
                    pkt_error_no = PKT_ERR_INCOMPLETE;
                    break;
                }
            }

            uint8_t size_d = get_address_size(raw_package.header.addr);

            if (size_d == 0) {
                pkt_error_no = PKT_ERR_DEST_ADDRESS;
                break;
            }
            if (!get_address(raw_package.header.addr, size_d, &lower, &upper)) {
                pkt_error_no = PKT_ERR_DEST_ADDRESS;
                break;
            }

            uint8_t size_s = get_address_size(raw_package.header.addr+size_d);

            if (size_s == 0) {
                pkt_error_no = PKT_ERR_SRC_ADDRESS;
                break;
            }
            if (!get_address(raw_package.header.addr+size_d, size_s, &lower, &upper)) {
                pkt_error_no = PKT_ERR_SRC_ADDRESS;
                break;
            }

            crc = checksum(pkt_buff+1, format.length-2);
            check_crc = pkt_buff[load_size-(meter.format.segmentation?1:2)];
            check_crc = (check_crc << 8) + pkt_buff[load_size-(meter.format.segmentation?2:3)];

            if (crc != check_crc) {
                pkt_error_no = PKT_ERR_CRC;
                break;
            }

            i = 100;
            pkt_error_no = PKT_OK;
            break;
        }
        sleep_ms(10);
    }

    if (load_size) {
#if UART_PRINTF_MODE && DEBUG_PACKAGE
        uint8_t head[] = "read from uart";
        print_package(head, pkt_buff, load_size);
#endif

    } else {
#if UART_PRINTF_MODE && DEBUG_PACKAGE
        uint8_t head[] = "read from uart error";
        print_package(head, pkt_buff, load_size);
#endif
    }

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    if (pkt_error_no != PKT_OK) print_error(pkt_error_no);
#endif

    if (load_size && pkt_error_no == PKT_OK) {
        if (meter.format.segmentation) {

            memcpy(result_package.buff+result_package.size, raw_package.data+2, load_size-(sizeof(header_t)+4));
            result_package.size += load_size-(sizeof(header_t)+4);
            send_notification();
        } else {
            memcpy(result_package.buff+result_package.size, raw_package.data+2, load_size-(sizeof(header_t)+3));
            result_package.size += load_size-(sizeof(header_t)+3);
            result_package.complete = true;
        }
    }

    return pkt_error_no;
}

static size_t set_header() {

    raw_package.header.flag = FLAG;
    meter.format.length = 2;   /* format 2 bytes */
    set_address(raw_package.header.addr, 2, meter.server_lower_addr, meter.server_upper_addr);
    set_address(raw_package.header.addr+2, 1, 0, meter.client_addr);
    meter.format.length += 3;

    return meter.format.length;
}

static uint8_t send_cmd_snrm() {

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand running of connect\r\n");
#endif

    uint8_t *pkt_buff = (uint8_t*)&raw_package;
    uint8_t info_field_data[PKT_BUFF_MAX_LEN] = {0};
    uint8_t info_field_len = 0;

    memset(pkt_buff, 0, sizeof(package_t));
    memset(&result_package, 0, sizeof(result_package_t));

    info_field_data[info_field_len++] = 0x81;
    info_field_data[info_field_len++] = 0x80;
    info_field_data[info_field_len++] = 0x00;
    info_field_data[info_field_len++] = 0x05;
    info_field_data[info_field_len++] = 0x02;
    info_field_data[info_field_len++] = (meter.max_info_field_tx >> 8) & 0xff;
    info_field_data[info_field_len++] = meter.max_info_field_tx & 0xff;
    info_field_data[info_field_len++] = 0x06;
    info_field_data[info_field_len++] = 0x02;
    info_field_data[info_field_len++] = (meter.max_info_field_rx >> 8) & 0xff;
    info_field_data[info_field_len++] = meter.max_info_field_rx & 0xff;
    info_field_data[info_field_len++] = 0x07;
    info_field_data[info_field_len++] = 0x04;
    info_field_data[info_field_len++] = (meter.window_tx >> 24) & 0xff;
    info_field_data[info_field_len++] = (meter.window_tx >> 16) & 0xff;
    info_field_data[info_field_len++] = (meter.window_tx >> 8) & 0xff;
    info_field_data[info_field_len++] = meter.window_tx & 0xff;
    info_field_data[info_field_len++] = 0x08;
    info_field_data[info_field_len++] = 0x04;
    info_field_data[info_field_len++] = (meter.window_rx >> 24) & 0xff;
    info_field_data[info_field_len++] = (meter.window_rx >> 16) & 0xff;
    info_field_data[info_field_len++] = (meter.window_rx >> 8) & 0xff;
    info_field_data[info_field_len++] = meter.window_rx & 0xff;
    info_field_data[2] = info_field_len-3;

    uint8_t hcs_len = set_header() + 1;
    raw_package.header.control = SNRM;
    meter.format.length += 3;                       /* + size command + size HCS   */

    memcpy(raw_package.data+2, info_field_data, info_field_len);

    meter.format.length += info_field_len + 2;      /* + size FCS                   */

    uint8_t *format = (uint8_t*)&(meter.format);

    raw_package.header.format[0] = format[1];
    raw_package.header.format[1] = format[0];

    uint16_t crc = checksum(pkt_buff+1, hcs_len);

    raw_package.data[1] = (crc >> 8) & 0xff;
    raw_package.data[0] = crc & 0xff;

    crc = checksum(pkt_buff+1, meter.format.length-2);
    raw_package.data[info_field_len+2] = crc & 0xff;
    raw_package.data[info_field_len+3] = (crc >> 8) & 0xff;
    raw_package.data[info_field_len+4] = FLAG;


    if (send_command(pkt_buff, meter.format.length+2)) {
        if (response_meter() == PKT_OK) {
            if (raw_package.header.control == UA)
                return true;
        }
    }


    return false;
}

static void send_cmd_disc() {

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand running of disconnect\r\n");
#endif

    uint8_t *pkt_buff = (uint8_t*)&raw_package;

    memset(pkt_buff, 0, sizeof(package_t));
    memset(&result_package, 0, sizeof(result_package_t));

    set_header();
    raw_package.header.control = DISC;
    meter.format.length += 3;                       /* + size command + size FCS   */

    uint8_t *format = (uint8_t*)&(meter.format);

    raw_package.header.format[0] = format[1];
    raw_package.header.format[1] = format[0];

    uint16_t crc = checksum(pkt_buff+1, meter.format.length-2);
    raw_package.data[0] = crc & 0xff;
    raw_package.data[1] = (crc >> 8) & 0xff;
    raw_package.data[2] = FLAG;

    if (send_command(pkt_buff, meter.format.length+2)) {
        response_meter();
    }
}

static void send_notification() {

    uint8_t *pkt_buff = (uint8_t*)&raw_package;

    memset(pkt_buff, 0, sizeof(package_t));

    set_header();

    // I don't know how true
    raw_package.header.control = ((((meter.rrr << 5) + (meter.sss << 1)) | 0x10)) & 0xFE;
//    raw_package.header.control = ((((meter.rrr << 5) + (meter.sss << 1)) | 0x10) + 2) & 0xFE;

    meter.format.length += 3;                       /* + size command + size FCS   */

    uint8_t *format = (uint8_t*)&(meter.format);

    raw_package.header.format[0] = format[1];
    raw_package.header.format[1] = format[0];

    uint16_t crc = checksum(pkt_buff+1, meter.format.length-2);
    raw_package.data[0] = crc & 0xff;
    raw_package.data[1] = (crc >> 8) & 0xff;
    raw_package.data[2] = FLAG;

    if (send_command(pkt_buff, meter.format.length+2)) {
        response_meter();
    }
}

static uint8_t send_cmd_open_session() {

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand running of open session\r\n");
#endif

    uint8_t *pkt_buff = (uint8_t*)&raw_package;

    uint8_t app_const_name[] = {0xa1, 0x09, 0x06, 0x07, 0x60, 0x85, 0x74, 0x05, 0x08, 0x01, 0x01};
    uint8_t asce[] = {0x8a, 0x02, 0x07, 0x80};
    uint8_t mech_name[] = {0x8b, 0x07, 0x60, 0x85, 0x74, 0x05, 0x08, 0x02, 0x01};
    uint8_t user_info[] = {0xbe, 0x10, 0x04, 0x0e, 0x01, 0x00, 0x00, 0x00, 0x06, 0x5f, 0x1f, 0x04, 0x00, 0x00, 0x1e, 0x9d, 0xff, 0xff};


    uint8_t info_field_data[PKT_BUFF_MAX_LEN] = {0};
    uint8_t info_field_len = 0;
    uint8_t aarq_len = 0;
    uint8_t aarq_len_idx;
    uint8_t auth_len = 0;
    uint8_t auth_len_idx;

    info_field_data[info_field_len++] = LSAP;
    info_field_data[info_field_len++] = CMD_LSAP;
    info_field_data[info_field_len++] = 0;
    info_field_data[info_field_len++] = AARQ;
    aarq_len_idx = info_field_len;
    info_field_data[info_field_len++] = aarq_len;
    memcpy(info_field_data+info_field_len, app_const_name, sizeof(app_const_name));
    info_field_len += sizeof(app_const_name);
    aarq_len += sizeof(app_const_name);
    memcpy(info_field_data+info_field_len, asce, sizeof(asce));
    info_field_len += sizeof(asce);
    aarq_len += sizeof(asce);
    memcpy(info_field_data+info_field_len, mech_name, sizeof(mech_name));
    info_field_len += sizeof(mech_name);
    aarq_len += sizeof(mech_name);
    info_field_data[info_field_len++] = AUTH;
    aarq_len++;
    auth_len_idx = info_field_len;
    info_field_data[info_field_len++] = auth_len;
    aarq_len++;
    info_field_data[info_field_len++] = 0x80;
    aarq_len++;
    auth_len++;
    info_field_data[info_field_len++] = 0x03;
    aarq_len++;
    auth_len++;
    memcpy(&info_field_data[info_field_len], meter.password.data, meter.password.size);
    info_field_len += meter.password.size;
    aarq_len += meter.password.size;
    auth_len += meter.password.size;
//    info_field_data[info_field_len++] = meter.password[0];
//    aarq_len++;
//    auth_len++;
//    info_field_data[info_field_len++] = meter.password[1];
//    aarq_len++;
//    auth_len++;
//    info_field_data[info_field_len++] = meter.password[2];
//    aarq_len++;
//    auth_len++;
    info_field_data[auth_len_idx] = auth_len;
    memcpy(info_field_data+info_field_len, user_info, sizeof(user_info));
    info_field_len += sizeof(user_info);
    aarq_len += sizeof(user_info);
    info_field_data[aarq_len_idx] = aarq_len;

    memset(pkt_buff, 0, sizeof(package_t));
    memset(&result_package, 0, sizeof(result_package_t));

    uint8_t hcs_len = set_header() + 1;
    raw_package.header.control = 0x10; //(((meter.rrr << 5) + (meter.sss << 1)) | 0x10) & 0xFE;
    meter.format.length += 3;                       /* + size command + size HCS   */

    memcpy(raw_package.data+2, info_field_data, info_field_len);

    meter.format.length += info_field_len + 2;      /* + size FCS                   */

    uint8_t *format = (uint8_t*)&(meter.format);

    raw_package.header.format[0] = format[1];
    raw_package.header.format[1] = format[0];

    uint16_t crc = checksum(pkt_buff+1, hcs_len);

    raw_package.data[1] = (crc >> 8) & 0xff;
    raw_package.data[0] = crc & 0xff;

    crc = checksum(pkt_buff+1, meter.format.length-2);
    raw_package.data[info_field_len+2] = crc & 0xff;
    raw_package.data[info_field_len+3] = (crc >> 8) & 0xff;
    raw_package.data[info_field_len+4] = FLAG;

    if (send_command(pkt_buff, meter.format.length+2)) {
        if (response_meter() == PKT_OK) {
            if (result_package.complete) {
                uint8_t *ptr = result_package.buff;

                if (*ptr++ == LSAP) {
                    if (*ptr++ == RESP_LSAP) {
                        if (*ptr++ == 0) {
                            if (*ptr++ == AARE) {
                                uint8_t aare_len = *ptr;
                                for(uint8_t i = 0; i < aare_len; i++) {
                                    if (*ptr++ == 0xA2) {
                                        ptr += *ptr;
                                        if (*ptr == 0) {
                                            /* open session successful */
                                            return true;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

            } else {
#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
                print_error(PKT_ERR_SEGMENTATION);
#endif
            }
        }
    }


    return false;
}


static uint8_t *get_request_data(request_t *request) {

    uint8_t *pkt_buff = (uint8_t*)&raw_package;

    uint8_t info_field_data[PKT_BUFF_MAX_LEN] = {0};
    uint8_t info_field_len = 0;

    info_field_data[info_field_len++] = LSAP;
    info_field_data[info_field_len++] = CMD_LSAP;
    info_field_data[info_field_len++] = 0;
    info_field_data[info_field_len++] = GET_REQUEST;
    info_field_data[info_field_len++] = 0x01;
    info_field_data[info_field_len++] = 0xc1;

    memcpy(info_field_data+info_field_len, request, sizeof(request_t));
    info_field_len += sizeof(request_t);

    memset(pkt_buff, 0, sizeof(package_t));
    memset(&result_package, 0, sizeof(result_package_t));


    uint8_t hcs_len = set_header() + 1;
    raw_package.header.control = ((((meter.rrr << 5) + (meter.sss << 1)) | 0x10) + (meter.format.segmentation?0:2)) & 0xFE;
    meter.format.length += 3;                       /* + size command + size HCS   */

    memcpy(raw_package.data+2, info_field_data, info_field_len);

    meter.format.length += info_field_len + 2;      /* + size FCS                   */

    uint8_t *format = (uint8_t*)&(meter.format);

    raw_package.header.format[0] = format[1];
    raw_package.header.format[1] = format[0];

    uint16_t crc = checksum(pkt_buff+1, hcs_len);

    raw_package.data[1] = (crc >> 8) & 0xff;
    raw_package.data[0] = crc & 0xff;

    crc = checksum(pkt_buff+1, meter.format.length-2);
    raw_package.data[info_field_len+2] = crc & 0xff;
    raw_package.data[info_field_len+3] = (crc >> 8) & 0xff;
    raw_package.data[info_field_len+4] = FLAG;

    if (send_command(pkt_buff, meter.format.length+2)) {
        if (response_meter() == PKT_OK) {
            if (result_package.complete) {
                uint8_t *ptr = result_package.buff;

                if (*ptr++ == LSAP) {
                    if (*ptr++ == RESP_LSAP) {
                        if (*ptr++ == 0) {
                            if (*ptr == GET_RESPONSE) {
                                ptr += 4;
                                return ptr;
                            }
                        }
                    }
                }
            } else {
#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
                print_error(PKT_ERR_SEGMENTATION);
#endif
            }
        }
    }
    return NULL;
}

static void get_serial_number_data() {

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand get serial number\r\n");
#endif

//    type_digit_t *unsigned32 = (type_digit_t*)get_request_data(attr_descriptor_serial_number);
    uint8_t *ptr = get_request_data(&attr_descriptor_serial_number);
    type_digit_t *unsigned32 = (type_digit_t*)ptr;
    type_octet_string_t *str = (type_octet_string_t*)ptr;


    if (ptr) {
        if (unsigned32->type == TYPE_UNSIGNED_32) {
            uint32_t addr = reverse32(unsigned32->value);

            uint8_t sn[SE_ATTR_SN_SIZE];

            itoa(addr, sn);

            if (set_zcl_str(sn, serial_number, SE_ATTR_SN_SIZE)) {
                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_METER_SERIAL_NUMBER, (uint8_t*)&serial_number);
#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
                printf("Serial Number: %s, len: %d\r\n", serial_number+1, *serial_number);
#endif
            }
        } else if (str->type == TYPE_OCTET_STRING) {

            serial_number[0] = str->size;
            memcpy(serial_number+1, &str->str, str->size);

            zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_METER_SERIAL_NUMBER, (uint8_t*)&serial_number);
#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
            printf("Serial Number: %s, len: %d\r\n", serial_number+1, *serial_number);
#endif
        }
    }
}

static void get_date_release_data() {

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand get date release\r\n");
#endif

    type_octet_string_t *o_str = (type_octet_string_t*)get_request_data(&attr_descriptor_date_release);

    if (o_str) {
        if (o_str->type == TYPE_OCTET_STRING && o_str->size) {
            uint8_t *ptr = &o_str->str;

            release_year = (uint16_t)((*ptr++) << 8);
            release_year |= *ptr++;
            release_month = *ptr++;
            uint8_t  release_day = *ptr++;
//            uint8_t  date_release[DATA_MAX_LEN+2] = {0};
            uint8_t  dr[11] = {0};
            uint8_t  dr_len = 0;

            if (release_day < 10) {
                dr[dr_len++] = '0';
                dr[dr_len++] = 0x30 + release_day;
            } else {
                dr[dr_len++] = 0x30 + release_day/10;
                dr[dr_len++] = 0x30 + release_day%10;
            }
            dr[dr_len++] = '.';

            if (release_month < 10) {
                dr[dr_len++] = '0';
                dr[dr_len++] = 0x30 + release_month;
            } else {
                dr[dr_len++] = 0x30 + release_month/10;
                dr[dr_len++] = 0x30 + release_month%10;
            }
            dr[dr_len++] = '.';

            uint8_t year_str[8] = {0};

            itoa(release_year, year_str);

            dr[dr_len++] = year_str[0];
            dr[dr_len++] = year_str[1];
            dr[dr_len++] = year_str[2];
            dr[dr_len++] = year_str[3];

            if (set_zcl_str(dr, date_release, DATA_MAX_LEN+1)) {
                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CUSTOM_DATE_RELEASE, (uint8_t*)&date_release);
#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
                printf("Date of release: %s, len: %d\r\n", date_release+1, *date_release);
#endif
            }
        }
    }
}

static void get_voltage_data() {

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand get voltage\r\n");
#endif

    uint32_t voltage = 0;
    type_digit_t *digit_voltage = (type_digit_t*)get_request_data(&attr_descriptor_voltage);

    if (digit_voltage) {

        if (digit_voltage->type == TYPE_UNSIGNED_32) {
            voltage = reverse32(digit_voltage->value);

            uint16_t volts = voltage / 10;

            zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_RMS_VOLTAGE, (uint8_t*)&volts);

#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
            printf("voltage: %d\r\n", volts);
#endif
        }
    }
}

static void get_power_data() {

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand get power\r\n");
#endif

    uint32_t power = 0;
    type_digit_t *digit_power = (type_digit_t*)get_request_data(&attr_descriptor_power);

    if (digit_power) {

        if (digit_power->type == TYPE_UNSIGNED_32) {

            power = reverse32(digit_power->value) / 1000;

            //power = 42258; //1505010; //
            //power /= 1000;

            uint16_t pwr = power & 0xffff;

            zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_ACTIVE_POWER, (uint8_t*)&pwr);

#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
            printf("power: %d\r\n", pwr);
#endif
        }
    }
}

static void get_current_data() {

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand get current\r\n");
#endif

    uint16_t current = 0;
    type_digit_t *digit_current = (type_digit_t*)get_request_data(&attr_descriptor_current);

    if (digit_current) {

        if (digit_current->type == TYPE_SIGNED_32 || digit_current->type == TYPE_UNSIGNED_32) {

            current = reverse32(digit_current->value) & 0xffff;

            zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_RMS_CURRENT, (uint8_t*)&current);

#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
            printf("current: %d\r\n", current);
#endif
        }
    }
}

static void get_time_data() {

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand get time\r\n");
#endif

    uint8_t *ptr;

    type_octet_string_t *present_date = (type_octet_string_t*)get_request_data(&attr_descriptor_time);

    if (present_date) {

        if (present_date->type == TYPE_OCTET_STRING) {

            ptr = &present_date->str;
            present_year = (uint16_t)((*ptr++) << 8);
            present_year |= *ptr++;
            present_month = *ptr++;

#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
            printf("year: %d, mon: %d\r\n", present_year, present_month);
#endif
        }
    }
}

static uint32_t get_tariffA(request_t *attr_desc) {

    uint32_t tariff = 0;
    type_digit_t *tariff_A = (type_digit_t*)get_request_data(attr_desc);

    if (tariff_A->type == TYPE_UNSIGNED_32) {
        tariff = reverse32(tariff_A->value);
    }

//    sleep_ms(300);

    return tariff;
}

static void get_tariffs_1_2_data() {

    uint64_t tariff;
    uint32_t tariffP, tariffM;

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand get 1 and 2 tariffs\r\n");
#endif

    tariff_summ = 0;

    tariffP = get_tariffA(&attr_descriptor_tariff1AP);
    tariffM = get_tariffA(&attr_descriptor_tariff1AM);

    tariff = (tariffP + tariffM) & 0xffffffffffff;

    tariff_summ += tariff;

    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_1_SUMMATION_DELIVERD, (uint8_t*)&tariff);

#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
    printf("tariff1: %d\r\n", tariff);
#endif

    tariffP = get_tariffA(&attr_descriptor_tariff2AP);
    tariffM = get_tariffA(&attr_descriptor_tariff2AM);

    tariff = (tariffP + tariffM) & 0xffffffffffff;

    tariff_summ += tariff;

    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_2_SUMMATION_DELIVERD, (uint8_t*)&tariff);

#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
    printf("tariff2: %d\r\n", tariff);
#endif

}

static void get_tariffs_3_4_data() {

    uint64_t tariff;
    uint32_t tariffP, tariffM;

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand get 3 adn 4 tariffs\r\n");
#endif

    tariffP = get_tariffA(&attr_descriptor_tariff3AP);
    tariffM = get_tariffA(&attr_descriptor_tariff3AM);

    tariff = (tariffP + tariffM) & 0xffffffffffff;

    tariff_summ += tariff;

    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_3_SUMMATION_DELIVERD, (uint8_t*)&tariff);

#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
    printf("tariff3: %d\r\n", tariff);
#endif

    tariffP = get_tariffA(&attr_descriptor_tariff4AP);
    tariffM = get_tariffA(&attr_descriptor_tariff4AM);

    tariff = (tariffP + tariffM) & 0xffffffffffff;

    tariff_summ += tariff;

    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_4_SUMMATION_DELIVERD, (uint8_t*)&tariff);

#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
    printf("tariff4: %d\r\n", tariff);
#endif

    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_SUMMATION_DELIVERD, (uint8_t*)&tariff_summ);
#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
    printf("tariff_summ: %d\r\n", tariff_summ);
#endif

}

static void get_resbat_data() {

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand get resource of battery\r\n");
#endif

    uint8_t worktime, lifetime = RESOURCE_BATTERY;


    worktime = lifetime - ((present_year * 12 + present_month) - (release_year * 12 + release_month));

    uint8_t battery_level = (worktime*100)/lifetime;

    if (((worktime*100)%lifetime) >= (lifetime/2)) {
        battery_level++;
    }

    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_REMAINING_BATTERY_LIFE, (uint8_t*)&battery_level);

#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
    printf("Resource battery: %d.%d%%\r\n", (worktime*100)/lifetime, ((worktime*100)%lifetime)*100/lifetime);
#endif

}

void nartis100_init() {
    memset(&meter, 0, sizeof(meter_t));

    meter.client_addr = CLIENT_ADDRESS;
    meter.server_upper_addr = LOGICAL_DEVICE;
    meter.server_lower_addr = PHY_DEVICE;
    meter.max_info_field_rx = MAX_INFO_FIELD;
    meter.max_info_field_tx = MAX_INFO_FIELD;
    meter.window_rx = 1;
    meter.window_tx = 1;
    meter.format.type = TYPE3;
    if (dev_config.device_password.size) {
        memcpy(&meter.password, &dev_config.device_password, sizeof(m_password_t));
    } else {
        strcpy((char*)meter.password.data, PASSWORD);
        meter.password.size = sizeof(PASSWORD)-1;
    }
    //printf("size: %d, meter password: %s\r\n", meter.password.size, meter.password.data);
//    memcpy(&meter.password, PASSWORD, sizeof(PASSWORD));
}

static uint8_t measure_meter_nartis_100_1() {

    send_cmd_snrm();                    /* start connection                             */

    uint8_t ret = send_cmd_open_session();

    if (ret) {
        if (new_start) {                /* after reset                                  */
            serial_number[0] = 0;
            date_release[0] = 0;
            new_start = false;
        }
        if (serial_number[0] == 0) {
            get_serial_number_data();
        }
        if (date_release[0] == 0) {
            get_date_release_data();
        }

        get_voltage_data();
        get_power_data();
        get_current_data();
        get_time_data();

        get_resbat_data();              /* get resource battery                         */
        send_cmd_disc();                /* disconnect                                   */

        fault_measure_flag = false;
    } else {
        fault_measure_flag = true;
        if (!timerFaultMeasurementEvt) {
            timerFaultMeasurementEvt = TL_ZB_TIMER_SCHEDULE(fault_measure_meterCb, NULL, TIMEOUT_10MIN);
        }
    }

    return ret;
}

static uint8_t measure_meter_nartis_100_2() {

    send_cmd_snrm();                    /* start connection                             */

    uint8_t ret = send_cmd_open_session();

    if (ret) {

        get_tariffs_1_2_data();
        send_cmd_disc();                /* disconnect                                   */

        fault_measure_flag = false;
    } else {
        fault_measure_flag = true;
        if (!timerFaultMeasurementEvt) {
            timerFaultMeasurementEvt = TL_ZB_TIMER_SCHEDULE(fault_measure_meterCb, NULL, TIMEOUT_10MIN);
        }
    }

    return ret;
}

static uint8_t measure_meter_nartis_100_3() {

    send_cmd_snrm();                    /* start connection                             */

    uint8_t ret = send_cmd_open_session();

    if (ret) {

        get_tariffs_3_4_data();
        send_cmd_disc();                /* disconnect                                   */

        fault_measure_flag = false;
    } else {
        fault_measure_flag = true;
        if (!timerFaultMeasurementEvt) {
            timerFaultMeasurementEvt = TL_ZB_TIMER_SCHEDULE(fault_measure_meterCb, NULL, TIMEOUT_10MIN);
        }
    }

    return ret;
}

uint8_t measure_meter_nartis_100() {

    uint8_t ret;

    ret = measure_meter_nartis_100_1();
    ret = measure_meter_nartis_100_2();
    ret = measure_meter_nartis_100_3();

    return ret;
}

