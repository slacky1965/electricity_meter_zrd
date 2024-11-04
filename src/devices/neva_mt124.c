#include "tl_common.h"
#include "zcl_include.h"

#include "se_custom_attr.h"
#include "app_uart.h"
#include "app_endpoint_cfg.h"
#include "app_dev_config.h"
#include "device.h"
#include "neva_mt124.h"

#define ACK             0x06
#define SOH             0x01
#define STX             0x02
#define ETX             0x03

#define BAUDRATE_300    300
#define BAUDRATE_9600   9600

#define MAX_VBAT_MV 3100                        /* 3100 mV - > battery = 100%         */
#define MIN_VBAT_MV BATTERY_SAFETY_THRESHOLD    /* 2200 mV - > battery = 0%           */

static uint32_t baudrate = BAUDRATE_300;

static uint8_t package_buff[PKT_BUFF_MAX_LEN];
static uint32_t divisor = 1;
static uint16_t multiplier = 1;

/*
 *  cmd_open_channel = 0,
    cmd_ack_start,
    cmd_password_6102,
    cmd_password_7109,
    cmd_serial_number,
    cmd_sensors_data,
    cmd_tariffs_6102,
    cmd_tariffs_7109,
    cmd_power_data,
    cmd_volts_data,
    cmd_amps_data,
    cmd_close_channel,
    cmd_max
 *
 */
static const uint8_t command_array[cmd_max][32] = {
        {0x2F, 0x3F, 0x21, 0x0D, 0x0A, 0x00},                                                                   /* open channel      */
        {ACK, 0x30, 0x35, 0x31, 0x0D, 0x0A, 0x00},                                                              /* changing baudrate */
        {SOH, 0x50, 0x31, STX, 0x28, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x29, ETX, 0x61, 0x00},    /* password_6102     */
        {SOH, 0x50, 0x31, STX, 0x28, 0x29, ETX,  0x61, 00},                                                     /* password_7109     */
        {SOH, 0x52, 0x31, STX, 0x36, 0x30, 0x30, 0x31, 0x30, 0x30, 0x46, 0x46, 0x28, 0x29, ETX, 0x64, 0x00},    /* serial number     */
        {SOH, 0x52, 0x31, STX, 0x36, 0x30, 0x30, 0x35, 0x30, 0x30, 0x46, 0x46, 0x28, 0x29, ETX, 0x60, 0x00},    /* sensors           */
        {SOH, 0x52, 0x31, STX, 0x30, 0x46, 0x30, 0x38, 0x38, 0x30, 0x46, 0x46, 0x28, 0x29, ETX, 0x15, 0x00},    /* tariff_6102       */
        {SOH, 0x52, 0x31, STX, 0x30, 0x46, 0x30, 0x38, 0x38, 0x30, 0x46, 0x46, 0x28, 0x53, 0x29, ETX, 0x46, 0x00},/* tariffs_7109    */
        {SOH, 0x52, 0x31, STX, 0x31, 0x30, 0x30, 0x37, 0x30, 0x30, 0x46, 0x46, 0x28, 0x29, ETX, 0x65, 0x00},    /* power             */
        {SOH, 0x52, 0x31, STX, 0x30, 0x43, 0x30, 0x37, 0x30, 0x30, 0x46, 0x46, 0x28, 0x29, ETX, 0x17, 0x00},    /* voltage           */
        {SOH, 0x52, 0x31, STX, 0x30, 0x42, 0x30, 0x37, 0x30, 0x30, 0x46, 0x46, 0x28, 0x29, ETX, 0x16, 0x00},    /* amperes           */
        {SOH, 0x42, 0x30, ETX, 0x71, 0x00},                                                                     /* close channel     */
        };

static neva_124_type_t neva_124_type = HEBA_124_UNKNOWN;

static uint8_t date_release[DATA_MAX_LEN+2] = {0};
static uint8_t serial_number[SE_ATTR_SN_SIZE] = {0};

static uint8_t get_password_6102();
static uint8_t get_password_7109();


static uint8_t checksum(const uint8_t *src_buffer, uint8_t len) {
    uint8_t crc = 0;

    len--;

    for (uint8_t i = 1; i < len; i++) {
        crc ^= src_buffer[i];
    }

    return crc & 0x7f;
}

static size_t sizeof_str(const uint8_t *str) {

    size_t len = 0;

    while(1) {
        if (str[len] == 0) break;
        len++;
    }

    return len;
}

static uint32_t str2uint(const uint8_t* str) {
    uint32_t num = 0;
    uint8_t i = 0;
    while (str[i] && (str[i] >= '0' && str[i] <= '9')){
        num = num * 10 + (str[i] - '0');
        i++;
    }
    return num;
}

static uint32_t number_from_brackets(uint8_t **p_str) {

    uint32_t value = 0;
    uint8_t *remainder, *integer;
    uint8_t *init_bracket, *end_bracket, *point;

    init_bracket = *p_str;

    divisor = 1;
    multiplier = 1;

    while (*init_bracket != '(' && *init_bracket != 0) {
        init_bracket++;
    }
    if (*(init_bracket++) == '(') {
        integer = end_bracket = init_bracket;
        while (*end_bracket != ')' && *end_bracket != 0x0d && *end_bracket != 0) {
            end_bracket++;
        }
        if (*end_bracket == ')') {
            *(end_bracket++) = 0;
            value = str2uint(integer);
//            printf("value: %d\r\n", value);
            point = init_bracket;
            while (*point != '.' && *point != 0) {
                point++;
            }
            if (*point == '.') {
                *(point++) = 0;
                remainder = point;
                size_t rmndr_len = sizeof_str(remainder);
                if (rmndr_len > 4) {
                    rmndr_len = 4;
                }
                remainder[rmndr_len] = 0;
                if (rmndr_len != 0) {
                    for (uint8_t i = 0; i < rmndr_len; i++) {
                        value *= 10;
                        divisor *= 10;
                    }
//                    printf("remainder: %d\r\n", str2uint(remainder));
                    value += str2uint(remainder);
//                    printf("value: %d, divisor: %d\r\n", value, divisor);
                }
            }
            *p_str = end_bracket;
        } else {
            *p_str = NULL;
        }
    } else {
        *p_str = NULL;
    }

    return value;

}

static uint32_t number_from_tariffs(uint8_t **p_str) {

    uint32_t value = 0;
    uint8_t *remainder, *integer;
    uint8_t *init_bracket, *end_bracket, *point;

    init_bracket = *p_str;

    divisor = 1;
    multiplier = 1;

    integer = end_bracket = init_bracket;
    while (*end_bracket != ',' && *end_bracket != ')' && *end_bracket != 0x0d && *end_bracket != 0) {
        end_bracket++;
    }
    if (*end_bracket == ',' || *end_bracket == ')') {
        *(end_bracket++) = 0;
        value = str2uint(integer);
//            printf("value: %d\r\n", value);
        point = init_bracket;
        while (*point != '.' && *point != 0) {
            point++;
        }
        if (*point == '.') {
            *(point++) = 0;
            remainder = point;
            size_t rmndr_len = sizeof_str(remainder);
            if (rmndr_len > 4) {
                rmndr_len = 4;
            }
            remainder[rmndr_len] = 0;
            if (rmndr_len != 0) {
                for (uint8_t i = 0; i < rmndr_len; i++) {
                    value *= 10;
                    divisor *= 10;
                }
//                    printf("remainder: %d\r\n", str2uint(remainder));
                value += str2uint(remainder);
//                    printf("value: %d, divisor: %d\r\n", value, divisor);
            }
        }
        *p_str = end_bracket;
    } else {
        *p_str = NULL;
    }


//    if (*(init_bracket++) == ',') {
//    } else {
//        *p_str = NULL;
//    }

    return value;

}

static uint8_t *str_from_brackets(uint8_t *p_str) {

    uint8_t *init_bracket = p_str, *end_bracket, *str = NULL;

    while (*init_bracket != '(' && *init_bracket != 0) {
        init_bracket++;
    }

    if (*(init_bracket++) == '(') {
        end_bracket = init_bracket;
        while (*end_bracket != ')' && *end_bracket != 0x0d && *end_bracket != 0) {
            end_bracket++;
        }
        if (*end_bracket == ')') {
            *end_bracket = 0;
            str = init_bracket;
        }
    }

    return str;
}

static uint8_t check_even_parity(uint8_t ch) {

         ch ^= ch >> 4;
         ch ^= ch >> 2;
         ch ^= ch >> 1;

         return ch & 1;
}

static size_t send_command(command_t command) {

    size_t len, size = sizeof_str(command_array[command]);
    uint8_t ch;

    app_uart_rx_off();
    flush_buff_uart();


    for(uint8_t i = 0; i < size; i++) {
        ch = command_array[command][i];
        if (check_even_parity(ch)) {
            ch |= 0x80;
        }
        package_buff[i] = ch;
    }

    /* three attempts to write to uart */
    for (uint8_t attempt = 0; attempt < 3; attempt++) {
        len = write_bytes_to_uart(package_buff, size);
        if (len == size) {
            break;
        } else {
            len = 0;
        }
#if UART_PRINTF_MODE && DEBUG_PACKAGE
        printf("Attempt to send data to uart: %d\r\n", attempt+1);
#endif
        sleep_ms(250);
    }


    if (baudrate == BAUDRATE_300) {
        sleep_ms(200);
    } else {
        sleep_ms(30);
    }
    app_uart_rx_on(baudrate);
    sleep_ms(100);


#if UART_PRINTF_MODE && DEBUG_PACKAGE
    if (len == 0) {
        uint8_t head[] = "write to uart error";
        print_package(head, package_buff, size);
    } else {
        uint8_t head[] = "write to uart";
        print_package(head, package_buff, len);
    }
#endif

    return len;
}

static pkt_error_t response_meter(command_t command) {

    size_t load_size;
    pkt_error_no = PKT_ERR_TIMEOUT;

    /* trying to read for 1 seconds */
    for(uint8_t i = 0; i < 100; i++ ) {
        load_size = 0;
        if (available_buff_uart()) {
            while (available_buff_uart() && load_size < PKT_BUFF_MAX_LEN) {
                package_buff[load_size++] = read_byte_from_buff_uart() & 0x7f;
                if (!available_buff_uart()) {
                    i = 100;
                    pkt_error_no = PKT_OK;
                    package_buff[load_size] = 0;
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
        for (uint16_t i = 0; i < load_size; i++) {
            printf("%c", package_buff[i]);
        }
        printf("\r\n");
#endif

        if (command == cmd_open_channel) {
            if (package_buff[0] == 0x2f) {
                pkt_error_no = PKT_OK;
            } else {
                pkt_error_no = PKT_ERR_RESPONSE;
            }
        } else if (command == cmd_password_6102) {
            if (package_buff[0] == ACK) {
                pkt_error_no = PKT_OK;
            } else {
                pkt_error_no = PKT_ERR_RESPONSE;
            }
        } else {
            uint8_t crc = checksum(package_buff, load_size);
            if (crc == package_buff[load_size-1]) {
                if (command == cmd_ack_start) {
                    if (package_buff[0] == SOH) {
                        pkt_error_no = PKT_OK;
                    } else {
                        pkt_error_no = PKT_ERR_RESPONSE;
                    }
                } else if (command == cmd_tariffs_6102  ||
                           command == cmd_tariffs_7109  ||
                           command == cmd_power_data    ||
                           command == cmd_volts_data    ||
                           command == cmd_amps_data     ||
                           command == cmd_sensors_data  ||
                           command == cmd_serial_number ||
                           command == cmd_password_7109) {
                    if (package_buff[0] == STX) {
                        pkt_error_no = PKT_OK;
                    } else {
                        pkt_error_no = PKT_ERR_RESPONSE;
                    }
                } else if(command == cmd_password_6102) {
                    if (package_buff[0] == ACK) {
                        pkt_error_no = PKT_OK;
                    } else {
                        pkt_error_no = PKT_ERR_RESPONSE;
                    }
                } else {
                    pkt_error_no = PKT_ERR_RESPONSE;
                }
            } else {
                pkt_error_no = PKT_ERR_CRC;
            }

        }
        if (command != cmd_open_channel && command != cmd_password_6102) {
            uint8_t crc = checksum(package_buff, load_size);
            if (crc == package_buff[load_size-1]) {
                pkt_error_no = PKT_OK;
            } else {
                pkt_error_no = PKT_ERR_CRC;
            }
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

    uint8_t *p_str;
    uint32_t type = 0;

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand open of channel\r\n");
#endif

    if (send_command(cmd_open_channel)) {
        if (response_meter(cmd_open_channel) == PKT_OK) {

            p_str = package_buff;
            while(*p_str != '.' && *p_str != 0x0d && *p_str != 0) {
                p_str++;
            }

            if (*p_str == '.') {
                p_str++;
                type =  (*p_str++ - 0x30)*1000;
                type += (*p_str++ - 0x30)*100;
                type += (*p_str++ - 0x30)*10;
                type += *p_str - 0x30;
            }

            switch (type) {
                case TYPE_6102:
                    neva_124_type = HEBA_124_6102;
                    break;
                case TYPE_7109:
                    neva_124_type = HEBA_124_7109;
                    break;
                default:
                    neva_124_type = HEBA_124_UNKNOWN;
                    break;
            }

            return true;
        }
    }

    return false;
}

static uint8_t ack_start() {

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand start confirmation\r\n");
#endif

    if (send_command(cmd_ack_start)) {
        baudrate = BAUDRATE_9600;
        app_uart_init(baudrate);
        sleep_ms(200);
        if (response_meter(cmd_ack_start) == PKT_OK) {
            uint8_t ret;
            switch (neva_124_type) {
                case HEBA_124_6102:
                    ret = get_password_6102();
                    break;
                case HEBA_124_7109:
                    ret = get_password_7109();
                    break;
                default:
                    neva_124_type = HEBA_124_UNKNOWN;
                    break;
            }
            return true;
        }
    }

    return false;
}

static uint8_t get_password_6102() {

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand get_password_6102\r\n");
#endif

    if (send_command(cmd_password_6102)) {
        if (response_meter(cmd_password_6102) == PKT_OK) {
            return true;
        }
    }

    return false;
}

static uint8_t get_password_7109() {

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand get_password_7109\r\n");
#endif

    if (send_command(cmd_password_7109)) {
        if (response_meter(cmd_password_7109) == PKT_OK) {
            return true;
        }
    }

    return false;
}

static void close_session() {

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand close_session\r\n");
#endif

    send_command(cmd_close_channel);

}


static void get_tariffs_7109() {

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand get tariffs\r\n");
#endif

    uint64_t tariff_summ = 0;

//    uint8_t buff[] = "0F0880FF([231006:232957]01242.39,00401.98,01234.99,05678.88)";

    if (send_command(cmd_tariffs_7109)) {
        if (response_meter(cmd_tariffs_7109) == PKT_OK) {
            uint8_t *p_str = package_buff;
//            uint8_t *p_str = buff;

            while(*p_str != ']' && *p_str != 0) {
                p_str++;
            }
            if (*(p_str++) == ']') {
                uint64_t tariff = number_from_tariffs(&p_str) & 0xffffffffffff;
                if (p_str) {
                    uint32_t energy_divisor = divisor;

                    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_DIVISOR, (uint8_t*)&energy_divisor);

                    tariff_summ += tariff;
                    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_1_SUMMATION_DELIVERD, (uint8_t*)&tariff);

#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
                    printf("tariff1: %d\r\n", tariff);
#endif

                    tariff = number_from_tariffs(&p_str) & 0xffffffffffff;
                    if (p_str) {
                        tariff_summ += tariff;
                        zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_2_SUMMATION_DELIVERD, (uint8_t*)&tariff);

#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
                        printf("tariff2: %d\r\n", tariff);
#endif
                        tariff = number_from_tariffs(&p_str) & 0xffffffffffff;
                        if (p_str) {
                            tariff_summ += tariff;
                            zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_3_SUMMATION_DELIVERD, (uint8_t*)&tariff);

#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
                            printf("tariff3: %d\r\n", tariff);
#endif
                            tariff = number_from_tariffs(&p_str) & 0xffffffffffff;
                            if (p_str) {
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
                        }
                    }
                }
            }
        }
    }
}


static void get_tariffs_6102() {

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand get tariffs\r\n");
#endif

//    uint8_t buff[] = "0F0880FF(008030.59,002236.84,002575.35,003218.40,000000.00)"

    if (send_command(cmd_tariffs_6102)) {
        if (response_meter(cmd_tariffs_6102) == PKT_OK) {
            uint8_t *p_str = package_buff;

            while(*p_str != '(' && *p_str != 0) {
                p_str++;
            }
            if (*(p_str++) == '(') {
                uint64_t tariff_summ = number_from_tariffs(&p_str) & 0xffffffffffff;
                uint64_t tariff;// = number_from_tariffs(&p_str) & 0xffffffffffff;

                if (p_str) {
                    uint32_t energy_divisor = divisor;

                    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_DIVISOR, (uint8_t*)&energy_divisor);

                    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_SUMMATION_DELIVERD, (uint8_t*)&tariff_summ);
#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
                    printf("tariff_summ: %d\r\n", tariff_summ);
#endif

                    tariff = number_from_tariffs(&p_str) & 0xffffffffffff;
                    if (p_str) {
                        zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_1_SUMMATION_DELIVERD, (uint8_t*)&tariff);

#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
                        printf("tariff1: %d\r\n", tariff);
#endif
                        tariff = number_from_tariffs(&p_str) & 0xffffffffffff;
                        if (p_str) {
                            zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_2_SUMMATION_DELIVERD, (uint8_t*)&tariff);
#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
                            printf("tariff2: %d\r\n", tariff);
#endif
                            tariff = number_from_tariffs(&p_str) & 0xffffffffffff;
                            if (p_str) {
                                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_3_SUMMATION_DELIVERD, (uint8_t*)&tariff);
#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
                                printf("tariff3: %d\r\n", tariff);
#endif
                                tariff = number_from_tariffs(&p_str) & 0xffffffffffff;
                                if (p_str) {
                                    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_4_SUMMATION_DELIVERD, (uint8_t*)&tariff);
#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
                                    printf("tariff4: %d\r\n", tariff);
#endif
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

static void get_voltage_data() {

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand get voltage\r\n");
#endif

    if (send_command(cmd_volts_data)) {
        if (response_meter(cmd_volts_data) == PKT_OK) {
            uint8_t *p_str = package_buff;
            uint16_t volts = number_from_brackets(&p_str);
            if (p_str) {

                uint16_t voltage_divisor = divisor & 0xffff;

                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_AC_VOLTAGE_DIVISOR, (uint8_t*)&voltage_divisor);

                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_RMS_VOLTAGE, (uint8_t*)&volts);

#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
                printf("voltage: %d\r\n", volts);
#endif
            }
        }
    }
}

static void get_amps_data() {

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand get current\r\n");
#endif

//    static uint16_t count = 0;
//    uint8_t buff[] = "CURRE(9.472)\0";

    if (send_command(cmd_amps_data)) {
        if (response_meter(cmd_amps_data) == PKT_OK) {
            uint8_t *p_str = package_buff;
//            uint8_t *p_str = buff;
            uint16_t amps = number_from_brackets(&p_str);
            if (p_str) {

                uint16_t current_divisor = divisor & 0xffff;

//                amps -= count++;

                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_AC_CURRENT_DIVISOR, (uint8_t*)&current_divisor);


                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_LINE_CURRENT, (uint8_t*)&amps);

#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
                printf("amps: %d\r\n", amps);
#endif
            }
        }
    }
}

static void get_power_data() {

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand get power\r\n");
#endif

//    uint8_t buff[] = "100700FF(00001356)"; //"100700FF(00025.45)";

    if (send_command(cmd_power_data)) {
        if (response_meter(cmd_power_data) == PKT_OK) {
            uint8_t *p_str = package_buff;
//            uint8_t *p_str = buff;

            uint32_t power = number_from_brackets(&p_str);
            if (p_str) {


                multiplier = 1;

//                printf("power: %d, multiplier: %d, divisor: %d\r\n", power, multiplier, divisor);

                if (neva_124_type == HEBA_124_6102) {
                    divisor = 1000;
                    if (power > 0xffff) {
                        power /= 10;
                        divisor /= 10;
                    }
                } else {
                    if (power != 0) {
                        power /= 100;
                        divisor = 1000;
                        if (power > 0xffff) {
                            power /= 10;
                            divisor /= 10;
                        }
                    } else {
                        divisor = 1;
                    }
                }

//                printf("power: %d, multiplier: %d, divisor: %d\r\n", power, multiplier, divisor);

                uint16_t pwr = power & 0xffff;
                uint16_t power_divisor = divisor & 0xffff;

                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_AC_POWER_DIVISOR, (uint8_t*)&power_divisor);
                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_AC_POWER_MULTIPLIER, (uint8_t*)&multiplier);

                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_APPARENT_POWER, (uint8_t*)&pwr);

#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
                printf("power: %d\r\n", pwr);
#endif
            }
        }
    }
}

static void get_serial_number_data() {

    uint8_t *sn, *p_str;

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand get serial number\r\n");
#endif

    if (send_command(cmd_serial_number)) {
        if (response_meter(cmd_serial_number) == PKT_OK) {
            p_str = package_buff;
            sn = str_from_brackets(p_str);
            if (sn) {
                if (set_zcl_str(sn, serial_number, SE_ATTR_SN_SIZE)) {
                    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_METER_SERIAL_NUMBER, (uint8_t*)&serial_number);
#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
                    printf("Serial Number: %s, len: %d\r\n", serial_number+1, *serial_number);
#endif
                }
            }
        }
    }

}


static void get_date_release_data() {
    uint8_t dr[] = "Not supported";

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand get date release\r\n");
#endif

    if (set_zcl_str(dr, date_release, DATA_MAX_LEN+1)) {
        zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CUSTOM_DATE_RELEASE, (uint8_t*)&date_release);
#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
        printf("Date of release: %s, len: %d\r\n", date_release+1, *date_release);
#endif
    }
}

static void get_resbat_data() {

#if UART_PRINTF_MODE
    printf("\r\nCommand get resource of battery\r\n");
#endif

    uint16_t battery_mv = 0;

    if (send_command(cmd_sensors_data)) {
        if (response_meter(cmd_sensors_data) == PKT_OK) {
            uint8_t *p_str = package_buff;

            while (*p_str != '(' && *p_str != 0) {
                p_str++;
            }
            if (*(p_str++) == '(') {
                while (*p_str != ',' && *p_str != 0) {
                    p_str++;
                }
                if (*(p_str++) == ',') {
                    battery_mv = str2uint(p_str++) * 1000;
                    battery_mv += str2uint(++p_str) * 10;
                    if (battery_mv < MIN_VBAT_MV) battery_mv = MIN_VBAT_MV;
                    uint8_t battery_level = (battery_mv - MIN_VBAT_MV) / ((MAX_VBAT_MV - MIN_VBAT_MV) / 100);
                    if (battery_level > 100) battery_level = 100;
                    zcl_getAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_REMAINING_BATTERY_LIFE, &attr_len, (uint8_t*)&attr_data);
                    uint8_t last_bl = fromPtoInteger(attr_len, attr_data);

                    if (battery_level != last_bl) {
                        zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_REMAINING_BATTERY_LIFE, (uint8_t*)&battery_level);
                    }

#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
                    printf("Resource battery: %d%%\r\n", battery_level);
#endif
                }
            }
        }
    }



}

static void get_data_6102() {
    if (new_start) {
        new_start = false;
    }

    if (serial_number[0] == 0) {
        get_serial_number_data();
    }
    if (date_release[0] == 0) {
        get_date_release_data();
    }

//    get_resbat_data(); // status not support
    get_tariffs_6102();
    get_power_data();
    get_amps_data();
    get_voltage_data();
}


static void get_data_7109() {
    if (new_start) {
        new_start = false;
    }

    if (serial_number[0] == 0) {
        get_serial_number_data();
    }
    if (date_release[0] == 0) {
        get_date_release_data();
    }

    get_resbat_data();
    get_tariffs_7109();
    get_power_data();
}


uint8_t measure_meter_neva_mt124() {

    baudrate = BAUDRATE_300;

    app_uart_init(baudrate);

    uint8_t ret = open_session();

    if (ret) {
        ret = ack_start();
        if (ret) {
            switch(neva_124_type) {
                case HEBA_124_6102:
                    get_data_6102();
                    break;
                case HEBA_124_7109:
                    get_data_7109();
                    break;
                default:
#if UART_PRINTF_MODE
                    printf("This version of HEBA-124 is not supported\r\n");
#endif
                    break;
            }
            fault_measure_flag = false;
        }
        close_session();
    } else {
        baudrate = BAUDRATE_9600;
        app_uart_init(baudrate);
        close_session();
        fault_measure_flag = true;
        if (!timerFaultMeasurementEvt) {
            timerFaultMeasurementEvt = TL_ZB_TIMER_SCHEDULE(fault_measure_meterCb, NULL, /*TIMEOUT_30SEC*/TIMEOUT_10MIN);
        }
    }

    return ret;
}

