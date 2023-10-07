#include "tl_common.h"
#include "zcl_include.h"

#include "se_custom_attr.h"
#include "app_uart.h"
#include "app_endpoint_cfg.h"
#include "app_dev_config.h"
#include "device.h"
#include "neva_mt124.h"

#define ACK         0x06
#define SOH         0x01
#define STX         0x02
#define ETX         0x03

#define MAX_VBAT_MV 3100                        /* 3100 mV - > battery = 100%         */
#define MIN_VBAT_MV BATTERY_SAFETY_THRESHOLD    /* 2200 mV - > battery = 0%           */

static u8 package_buff[PKT_BUFF_MAX_LEN];
static u32 divisor = 1;
static u16 multiplier = 1;

static const u8 cmd0[] = {0x2F, 0x3F, 0x21, 0x0D, 0x0A, 0x00};                                                                  /* open channel */
static const u8 cmd1[] = {ACK, 0x30, 0x35, 0x31, 0x0D, 0x0A, 0x00};                                                             /* changing baudrate */
static const u8 cmd2[] = {SOH, 0x52, 0x31, STX, 0x30, 0x46, 0x30, 0x38, 0x38, 0x30, 0x46, 0x46, 0x28, 0x53, 0x29, ETX, 0x46, 0x00};   /* tariffs  */
static const u8 cmd3[] = {SOH, 0x52, 0x31, STX, 0x31, 0x30, 0x30, 0x37, 0x30, 0x30, 0x46, 0x46, 0x28, 0x29, ETX, 0x65, 0x00};   /* power    */
static const u8 cmd4[] = {SOH, 0x52, 0x31, STX, 0x36, 0x30, 0x30, 0x31, 0x30, 0x30, 0x46, 0x46, 0x28, 0x29, ETX, 0x64, 0x00};   /* serial number */
static const u8 cmd5[] = {SOH, 0x52, 0x31, STX, 0x36, 0x30, 0x30, 0x35, 0x30, 0x30, 0x46, 0x46, 0x28, 0x29, ETX, 0x60, 0x00};   /* sensors */

static const u8* command_array[] = {cmd0, cmd1, cmd2, cmd3, cmd4, cmd5};

static u8 checksum(const u8 *src_buffer, u8 len) {
    u8 crc = 0;

    len--;

    for (u8 i = 1; i < len; i++) {
        crc ^= src_buffer[i];
    }

    return crc & 0x7f;
}

static size_t sizeof_str(const u8 *str) {

    size_t len = 0;

    while(1) {
        if (str[len] == 0) break;
        len++;
    }

    return len;
}

static u32 str2uint(const u8* str) {
    u32 num = 0;
    u8 i = 0;
    while (str[i] && (str[i] >= '0' && str[i] <= '9')){
        num = num * 10 + (str[i] - '0');
        i++;
    }
    return num;
}

static u32 number_from_brackets(u8 **p_str) {

    u32 value = 0;
    u8 *remainder, *integer;
    u8 *init_bracket, *end_bracket, *point;

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
                    for (u8 i = 0; i < rmndr_len; i++) {
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

static u32 number_from_tariffs(u8 **p_str) {

    u32 value = 0;
    u8 *remainder, *integer;
    u8 *init_bracket, *end_bracket, *point;

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
                for (u8 i = 0; i < rmndr_len; i++) {
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

static u8 *str_from_brackets(u8 *p_str) {

    u8 *init_bracket = p_str, *end_bracket, *str = NULL;

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

static u8 check_even_parity(u8 ch) {

         ch ^= ch >> 4;
         ch ^= ch >> 2;
         ch ^= ch >> 1;

         return ch & 1;
}

static size_t send_command(command_t command) {

    size_t len, size = sizeof_str(command_array[command]);
    u8 ch;

//    app_uart_rx_off();
    flush_buff_uart();


    for(u8 i = 0; i < size; i++) {
        ch = command_array[command][i];
        if (check_even_parity(ch)) {
            ch |= 0x80;
        }
        package_buff[i] = ch;
    }

    /* three attempts to write to uart */
    for (u8 attempt = 0; attempt < 3; attempt++) {
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


    sleep_ms(200);
//    app_uart_rx_on();

#if UART_PRINTF_MODE && DEBUG_PACKAGE
    if (len == 0) {
        u8 head[] = "write to uart error";
        print_package(head, package_buff, size);
    } else {
        u8 head[] = "write to uart";
        print_package(head, package_buff, len);
    }
#endif

    return len;
}

static pkt_error_t response_meter(command_t command) {

    size_t load_size;
    pkt_error_no = PKT_ERR_TIMEOUT;

    /* trying to read for 1 seconds */
    for(u8 i = 0; i < 100; i++ ) {
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
        u8 head[] = "read from uart";
        print_package(head, package_buff, load_size);
        for (u16 i = 0; i < load_size; i++) {
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
        } else {
            u8 crc = checksum(package_buff, load_size);
            if (crc == package_buff[load_size-1]) {
                if (command == cmd_ack_start) {
                    if (package_buff[0] == SOH) {
                        pkt_error_no = PKT_OK;
                    } else {
                        pkt_error_no = PKT_ERR_RESPONSE;
                    }
                } else if (command == cmd_tariffs_data ||
                           command == cmd_power_data ||
                           command == cmd_sensors_data ||
                           command == cmd_serial_number) {
                    if (package_buff[0] == STX) {
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
        if (command != cmd_open_channel) {
            u8 crc = checksum(package_buff, load_size);
            if (crc == package_buff[load_size-1]) {
                pkt_error_no = PKT_OK;
            } else {
                pkt_error_no = PKT_ERR_CRC;
            }
        }
    } else {
#if UART_PRINTF_MODE && DEBUG_PACKAGE
        u8 head[] = "read from uart error";
        print_package(head, package_buff, load_size);
#endif
    }

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    if (pkt_error_no != PKT_OK) print_error(pkt_error_no);
#endif

    return pkt_error_no;
}



static u8 open_session() {

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand open of channel\r\n");
#endif

    if (send_command(cmd_open_channel)) {
        if (response_meter(cmd_open_channel) == PKT_OK) {
            return true;
        }
    }

    return false;
}

static u8 ack_start() {

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand start confirmation\r\n");
#endif

    if (send_command(cmd_ack_start)) {
        app_uart_init(9600);
        sleep_ms(200);
        if (response_meter(cmd_ack_start) == PKT_OK) {
            return true;
        }
    }

    return false;
}

static u8 get_sensors_data() {

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand get sensors\r\n");
#endif

    if (send_command(cmd_sensors_data)) {
        if (response_meter(cmd_sensors_data) == PKT_OK) {
            return true;
        }
    }

    return false;
}

static void get_tariffs_data() {

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand get tariffs\r\n");
#endif

//    u8 buff[] = "0F0880FF([231006:232957]01242.39,00401.98,01234.99,05678.88)";

    if (send_command(cmd_tariffs_data)) {
        if (response_meter(cmd_tariffs_data) == PKT_OK) {
            u8 *p_str = package_buff;
//            u8 *p_str = buff;

            while(*p_str != ']' && *p_str != 0) {
                p_str++;
            }
            if (*(p_str++) == ']') {
                u64 tariff = number_from_tariffs(&p_str) & 0xffffffffffff;
                u64 last_tariff;

                if (p_str) {
                    u32 energy_divisor = divisor;

                    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_DIVISOR, (u8*)&energy_divisor);

                    zcl_getAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_1_SUMMATION_DELIVERD, &attr_len, (u8*)&attr_data);
                    last_tariff = fromPtoInteger(attr_len, attr_data);

                    if (tariff > last_tariff) {
                        zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_1_SUMMATION_DELIVERD, (u8*)&tariff);
                    }
#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
                    printf("tariff1: %d\r\n", tariff);
#endif

                    tariff = number_from_tariffs(&p_str) & 0xffffffffffff;
                    if (p_str) {
                        zcl_getAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_2_SUMMATION_DELIVERD, &attr_len, (u8*)&attr_data);
                        last_tariff = fromPtoInteger(attr_len, attr_data);

                        if (tariff > last_tariff) {
                            zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_2_SUMMATION_DELIVERD, (u8*)&tariff);
                        }

#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
                        printf("tariff2: %d\r\n", tariff);
#endif
                        tariff = number_from_tariffs(&p_str) & 0xffffffffffff;
                        if (p_str) {
                            zcl_getAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_3_SUMMATION_DELIVERD, &attr_len, (u8*)&attr_data);
                            last_tariff = fromPtoInteger(attr_len, attr_data);

                            if (tariff > last_tariff) {
                                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_3_SUMMATION_DELIVERD, (u8*)&tariff);
                            }
#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
                            printf("tariff3: %d\r\n", tariff);
#endif
                            tariff = number_from_tariffs(&p_str) & 0xffffffffffff;
                            if (p_str) {
                                zcl_getAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_4_SUMMATION_DELIVERD, &attr_len, (u8*)&attr_data);
                                last_tariff = fromPtoInteger(attr_len, attr_data);

                                if (tariff > last_tariff) {
                                    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_4_SUMMATION_DELIVERD, (u8*)&tariff);
                                }
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

static void get_power_data() {

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand get power\r\n");
#endif

//    u8 buff[] = "100700FF(00025.45)";

    if (send_command(cmd_power_data)) {
        if (response_meter(cmd_power_data) == PKT_OK) {
            u8 *p_str = package_buff;
//            u8 *p_str = buff;

            u32 power = number_from_brackets(&p_str);
            if (p_str) {


                multiplier = 1;

//                printf("power: %d, multiplier: %d, divisor: %d\r\n", power, multiplier, divisor);

                if (power != 0) {
                    while (power > 0xffff) {
                        power /= 10;
                        if (divisor >= 10) divisor /= 10;
                        else multiplier *= 10;
                    }
                } else {
                    divisor = 1;
                }

//                printf("power: %d, multiplier: %d, divisor: %d\r\n", power, multiplier, divisor);

                u16 pwr = power & 0xffff;
                u16 power_divisor = divisor & 0xffff;

                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_AC_POWER_DIVISOR, (u8*)&power_divisor);
                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_AC_POWER_MULTIPLIER, (u8*)&multiplier);

                zcl_getAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_APPARENT_POWER, &attr_len, (u8*)&attr_data);
                u16 last_pwr = fromPtoInteger(attr_len, attr_data);

                if (pwr != last_pwr) {
                    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_APPARENT_POWER, (u8*)&pwr);
                }

#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
                printf("power: %d\r\n", power);
#endif
            }
        }
    }
}

static void get_serial_number_data() {

    u8 *sn, *p_str;

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand get serial number\r\n");
#endif

    if (send_command(cmd_serial_number)) {
        if (response_meter(cmd_serial_number) == PKT_OK) {
            p_str = package_buff;
            sn = str_from_brackets(p_str);
            if (sn) {
                u8 serial_number[SE_ATTR_SN_SIZE] = {0};
                if (set_zcl_str(sn, serial_number, SE_ATTR_SN_SIZE)) {
                    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_METER_SERIAL_NUMBER, (u8*)&serial_number);
#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
                    printf("Serial Number: %s, len: %d\r\n", serial_number+1, *serial_number);
#endif
                }
            }
        }
    }

}


static void get_date_release_data() {
    u8 dr[] = "Not supported";
    u8 date_release[DATA_MAX_LEN+2] = {0};

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand get date release\r\n");
#endif

    if (set_zcl_str(dr, date_release, DATA_MAX_LEN+1)) {
        zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CUSTOM_DATE_RELEASE, (u8*)&date_release);
#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
        printf("Date of release: %s, len: %d\r\n", date_release+1, *date_release);
#endif
    }
}

static void get_resbat_data() {

#if UART_PRINTF_MODE
    printf("\r\nCommand get resource of battery\r\n");
#endif

    u16 battery_mv = 0;

    if (send_command(cmd_sensors_data)) {
        if (response_meter(cmd_sensors_data) == PKT_OK) {
            u8 *p_str = package_buff;

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
                    u8 battery_level = (battery_mv - MIN_VBAT_MV) / ((MAX_VBAT_MV - MIN_VBAT_MV) / 100);
                    if (battery_level > 100) battery_level = 100;
                    zcl_getAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_REMAINING_BATTERY_LIFE, &attr_len, (u8*)&attr_data);
                    u8 last_bl = fromPtoInteger(attr_len, attr_data);

                    if (battery_level != last_bl) {
                        zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_REMAINING_BATTERY_LIFE, (u8*)&battery_level);
                    }

#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
                    printf("Resource battery: %d%%\r\n", battery_level);
#endif
                }
            }
        }
    }



}


static void get_data() {
    if (new_start) {
        get_serial_number_data();
        get_date_release_data();
        new_start = false;
    }
    get_resbat_data();
    get_tariffs_data();
    get_power_data();
}


u8 measure_meter_neva_mt124() {

    app_uart_init(300);

    u8 ret = open_session();

    if (ret) {
        ret = ack_start();
        if (ret) {
            get_data();
            fault_measure_flag = false;
        } else {
            fault_measure_flag = true;
            if (!timerFaultMeasurementEvt) {
                timerFaultMeasurementEvt = TL_ZB_TIMER_SCHEDULE(fault_measure_meterCb, NULL, /*TIMEOUT_30SEC*/TIMEOUT_10MIN);
            }
        }
    } else {
        app_uart_init(9600);
        ret = get_sensors_data();
        if (ret) {
            get_data();
            fault_measure_flag = false;
        } else {
            fault_measure_flag = true;
            if (!timerFaultMeasurementEvt) {
                timerFaultMeasurementEvt = TL_ZB_TIMER_SCHEDULE(fault_measure_meterCb, NULL, /*TIMEOUT_30SEC*/TIMEOUT_10MIN);
            }
        }
    }

    return ret;
}


