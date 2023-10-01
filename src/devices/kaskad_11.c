#include "tl_common.h"
#include "zcl_include.h"

#ifndef METER_MODEL
#define METER_MODEL KASKAD_11_C1
#endif

#if (METER_MODEL == KASKAD_11_C1)

#include "device.h"
#include "kaskad_11.h"
#include "app_dev_config.h"
#include "app_uart.h"
#include "se_custom_attr.h"
#include "app_main.h"

#define LEVEL_READ 0x02
#define MIN_PKT_SIZE 0x06

static package_t request_pkt;
static package_t response_pkt;

static u8 def_password[] = {0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30};
static u8 phases3;
static u8 release_month;
static u8 release_year;



static u8 checksum(const u8 *src_buffer) {
    u8 crc = 0;
    u8 len = src_buffer[0]-1;

    for (u8 i = 0; i < len; i++) {
        crc += src_buffer[i];
    }

    return crc;
}

static u8 send_command(package_t *pkt) {

    size_t len;

//    app_uart_rx_off();

    /* three attempts to write to uart */
    for (u8 attempt = 0; attempt < 3; attempt++) {
        len = write_bytes_to_uart((u8*)pkt, pkt->len);
        if (len == pkt->len) {
            break;
        } else {
            len = 0;
        }
#if UART_PRINTF_MODE
        printf("Attempt to send data to uart: %u\r\n", attempt+1);
#endif
        sleep_ms(250);
    }

    sleep_ms(200);
//    app_uart_rx_on();

#if UART_PRINTF_MODE && DEBUG_PACKAGE
    if (len == 0) {
        u8 head[] = "write to uart error";
        print_package(head, (u8*)pkt, pkt->len);
    } else {
        u8 head[] = "write to uart";
        print_package(head, (u8*)pkt, len);
    }
#endif

    return len;
}

static pkt_error_t response_meter(u8 command) {

    size_t load_size, load_len = 0, len = PKT_BUFF_MAX_LEN;
    u8 ch, complete = false;;
    pkt_error_no = PKT_ERR_TIMEOUT;
    u8 *buff = (u8*)&response_pkt;

    memset(buff, 0, sizeof(package_t));

    for (u8 attempt = 0; attempt < 3; attempt ++) {
        load_size = 0;
        while (available_buff_uart() && load_size < PKT_BUFF_MAX_LEN) {
            ch = read_byte_from_buff_uart();
            if (load_size == 0) {
                if (ch == 0) {
                    load_len++;
                    continue;
                } else if (get_queue_len_buff_uart() < ch-1) {
                    load_len++;
                    continue;
                }
                len = ch;
            }

            load_len++;
            buff[load_size++] = ch;
            len--;

            if (len == 0) {
                if (load_size < MIN_PKT_SIZE) {
                    len = PKT_BUFF_MAX_LEN;
                    load_size = 0;
                    pkt_error_no = PKT_ERR_INCOMPLETE;
                    continue;
                }

                if (load_size == buff[0]) {
                    attempt = 3;
                    pkt_error_no = PKT_OK;
                    complete = true;
                    break;
                }
            }

        }
        sleep_ms(250);
    }

    if (load_size) {
#if UART_PRINTF_MODE && DEBUG_PACKAGE
        u8 head[] = "read from uart";
        print_package(head, buff, load_size);
#endif
        if (complete) {
            u8 crc = checksum(buff);
            if (crc == buff[response_pkt.len-1]) {
                if (buff[response_pkt.len-2] == 0x01) {
                    if (response_pkt.address == dev_config.device_address) {
                        if (response_pkt.cmd == command) {
                            pkt_error_no = PKT_OK;
                        } else {
                            pkt_error_no = PKT_ERR_DIFFERENT_COMMAND;
                        }
                    } else {
                        pkt_error_no = PKT_ERR_ADDRESS;
                    }
                } else {
                    pkt_error_no = PKT_ERR_RESPONSE;
                }
            } else {
                pkt_error_no = PKT_ERR_CRC;
            }
        } else {
            pkt_error_no = PKT_ERR_INCOMPLETE;
        }
    } else {
#if UART_PRINTF_MODE && DEBUG_PACKAGE
        u8 head[] = "read from uart error";
        print_package(head, buff, load_size);
#endif
    }

#if UART_PRINTF_MODE
    if (pkt_error_no != PKT_OK) print_error(pkt_error_no);
#endif

    if (pkt_error_no != PKT_OK && get_queue_len_buff_uart()) {
        pkt_error_no = response_meter(command);
    }

    return pkt_error_no;
}

static void set_header(u8 cmd) {

    memset(&request_pkt, 0, sizeof(package_t));

    request_pkt.len = 1;
    request_pkt.cmd = cmd;
    request_pkt.len++;
    request_pkt.address = dev_config.device_address;
    request_pkt.len += 2;
}

static u8 open_channel() {

    u8 pos = 0;

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand running of open channel\r\n");
#endif

    set_header(cmd_open_channel);

    request_pkt.data[pos++] = LEVEL_READ;

    memcpy(request_pkt.data+pos, def_password, sizeof(def_password));
    pos += sizeof(def_password);
    request_pkt.len += pos+1;
    u8 crc = checksum((u8*)&request_pkt);
    request_pkt.data[pos] = crc;

    if (send_command(&request_pkt)) {
        if (response_meter(cmd_open_channel) == PKT_OK) {
            return true;
        }
    }

    return false;
}

static void close_channel() {

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand running of close channel\r\n");
#endif

    set_header(cmd_close_channel);

    request_pkt.len++;
    u8 crc = checksum((u8*)&request_pkt);
    request_pkt.data[0] = crc;

    if (send_command(&request_pkt)) {
        response_meter(cmd_close_channel);
    }
}

static void set_tariff_num(u8 tariff_num) {

    set_header(cmd_tariffs_data);

    request_pkt.data[0] = tariff_num;
    request_pkt.len += 2;
    request_pkt.data[1] = checksum((u8*)&request_pkt);
}

static void get_tariffs_data() {

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand running to get tariffs\r\n");
#endif

    u64 tariff;
    u64 last_tariff;

    for (u8 tariff_num = 1; tariff_num <= 4; tariff_num++) {
        set_tariff_num(tariff_num);
        if (send_command(&request_pkt)) {
            if (response_meter(cmd_tariffs_data) == PKT_OK) {

                pkt_tariff_t *pkt_tariff = (pkt_tariff_t*)&response_pkt;

                tariff = pkt_tariff->value & 0xffffffffffff;

                switch (tariff_num) {
                    case 1:
                        zcl_getAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_1_SUMMATION_DELIVERD, &attr_len, (u8*)&attr_data);
                        last_tariff = fromPtoInteger(attr_len, attr_data);

                        if (tariff > last_tariff) {
                            zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_1_SUMMATION_DELIVERD, (u8*)&tariff);
                        }
#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
                        printf("tariff1: %d\r\n", tariff);
#endif
                        break;
                    case 2:
                        zcl_getAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_2_SUMMATION_DELIVERD, &attr_len, (u8*)&attr_data);
                        last_tariff = fromPtoInteger(attr_len, attr_data);

                        if (tariff > last_tariff) {
                            zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_2_SUMMATION_DELIVERD, (u8*)&tariff);
                        }

#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
                        printf("tariff2: %d\r\n", tariff);
#endif
                        break;
                    case 3:
                        zcl_getAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_3_SUMMATION_DELIVERD, &attr_len, (u8*)&attr_data);
                        last_tariff = fromPtoInteger(attr_len, attr_data);

                        if (tariff > last_tariff) {
                            zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_3_SUMMATION_DELIVERD, (u8*)&tariff);
                        }
#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
                        printf("tariff3: %d\r\n", tariff);
#endif
                        break;
                    case 4:
                        zcl_getAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_4_SUMMATION_DELIVERD, &attr_len, (u8*)&attr_data);
                        last_tariff = fromPtoInteger(attr_len, attr_data);

                        if (tariff > last_tariff) {
                            zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_4_SUMMATION_DELIVERD, (u8*)&tariff);
                        }
#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
                        printf("tariff4: %d\r\n", tariff);
#endif
                        break;
                    default:
                        break;
                }
            }
        }
    }
}

static void set_net_parameters(u8 param) {

    set_header(cmd_net_parameters);

    request_pkt.data[0] = param;
    request_pkt.len += 2;
    request_pkt.data[1] = checksum((u8*)&request_pkt);

}

static void get_voltage_data() {

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand running to get voltage\r\n");
#endif

    set_net_parameters(net_voltage);

    if (send_command(&request_pkt)) {
        if (response_meter(cmd_net_parameters) == PKT_OK) {
            pkt_voltage_t *pkt_voltage = (pkt_voltage_t*)&response_pkt;

            zcl_getAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_RMS_VOLTAGE, &attr_len, (u8*)&attr_data);
            u16 last_volts = fromPtoInteger(attr_len, attr_data);

            if (pkt_voltage->voltage != last_volts) {
                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_RMS_VOLTAGE, (u8*)&pkt_voltage->voltage);
            }
#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
            printf("voltage: %d\r\n", pkt_voltage->voltage);
#endif
        }
    }
}

static void get_power_data() {

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand running to get power\r\n");
#endif

    set_net_parameters(net_power);

    if (send_command(&request_pkt)) {
        if (response_meter(cmd_net_parameters) == PKT_OK) {
            pkt_power_t *pkt_power = (pkt_power_t*)&response_pkt;
            u32 power = from24to32(pkt_power->power);

            while (power > 0xffff) power /= 10;

            u16 pwr = power & 0xffff;

            zcl_getAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_APPARENT_POWER, &attr_len, (u8*)&attr_data);
            u16 last_pwr = fromPtoInteger(attr_len, attr_data);

            if (pwr != last_pwr) {
                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_APPARENT_POWER, (u8*)&pwr);
            }

#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
            printf("power: %d\r\n", pwr);
#endif

        }
    }
}


static void get_amps_data() {

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand running to get current\r\n");
#endif

    set_net_parameters(net_amps);

    if (send_command(&request_pkt)) {
        if (response_meter(cmd_net_parameters) == PKT_OK) {
            /* check 1 phase or 3 phases */
            if (response_pkt.len > 9) {
                phases3 = true;
            } else {
                phases3 = false;
                pkt_amps_t *pkt_amps = (pkt_amps_t*)&response_pkt;

                zcl_getAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_LINE_CURRENT, &attr_len, (u8*)&attr_data);
                u16 last_amps = fromPtoInteger(attr_len, attr_data);

                if (pkt_amps->amps != last_amps) {
                    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_LINE_CURRENT, (u8*)&pkt_amps->amps);
                }
#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
                printf("amps: %d\r\n", pkt_amps->amps);
#endif
            }
        }
    }
}

static void get_resbat_data() {

    struct  datetime {
        u64 sec    :6;
        u64 min    :6;
        u64 hour   :5;
        u64 day_n  :3;
        u64 day    :5;
        u64 month  :4;
        u64 year   :7;
        u64 rsv    :28;
    };

    u8 worktime, lifetime = RESOURCE_BATTERY;

#if UART_PRINTF_MODE
    printf("\r\nCommand running to get resource of battery\r\n");
#endif

    set_header(cmd_datetime_device);

    request_pkt.len++;
    u8 crc = checksum((u8*)&request_pkt);
    request_pkt.data[0] = crc;

    if (send_command(&request_pkt)) {
        if (response_meter(cmd_datetime_device) == PKT_OK) {
            pkt_datetime_t *pkt_datetime = (pkt_datetime_t*)&response_pkt;

            struct datetime *dt = (struct datetime*)pkt_datetime->datetime;

            worktime = lifetime - ((dt->year * 12 + dt->month) - (release_year * 12 + release_month));

        }

        u8 battery_level = (worktime*100)/lifetime;

        if (((worktime*100)%lifetime) >= (lifetime/2)) {
            battery_level++;
        }

        zcl_getAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_REMAINING_BATTERY_LIFE, &attr_len, (u8*)&attr_data);
        u8 last_bl = fromPtoInteger(attr_len, attr_data);

        if (battery_level != last_bl) {
            zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_REMAINING_BATTERY_LIFE, (u8*)&battery_level);
        }

#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
        printf("Resource battery: %d.%d\r\n", (worktime*100)/lifetime, ((worktime*100)%lifetime)*100/lifetime);
#endif
    }


}

static void get_date_release_data() {

    pkt_release_t *pkt;

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand running to get date release\r\n");
#endif

    set_header(cmd_date_release);

    request_pkt.len++;
    u8 crc = checksum((u8*)&request_pkt);
    request_pkt.data[0] = crc;

    if (send_command(&request_pkt)) {
        if (response_meter(cmd_date_release) == PKT_OK) {
            pkt = (pkt_release_t*)&response_pkt;
            u8 release_day = pkt->day;
            release_month = pkt->month;
            release_year = pkt->year;

            u8 date_release[DATA_MAX_LEN+2] = {0};
            u8 dr[11] = {0};
            u8 dr_len = 0;


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

            u8 year_str[8] = {0};

            u16 year = 2000 + release_year;

            itoa(year, year_str);

            dr[dr_len++] = year_str[0];
            dr[dr_len++] = year_str[1];
            dr[dr_len++] = year_str[2];
            dr[dr_len++] = year_str[3];


            if (set_zcl_str(dr, date_release, DATA_MAX_LEN+1)) {
                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CUSTOM_DATE_RELEASE, (u8*)&date_release);
#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
                printf("Date of release: %s, len: %d\r\n", date_release+1, *date_release);
#endif
            }
        }
    }
}

static void get_serial_number_data() {

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand running to get serial number\r\n");
#endif

    set_header(cmd_serial_number);

    request_pkt.len++;
    u8 crc = checksum((u8*)&request_pkt);
    request_pkt.data[0] = crc;

    if (send_command(&request_pkt)) {
        if (response_meter(cmd_serial_number) == PKT_OK) {
            u8 serial_number[SE_ATTR_SN_SIZE+1] = {0};
            u8 sn[SE_ATTR_SN_SIZE] = {0};

            memcpy(sn, response_pkt.data, (response_pkt.len - MIN_PKT_SIZE));

            if (set_zcl_str(sn, serial_number, SE_ATTR_SN_SIZE)) {
                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_METER_SERIAL_NUMBER, (u8*)&serial_number);
#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
            printf("Serial Number: %s, len: %d\r\n", serial_number+1, *serial_number);
#endif
            }
        }
    }
}

u8 measure_meter_kaskad_11() {

    u8 ret = open_channel();

    if (ret) {

        if (new_start) {
            get_serial_number_data();
            get_date_release_data();
            new_start = false;
        }

        get_amps_data();            /* get amps and check phases num */

        if (phases3) {
#if UART_PRINTF_MODE
            printf("Sorry, three-phase meter!\r\n");
#endif
        } else {
            get_tariffs_data();     /* get 4 tariffs        */
            get_voltage_data();     /* get voltage net ~220 */
            get_power_data();       /* get power            */
            get_resbat_data();      /* get resource battery */

            ret = true;
        }
        close_channel();
        fault_measure_flag = false;
    } else {
        fault_measure_flag = true;
        if (!timerFaultMeasurementEvt) {
            timerFaultMeasurementEvt = TL_ZB_TIMER_SCHEDULE(fault_measure_meterCb, NULL, TIMEOUT_10MIN);
        }
    }

    return ret;
}

#endif
