#include <stdint.h>
#include "tl_common.h"
#include "stack/ble/ble.h"

#include "device.h"
#include "energomera_ce102m.h"
#include "cfg.h"
#include "app_uart.h"
#include "app.h"
#include "ble.h"

#define ACK 0x06
#define SOH 0x01
#define STX 0x02
#define ETX 0x03

static uint8_t package_buff[PKT_BUFF_MAX_LEN];

static const uint8_t cmd0[] = {0x2F, 0x3F, 0x21, 0x0D, 0x0A, 0x00};
static const uint8_t cmd1[] = {SOH, 0x42, 0x30, ETX, 0x75, 0x00};
static const uint8_t cmd2[] = {ACK, 0x30, 0x35, 0x31, 0x0D, 0x0A, 0x00};
static const uint8_t cmd3[] = {SOH, 0x50, 0x31, STX, 0x28, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x29, ETX, 0x21, 0x00};
static const uint8_t cmd4[] = {SOH, 0x52, 0x31, STX, 0x53, 0x54, 0x41, 0x54, 0x5F, 0x28, 0x29, ETX, 0x74, 0x00};    /* status   */
static const uint8_t cmd5[] = {SOH, 0x52, 0x31, STX, 0x45, 0x54, 0x30, 0x50, 0x45, 0x28, 0x29, ETX, 0x37, 0x00};    /* tariffs  */
static const uint8_t cmd6[] = {SOH, 0x52, 0x31, STX, 0x50, 0x4F, 0x57, 0x45, 0x50, 0x28, 0x29, ETX, 0x64, 0x00};    /* power    */
static const uint8_t cmd7[] = {SOH, 0x52, 0x31, STX, 0x56, 0x4F, 0x4C, 0x54, 0x41, 0x28, 0x29, ETX, 0x5F, 0x00};    /* volts    */
static const uint8_t cmd8[] = {SOH, 0x52, 0x31, STX, 0x43, 0x55, 0x52, 0x52, 0x45, 0x28, 0x29, ETX, 0x5A, 0x00};    /* current  */
static const uint8_t cmd9[] = {SOH, 0x52, 0x31, STX, 0x53, 0x4E, 0x55, 0x4D, 0x42, 0x28, 0x29, ETX, 0x5E, 0x00};    /* serial number */

static const uint8_t* command_array[] = {cmd0, cmd1, cmd2, cmd3, cmd4, cmd5, cmd6, cmd7, cmd8, cmd9};

_attribute_ram_code_ static uint8_t checksum(const uint8_t *src_buffer, uint8_t len) {
    uint8_t crc = 0;

    len--;

    for (uint8_t i = 1; i < len; i++) {
        crc += src_buffer[i];
    }

    return crc & 0x7f;
}

_attribute_ram_code_ static size_t sizeof_str(const uint8_t *str) {

    size_t len = 0;

    while(1) {
        if (str[len] == 0) break;
        len++;
    }

    return len;
}

_attribute_ram_code_ static uint8_t check_even_parity(uint8_t ch) {

         ch ^= ch >> 4;
         ch ^= ch >> 2;
         ch ^= ch >> 1;

         return ch & 1;
}

_attribute_ram_code_ static uint32_t str2uint(const uint8_t* str){
    uint32_t num = 0;
    uint8_t i = 0;
    while (str[i] && (str[i] >= '0' && str[i] <= '9')){
        num = num * 10 + (str[i] - '0');
        i++;
    }
    return num;
}

_attribute_ram_code_ static uint32_t number_from_brackets(uint8_t **p_str, command_t command) {

    uint32_t value = 0;
    uint8_t *remainder, *integer;
    uint8_t *init_bracket, *end_bracket, *point;

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

_attribute_ram_code_ static uint8_t *str_from_brackets(uint8_t *p_str) {

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


_attribute_ram_code_ static size_t send_command(command_t command) {

    size_t len, size = sizeof_str(command_array[command]);
    uint8_t ch;

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
#if UART_PRINT_DEBUG_ENABLE && UART_DEBUG
            printf("send bytes: %u\r\n", len);
#endif
            break;
        } else {
            len = 0;
        }
#if UART_PRINT_DEBUG_ENABLE && UART_DEBUG
        printf("Attempt to send data to uart: %u\r\n", attempt+1);
#endif
        sleep_ms(250);
    }

    if (len == 0) {
#if UART_PRINT_DEBUG_ENABLE && UART_DEBUG
        printf("Can't send a request pkt\r\n");
#endif
    } else {
#if UART_PRINT_DEBUG_ENABLE && UART_DEBUG
        printf("request pkt: 0x");
        for (int i = 0; i < len; i++) {
            printf("%02x", package_buff[i]);
        }
        printf("\r\n");
        sleep_ms(200);
#endif
    }

    return len;
}

_attribute_ram_code_ static pkt_error_t response_meter(command_t command) {

    uint8_t load_size;
    pkt_error_no = PKT_ERR_TIMEOUT;

    for (uint8_t attempt = 0; attempt < 3; attempt ++) {
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

#if UART_PRINT_DEBUG_ENABLE && UART_DEBUG
    printf("read bytes: %u\r\n", load_size);
#endif

    if (load_size) {
#if UART_PRINT_DEBUG_ENABLE && UART_DEBUG
        printf("response pkt: 0x");
        for (int i = 0; i < load_size; i++) {
            printf("%02x", package_buff[i]);
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
            uint8_t crc = checksum(package_buff, load_size);
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
            uint8_t crc = checksum(package_buff, load_size);
            if (crc == package_buff[load_size-1]) {
                pkt_error_no = PKT_OK;
            } else {
                pkt_error_no = PKT_ERR_CRC;
            }
        }
    }

#if UART_PRINT_DEBUG_ENABLE
    if (pkt_error_no != PKT_OK) print_error(pkt_error_no);
#endif

    return pkt_error_no;
}

_attribute_ram_code_ static uint8_t open_session() {

    flush_buff_uart();

    if (send_command(cmd_open_channel)) {
        if (response_meter(cmd_open_channel) == PKT_OK) {
            return true;
        }
    }

    return false;
}

_attribute_ram_code_ static void close_session() {

    send_command(cmd_close_channel);
}

_attribute_ram_code_ static uint8_t ack_start() {

    if (send_command(cmd_ack_start)) {
        if (response_meter(cmd_ack_start) == PKT_OK) {
            return true;
        }
    }

    return false;
}

_attribute_ram_code_ static void get_stat_data() {

    if (send_command(cmd_stat_data)) {
        if (response_meter(cmd_stat_data) == PKT_OK) {
            uint32_t val;
            uint8_t v, battery_level, *stat_str, *p_str = package_buff;

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

#if UART_PRINT_DEBUG_ENABLE
                printf("status: 0x%08x\r\n", val);
#endif

                ce102m_status_t *status = (ce102m_status_t*) &val;
                if (status->stat_battery) {
                    battery_level = 0;
                } else {
                    battery_level = 100;
                }
                if (meter.battery_level != battery_level) {
                    meter.battery_level = battery_level;
                    pva_changed = true;
                }
            }

        }
    }
}

_attribute_ram_code_ static void get_tariffs_data() {

    if (send_command(cmd_tariffs_data)) {
        if (response_meter(cmd_tariffs_data) == PKT_OK) {
            uint32_t tariff;
            uint8_t *p_str = package_buff;

            while(*p_str != '(' && *p_str != 0) {
                p_str++;
            }

            if (*(p_str++) == '(') {
                tariff = number_from_brackets(&p_str, cmd_tariffs_data);

                if (p_str) {
                    if (meter.tariff_1 < tariff) {
                        meter.tariff_1 = tariff;
                        tariff_changed = true;
                        tariff1_notify = NOTIFY_MAX;
                    }
#if UART_PRINT_DEBUG_ENABLE
                    printf("tariff1: %u\r\n", tariff);
#endif

                    tariff = number_from_brackets(&p_str, cmd_tariffs_data);
                    if (p_str) {
                        if (meter.tariff_2 < tariff) {
                            meter.tariff_2 = tariff;
                            tariff_changed = true;
                            tariff2_notify = NOTIFY_MAX;
                        }
#if UART_PRINT_DEBUG_ENABLE
                        printf("tariff2: %u\r\n", tariff);
#endif

                        tariff = number_from_brackets(&p_str, cmd_tariffs_data);
                        if (p_str) {
                            if (meter.tariff_3 < tariff) {
                                meter.tariff_3 = tariff;
                                tariff_changed = true;
                                tariff3_notify = NOTIFY_MAX;
                            }
#if UART_PRINT_DEBUG_ENABLE
                            printf("tariff3: %u\r\n", tariff);
#endif
                        }
                    }
                }
            }

        }
    }
}

_attribute_ram_code_ static void get_power_data() {

    if (send_command(cmd_power_data)) {
        if (response_meter(cmd_power_data) == PKT_OK) {
            uint8_t *p_str = package_buff;
            uint32_t power = number_from_brackets(&p_str, cmd_power_data);
            if (p_str) {
                if (meter.power != power) {
                    meter.power = power;
                    pva_changed = true;
                    power_notify = NOTIFY_MAX;
                }
#if UART_PRINT_DEBUG_ENABLE
                printf("power: %u\r\n", power);
#endif
            }
        }
    }
}

_attribute_ram_code_ static void get_voltage_data() {

    if (send_command(cmd_volts_data)) {
        if (response_meter(cmd_volts_data) == PKT_OK) {
            uint8_t *p_str = package_buff;
            uint16_t volts = number_from_brackets(&p_str, cmd_volts_data);
            if (p_str) {
                if (meter.voltage != volts) {
                    meter.voltage = volts;
                    pva_changed = true;
                    voltage_notify = NOTIFY_MAX;
                }
#if UART_PRINT_DEBUG_ENABLE
                printf("voltage: %u\r\n", volts);
#endif
            }
        }
    }
}

_attribute_ram_code_ static void get_amps_data() {

    if (send_command(cmd_amps_data)) {
        if (response_meter(cmd_amps_data) == PKT_OK) {
            uint8_t *p_str = package_buff;
            uint16_t amps = number_from_brackets(&p_str, cmd_amps_data);
            if (p_str) {
                if (meter.amps != amps) {
                    meter.amps = amps;
                    pva_changed = true;
                    ampere_notify = NOTIFY_MAX;
                }
#if UART_PRINT_DEBUG_ENABLE
                printf("amps: %u\r\n", amps);
#endif
            }
        }
    }
}

_attribute_ram_code_ void get_serial_number_data_energomera_ce102m() {

    uint8_t *serial_number, *p_str;

    if (send_command(cmd_serial_number)) {
        if (response_meter(cmd_serial_number) == PKT_OK) {
            p_str = package_buff;
            serial_number = str_from_brackets(p_str);
            if (serial_number) {
#if UART_PRINT_DEBUG_ENABLE
                printf("Serial Number: %s\r\n", serial_number);
#endif
                if (memcmp(meter.serial_number, serial_number, sizeof_str(serial_number)) != 0) {
                    meter.serial_number_len = sprintf((char*)meter.serial_number, "%s", serial_number);
                    memcpy(serial_number_notify.serial_number, meter.serial_number,
                           meter.serial_number_len > sizeof(serial_number_notify.serial_number)?
                           sizeof(serial_number_notify.serial_number):meter.serial_number_len);
                    sn_notify = NOTIFY_MAX;
                }
            }
        }
    }

}

_attribute_ram_code_ void get_date_release_data_energomera_ce102m() {
    meter.date_release_len = sprintf((char*)meter.date_release, "xx.xx.xxxx");
}

_attribute_ram_code_ void measure_meter_energomera_ce102m() {

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
        }
        close_session();
    }
}
