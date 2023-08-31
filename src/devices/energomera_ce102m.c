#include "tl_common.h"
#include "zcl_include.h"

#include "config.h"
#include "device.h"
#include "energomera_ce102m.h"
#include "app_uart.h"
#include "app_ui.h"
#include "app_utility.h"
#include "se_custom_attr.h"
#include "electricityMeter.h"

#define ACK 0x06
#define SOH 0x01
#define STX 0x02
#define ETX 0x03

static u8 package_buff[PKT_BUFF_MAX_LEN];

static const u8 cmd0[] = {0x2F, 0x3F, 0x21, 0x0D, 0x0A, 0x00};
static const u8 cmd1[] = {SOH, 0x42, 0x30, ETX, 0x75, 0x00};
static const u8 cmd2[] = {ACK, 0x30, 0x35, 0x31, 0x0D, 0x0A, 0x00};
static const u8 cmd3[] = {SOH, 0x50, 0x31, STX, 0x28, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x29, ETX, 0x21, 0x00};
static const u8 cmd4[] = {SOH, 0x52, 0x31, STX, 0x53, 0x54, 0x41, 0x54, 0x5F, 0x28, 0x29, ETX, 0x74, 0x00};    /* status   */
static const u8 cmd5[] = {SOH, 0x52, 0x31, STX, 0x45, 0x54, 0x30, 0x50, 0x45, 0x28, 0x29, ETX, 0x37, 0x00};    /* tariffs  */
static const u8 cmd6[] = {SOH, 0x52, 0x31, STX, 0x50, 0x4F, 0x57, 0x45, 0x50, 0x28, 0x29, ETX, 0x64, 0x00};    /* power    */
static const u8 cmd7[] = {SOH, 0x52, 0x31, STX, 0x56, 0x4F, 0x4C, 0x54, 0x41, 0x28, 0x29, ETX, 0x5F, 0x00};    /* volts    */
static const u8 cmd8[] = {SOH, 0x52, 0x31, STX, 0x43, 0x55, 0x52, 0x52, 0x45, 0x28, 0x29, ETX, 0x5A, 0x00};    /* current  */
static const u8 cmd9[] = {SOH, 0x52, 0x31, STX, 0x53, 0x4E, 0x55, 0x4D, 0x42, 0x28, 0x29, ETX, 0x5E, 0x00};    /* serial number */

static const u8* command_array[] = {cmd0, cmd1, cmd2, cmd3, cmd4, cmd5, cmd6, cmd7, cmd8, cmd9};

static u8 checksum(const u8 *src_buffer, u8 len) {
    u8 crc = 0;

    len--;

    for (u8 i = 1; i < len; i++) {
        crc += src_buffer[i];
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

static u8 check_even_parity(u8 ch) {

         ch ^= ch >> 4;
         ch ^= ch >> 2;
         ch ^= ch >> 1;

         return ch & 1;
}

static u32 str2uint(const u8* str){
    u32 num = 0;
    u8 i = 0;
    while (str[i] && (str[i] >= '0' && str[i] <= '9')){
        num = num * 10 + (str[i] - '0');
        i++;
    }
    return num;
}

static u32 number_from_brackets(u8 **p_str, command_t command) {

    u32 value = 0;
    u8 *remainder, *integer;
    u8 *init_bracket, *end_bracket, *point;

    init_bracket = *p_str;

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
            if (command == cmd_power_data) {
                value = str2uint(integer) * 1000000;
            } else if (command == cmd_amps_data) {
                value = str2uint(integer) * 1000;
            } else {
                value = str2uint(integer) * 100;
            }
            point = init_bracket;
            while (*point != '.' && *point != 0) {
                point++;
            }
            if (*point == '.') {
                *(point++) = 0;
                remainder = point;
                size_t rmndr_len = sizeof_str(remainder);
                if (rmndr_len != 0) {
                    if (rmndr_len == 1) {
                        value += (str2uint(remainder) * 10);
                    } else {
                        if (command == cmd_power_data) {
                            remainder[6] = 0;
                        } else if (command == cmd_amps_data) {
                            remainder[3] = 0;
                        } else {
                            remainder[2] = 0;
                        }
                        value += str2uint(remainder);
                    }
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


static size_t send_command(command_t command) {

    size_t len, size = sizeof_str(command_array[command]);
    u8 ch;

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
#if UART_PRINTF_MODE && UART_DEBUG
            printf("send bytes: %d\r\n", len);
#endif
            break;
        } else {
            len = 0;
        }
#if UART_PRINTF_MODE && UART_DEBUG
        printf("Attempt to send data to uart: %d\r\n", attempt+1);
#endif
        sleep_ms(250);
    }

    if (len == 0) {
#if UART_PRINTF_MODE && UART_DEBUG
        printf("Can't send a request pkt\r\n");
#endif
    } else {
#if UART_PRINTF_MODE && UART_DEBUG
        u8 ch;
        printf("request pkt: 0x");
        for (int i = 0; i < len; i++) {
            ch = ((u8*)&request_pkt)[i];
            if (ch < 0x10) {
                printf("0%x", ch);
            } else {
                printf("%x", ch);
            }
//            printf("%02x", ((u8*)&request_pkt)[i]);
        }
        printf("\r\n");
#endif
        sleep_ms(200);
    }

    return len;
}

static pkt_error_t response_meter(command_t command) {

    u8 load_size;
    pkt_error_no = PKT_ERR_TIMEOUT;

    for (u8 attempt = 0; attempt < 3; attempt ++) {
        load_size = 0;
        while (available_buff_uart() && load_size < PKT_BUFF_MAX_LEN) {
            package_buff[load_size++] = read_byte_from_buff_uart() & 0x7f;
            if (!available_buff_uart()) {
                attempt = 3;
                pkt_error_no = PKT_OK;
                package_buff[load_size] = 0;
                break;
            }
        }
        if (attempt < 3) sleep_ms(250);
    }

#if UART_PRINTF_MODE && UART_DEBUG
    printf("read bytes: %d\r\n", load_size);
#endif

    if (load_size) {
#if UART_PRINTF_MODE && UART_DEBUG
        u8 ch;
        printf("response pkt: 0x");
        for (int i = 0; i < load_size; i++) {
            ch = package_buff[i];
            if (ch < 0x10) {
                printf("0%x", ch);
            } else {
                printf("%x", ch);
            }
//            printf("%02x", package_buff[i]);
        }
        printf(" ");
        for (int i = 0; i < load_size; i++) {
            if (package_buff[i] < 0x20) printf(".");
            else printf("%c", package_buff[i]);
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
                } else if (command == cmd_stat_data ||
                           command == cmd_tariffs_data ||
                           command == cmd_power_data ||
                           command == cmd_volts_data ||
                           command == cmd_amps_data ||
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
    }

#if UART_PRINTF_MODE
    if (pkt_error_no != PKT_OK) print_error(pkt_error_no);
#endif

    return pkt_error_no;
}

static u8 open_session() {

    flush_buff_uart();

    if (send_command(cmd_open_channel)) {
        if (response_meter(cmd_open_channel) == PKT_OK) {
            return true;
        }
    }

    return false;
}

static void close_session() {

    send_command(cmd_close_channel);
}

static u8 ack_start() {

    if (send_command(cmd_ack_start)) {
        if (response_meter(cmd_ack_start) == PKT_OK) {
            return true;
        }
    }

    return false;
}

static void get_stat_data() {

    if (send_command(cmd_stat_data)) {
        if (response_meter(cmd_stat_data) == PKT_OK) {
            u32 val;
            u8 v, battery_level, *stat_str, *p_str = package_buff;

            stat_str = str_from_brackets(p_str);

            if (stat_str) {

                v = (*(stat_str++) << 4) & 0xf0;
                v |= (*(stat_str++)) & 0x0f;
                val = (v << 24) & 0xff000000;
                v = (*(stat_str++) << 4) & 0xf0;
                v |= (*(stat_str++)) & 0x0f;
                val |= (v << 16) & 0xff0000;
                v = (*(stat_str++) << 4) & 0xf0;
                v |= (*(stat_str++)) & 0x0f;
                val |= (v << 8) & 0xff00;
                v = (*(stat_str++) << 4) & 0xf0;
                v |= (*(stat_str++)) & 0x0f;
                val |= v & 0xff;

#if UART_PRINTF_MODE
                printf("status: 0x");
                if (val <= 0xf) {
                    printf("0000000");
                } else if (val <= 0xff) {
                    printf("000000");
                } else if (val <= 0xfff) {
                    printf("00000");
                } else if (val <= 0xffff) {
                    printf("0000");
                } else if (val <= 0xfffff) {
                    printf("000");
                } else if (val <= 0xffffff) {
                    printf("00");
                } else if (val <= 0xfffffff) {
                    printf("0");
                }
                printf("%x\r\n", val);
#endif

                ce102m_status_t *status = (ce102m_status_t*) &val;
                if (status->stat_battery) {
                    battery_level = 0;
                } else {
                    battery_level = 100;
                }
                zcl_setAttrVal(ELECTRICITY_METER_EP1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_REMAINING_BATTERY_LIFE, (u8*)&battery_level);
            }

        }
    }
}

static void get_tariffs_data() {

    if (send_command(cmd_tariffs_data)) {
        if (response_meter(cmd_tariffs_data) == PKT_OK) {
            u8 *p_str = package_buff;

            while(*p_str != '(' && *p_str != 0) {
                p_str++;
            }

            if (*(p_str++) == '(') {

                u64 tariff = number_from_brackets(&p_str, cmd_tariffs_data) & 0xffffffffffff;
                u64 last_tariff;

                if (p_str) {

                    zcl_getAttrVal(ELECTRICITY_METER_EP1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_1_SUMMATION_DELIVERD, &attr_len, (u8*)&attr_data);
                    last_tariff = fromPtoInteger(attr_len, attr_data);

                    if (tariff > last_tariff) {
                        zcl_setAttrVal(ELECTRICITY_METER_EP1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_1_SUMMATION_DELIVERD, (u8*)&tariff);
                    }
#if UART_PRINTF_MODE
                    printf("tariff1: %d\r\n", tariff);
#endif

                    tariff = number_from_brackets(&p_str, cmd_tariffs_data) & 0xffffffffffff;

                    if (p_str) {

                        zcl_getAttrVal(ELECTRICITY_METER_EP1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_2_SUMMATION_DELIVERD, &attr_len, (u8*)&attr_data);
                        last_tariff = fromPtoInteger(attr_len, attr_data);

                        if (tariff > last_tariff) {
                            zcl_setAttrVal(ELECTRICITY_METER_EP1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_2_SUMMATION_DELIVERD, (u8*)&tariff);
                        }

#if UART_PRINTF_MODE
                        printf("tariff2: %d\r\n", tariff);
#endif

                        tariff = number_from_brackets(&p_str, cmd_tariffs_data) & 0xffffffffffff;

                        if (p_str) {

                            zcl_getAttrVal(ELECTRICITY_METER_EP1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_3_SUMMATION_DELIVERD, &attr_len, (u8*)&attr_data);
                            last_tariff = fromPtoInteger(attr_len, attr_data);

                            if (tariff > last_tariff) {
                                zcl_setAttrVal(ELECTRICITY_METER_EP1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_3_SUMMATION_DELIVERD, (u8*)&tariff);
                            }
#if UART_PRINTF_MODE
                            printf("tariff3: %d\r\n", tariff);
#endif
                            tariff = number_from_brackets(&p_str, cmd_tariffs_data) & 0xffffffffffff;

                            if (p_str) {

                                zcl_getAttrVal(ELECTRICITY_METER_EP1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_4_SUMMATION_DELIVERD, &attr_len, (u8*)&attr_data);
                                last_tariff = fromPtoInteger(attr_len, attr_data);

                                if (tariff > last_tariff) {
                                    zcl_setAttrVal(ELECTRICITY_METER_EP1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_4_SUMMATION_DELIVERD, (u8*)&tariff);
                                }
#if UART_PRINTF_MODE
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

    if (send_command(cmd_power_data)) {
        if (response_meter(cmd_power_data) == PKT_OK) {
            u8 *p_str = package_buff;
            u32 power = number_from_brackets(&p_str, cmd_power_data);
            if (p_str) {

                while (power > 0xffff) power /= 10;

                u16 pwr = power & 0xffff;

                zcl_getAttrVal(ELECTRICITY_METER_EP1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_APPARENT_POWER, &attr_len, (u8*)&attr_data);
                u16 last_pwr = fromPtoInteger(attr_len, attr_data);

                if (pwr != last_pwr) {
                    zcl_setAttrVal(ELECTRICITY_METER_EP1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_APPARENT_POWER, (u8*)&pwr);
                }

#if UART_PRINTF_MODE
                printf("power: %d\r\n", power);
#endif
            }
        }
    }
}

static void get_voltage_data() {

    if (send_command(cmd_volts_data)) {
        if (response_meter(cmd_volts_data) == PKT_OK) {
            u8 *p_str = package_buff;
            u16 volts = number_from_brackets(&p_str, cmd_volts_data);
            if (p_str) {

                zcl_getAttrVal(ELECTRICITY_METER_EP1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_RMS_VOLTAGE, &attr_len, (u8*)&attr_data);
                u16 last_volts = fromPtoInteger(attr_len, attr_data);

                if (volts != last_volts) {
                    zcl_setAttrVal(ELECTRICITY_METER_EP1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_RMS_VOLTAGE, (u8*)&volts);
                }
#if UART_PRINTF_MODE
                printf("voltage: %d\r\n", volts);
#endif
            }
        }
    }
}

static void get_amps_data() {

    if (send_command(cmd_amps_data)) {
        if (response_meter(cmd_amps_data) == PKT_OK) {
            u8 *p_str = package_buff;
            u16 amps = number_from_brackets(&p_str, cmd_amps_data);
            if (p_str) {

                zcl_getAttrVal(ELECTRICITY_METER_EP1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_LINE_CURRENT, &attr_len, (u8*)&attr_data);
                u16 last_amps = fromPtoInteger(attr_len, attr_data);

                if (amps != last_amps) {
                    zcl_setAttrVal(ELECTRICITY_METER_EP1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_LINE_CURRENT, (u8*)&amps);
                }
#if UART_PRINTF_MODE
                printf("amps: %d\r\n", amps);
#endif
            }
        }
    }
}


static void get_serial_number_data_energomera_ce102m() {

    u8 *sn, *p_str;

    if (send_command(cmd_serial_number)) {
        if (response_meter(cmd_serial_number) == PKT_OK) {
            p_str = package_buff;
            sn = str_from_brackets(p_str);
            if (sn) {
                u8 serial_number[SE_ATTR_SN_SIZE] = {0};
                if (set_zcl_str(sn, serial_number, SE_ATTR_SN_SIZE)) {
                    zcl_setAttrVal(ELECTRICITY_METER_EP1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_METER_SERIAL_NUMBER, (u8*)&serial_number);
#if UART_PRINTF_MODE
                    printf("Serial Number: %s\r\n", serial_number+1);
#endif
                }
            }
        }
    }

}

static void get_date_release_data_energomera_ce102m() {
    u8 dr[] = "xx.xx.xxxx";
    u8 date_release[DATA_MAX_LEN+2] = {0};

    if (set_zcl_str(dr, date_release, DATA_MAX_LEN+1)) {
        zcl_setAttrVal(ELECTRICITY_METER_EP1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CUSTOM_DATE_RELEASE, (u8*)&date_release);
#if UART_PRINTF_MODE
        printf("Date of release: %s\r\n", date_release+1);
#endif
    }
}

u8 measure_meter_energomera_ce102m() {

    u8 ret = false;

    if (open_session()) {
        if (ack_start()) {
            get_stat_data();
            if (new_start) {
                get_serial_number_data_energomera_ce102m();
                get_date_release_data_energomera_ce102m();
                new_start = false;
            }
            get_tariffs_data();
            get_power_data();
            get_voltage_data();
            get_amps_data();

            fault_measure_counter = 0;
            ret = true;
        } else {
            fault_measure_counter++;
        }
        close_session();
    } else {
        fault_measure_counter++;
    }

    if (fault_measure_counter == 10) {
#if UART_PRINTF_MODE
        printf("Fault get data from device. Restart!!!\r\n");
#endif
        zb_resetDevice();
    }

    return ret;
}
