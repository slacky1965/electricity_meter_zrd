#include "tl_common.h"
#include "zcl_include.h"

#include "app_dev_config.h"
#include "device.h"
#include "mercury_206.h"
#include "app_uart.h"
#include "se_custom_attr.h"
#include "app_endpoint_cfg.h"

static package_t request_pkt;
static package_t response_pkt;

static uint16_t checksum(const uint8_t *src_buffer, uint8_t len) {

    const uint16_t generator = 0xa001;

    uint16_t crc = 0xffff;

    for (const uint8_t *ptr = src_buffer; ptr < src_buffer + len; ptr++) {
        crc ^= *ptr;

        for (uint8_t bit = 8; bit > 0; bit--) {
            if (crc & 1)
                crc = (crc >> 1) ^ generator;
            else
                crc >>= 1;
        }
    }

    return crc;
}

static uint8_t from_bcd_to_dec(uint8_t bcd) {

    uint8_t dec = ((bcd >> 4) & 0x0f) * 10 + (bcd & 0x0f);

    return dec;
}

static uint32_t get_running_time(uint8_t *str) {

    uint32_t tl = from_bcd_to_dec(str[0]) * 10000;
    tl += from_bcd_to_dec(str[1]) * 100;
    tl += from_bcd_to_dec(str[2]);

    return tl;
}

static uint8_t send_command(package_t *pkt) {

    size_t len;

//    app_uart_rx_off();
    flush_buff_uart();

    /* three attempts to write to uart */
    for (uint8_t attempt = 0; attempt < 3; attempt++) {
        len = write_bytes_to_uart((uint8_t*)pkt, pkt->pkt_len);
        if (len == pkt->pkt_len) {
            break;
        } else {
            len = 0;
        }
#if UART_PRINTF_MODE && DEBUG_PACKAGE
        printf("Attempt to send data to uart: %u\r\n", attempt+1);
#endif
        sleep_ms(250);
    }


    sleep_ms(200);
//    app_uart_rx_off();

#if UART_PRINTF_MODE && DEBUG_PACKAGE
    if (len == 0) {
        uint8_t head[] = "write to uart error";
        print_package(head, (uint8_t*)pkt, pkt->pkt_len);
    } else {
        uint8_t head[] = "write to uart";
        print_package(head, (uint8_t*)pkt, len);
    }
#endif

    return len;
}

static pkt_error_t response_meter(uint8_t command) {

    uint8_t load_size, load_len = 0;
    pkt_error_no = PKT_ERR_TIMEOUT;
    uint8_t *buff = (uint8_t*)&response_pkt;

    memset(buff, 0, sizeof(package_t));

    /* trying to read for 1 seconds */
    for(uint8_t i = 0; i < 100; i++ ) {
        load_size = 0;
        if (available_buff_uart()) {
            while(available_buff_uart() && load_size < PKT_BUFF_MAX_LEN) {
                buff[load_size++] = read_byte_from_buff_uart();
                load_len++;
            }

            if (load_size > 6) break;
        }
        sleep_ms(10);
    }

    if (load_size) {
#if UART_PRINTF_MODE && DEBUG_PACKAGE
        uint8_t head[] = "read from uart";
        print_package(head, buff, load_size);
#endif
        if (load_size > 6) {
            response_pkt.pkt_len = load_size;
            uint16_t crc = checksum((uint8_t*)&response_pkt, load_size-2);
            uint16_t crc_pkt = ((uint8_t*)&response_pkt)[load_size-2] & 0xff;
            crc_pkt |= (((uint8_t*)&response_pkt)[load_size-1] << 8) & 0xff00;
            if (crc == crc_pkt) {
                if (reverse32(response_pkt.address) == dev_config.device_address) {
                    if (response_pkt.cmd == command) {
                        pkt_error_no = PKT_OK;
                    } else {
                        pkt_error_no = PKT_ERR_DIFFERENT_COMMAND;
                    }
                } else {
                    pkt_error_no = PKT_ERR_ADDRESS;
                }
            } else {
                pkt_error_no = PKT_ERR_CRC;
            }
        } else {
            pkt_error_no = PKT_ERR_RESPONSE;
        }
    } else {
#if UART_PRINTF_MODE && DEBUG_PACKAGE
        uint8_t head[] = "read from uart error";
        print_package(head, buff, load_size);
#endif
    }

#if UART_PRINTF_MODE
    if (pkt_error_no != PKT_OK) print_error(pkt_error_no);
#endif

    return pkt_error_no;
}

static void set_command(uint8_t cmd) {

    memset(&request_pkt, 0, sizeof(package_t));

    request_pkt.address = reverse32(dev_config.device_address);
    request_pkt.cmd = cmd;
    request_pkt.pkt_len = 5;
    uint16_t crc = checksum((uint8_t*)&request_pkt, request_pkt.pkt_len);
    request_pkt.data[0] = crc & 0xff;
    request_pkt.data[1] = (crc >> 8) & 0xff;
    request_pkt.pkt_len += 2;

}

static uint32_t tariff_from_bcd(uint32_t tariff_bcd) {

    uint32_t tariff_dec = 0;
    uint8_t *p_tariff = (uint8_t*)&tariff_bcd;

    tariff_dec += from_bcd_to_dec(p_tariff[0]) * 1000000;
    tariff_dec += from_bcd_to_dec(p_tariff[1]) * 10000;
    tariff_dec += from_bcd_to_dec(p_tariff[2]) * 100;
    tariff_dec += from_bcd_to_dec(p_tariff[3]);

    return tariff_dec;
}

static void get_tariffs_data() {

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand get tariffs\r\n");
#endif

    set_command(cmd_tariffs_data);

    if (send_command(&request_pkt)) {
        if (response_meter(cmd_tariffs_data) == PKT_OK) {
            pkt_tariffs_t *pkt_tariffs = (pkt_tariffs_t*)&response_pkt;

            uint64_t tariff = tariff_from_bcd(pkt_tariffs->tariff_1) & 0xffffffffffff;
            uint64_t last_tariff;

            zcl_getAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_1_SUMMATION_DELIVERD, &attr_len, (uint8_t*)&attr_data);
            last_tariff = fromPtoInteger(attr_len, attr_data);

            if (tariff > last_tariff) {
                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_1_SUMMATION_DELIVERD, (uint8_t*)&tariff);
            }
#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
            printf("tariff1: %d\r\n", tariff);
#endif

            tariff = tariff_from_bcd(pkt_tariffs->tariff_2) & 0xffffffffffff;

            zcl_getAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_2_SUMMATION_DELIVERD, &attr_len, (uint8_t*)&attr_data);
            last_tariff = fromPtoInteger(attr_len, attr_data);

            if (tariff > last_tariff) {
                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_2_SUMMATION_DELIVERD, (uint8_t*)&tariff);
            }

#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
            printf("tariff2: %d\r\n", tariff);
#endif

            tariff = tariff_from_bcd(pkt_tariffs->tariff_3) & 0xffffffffffff;

            zcl_getAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_3_SUMMATION_DELIVERD, &attr_len, (uint8_t*)&attr_data);
            last_tariff = fromPtoInteger(attr_len, attr_data);

            if (tariff > last_tariff) {
                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_3_SUMMATION_DELIVERD, (uint8_t*)&tariff);
            }
#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
            printf("tariff3: %d\r\n", tariff);
#endif

            tariff = tariff_from_bcd(pkt_tariffs->tariff_4) & 0xffffffffffff;

            zcl_getAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_4_SUMMATION_DELIVERD, &attr_len, (uint8_t*)&attr_data);
            last_tariff = fromPtoInteger(attr_len, attr_data);

            if (tariff > last_tariff) {
                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_4_SUMMATION_DELIVERD, (uint8_t*)&tariff);
            }
#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
            printf("tariff4: %d\r\n", tariff);
#endif
        }
    }
}

static void get_net_params_data() {

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand get net parameters (power, voltage, current)\r\n");
#endif

    set_command(cmd_net_params);

    if (send_command(&request_pkt)) {
        if (response_meter(cmd_net_params) == PKT_OK) {
            pkt_net_params_t *pkt_net_params = (pkt_net_params_t*)&response_pkt;

            uint16_t volts = 0;
            uint8_t *volts_bcd = (uint8_t*)&pkt_net_params->volts;
            volts += from_bcd_to_dec(volts_bcd[0]) * 100;
            volts += from_bcd_to_dec(volts_bcd[1]);

            zcl_getAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_RMS_VOLTAGE, &attr_len, (uint8_t*)&attr_data);
            uint16_t last_volts = fromPtoInteger(attr_len, attr_data);

            if (volts != last_volts) {
                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_RMS_VOLTAGE, (uint8_t*)&volts);
            }

#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
            printf("voltage: %d\r\n", volts);
#endif

            uint16_t amps = 0;
            uint8_t *amps_bcd = (uint8_t*)&pkt_net_params->amps;
            amps += from_bcd_to_dec(amps_bcd[0]) * 100;
            amps += from_bcd_to_dec(amps_bcd[1]);

            //amps = 1050;

            zcl_getAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_LINE_CURRENT, &attr_len, (uint8_t*)&attr_data);
            uint16_t last_amps = fromPtoInteger(attr_len, attr_data);

            if (amps != last_amps) {
                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_LINE_CURRENT, (uint8_t*)&amps);
            }
#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
            printf("amps: %d\r\n", amps);
#endif

            uint32_t power = 0;
            power += from_bcd_to_dec(pkt_net_params->power[0]) * 10000;
            power += from_bcd_to_dec(pkt_net_params->power[1]) * 100;
            power += from_bcd_to_dec(pkt_net_params->power[2]);

            while (power > 0xffff) power /= 10;

            uint16_t pwr = power & 0xffff;

            //pwr = 3012;

            zcl_getAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_APPARENT_POWER, &attr_len, (uint8_t*)&attr_data);
            uint16_t last_pwr = fromPtoInteger(attr_len, attr_data);

            if (pwr != last_pwr) {
                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_APPARENT_POWER, (uint8_t*)&pwr);
            }

#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
            printf("power: %d\r\n", pwr);
#endif


        }
    }
}

static void get_resbat_data() {

#if UART_PRINTF_MODE
    printf("\r\nCommand get resource of battery\r\n");
#endif

    uint8_t lifetime = RESOURCE_BATTERY, worktime;

    set_command(cmd_running_time);

    if (send_command(&request_pkt)) {
        if (response_meter(cmd_running_time) == PKT_OK) {
            pkt_running_time_t *pkt_running_time = (pkt_running_time_t*)&response_pkt;

            uint32_t rt = get_running_time(pkt_running_time->tl);
            rt += get_running_time(pkt_running_time->tlb);

            worktime = lifetime - (rt / 24 / 30);

            uint8_t battery_level = (worktime*100)/lifetime;

            if (((worktime*100)%lifetime) >= (lifetime/2)) {
                battery_level++;
            }

            zcl_getAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_REMAINING_BATTERY_LIFE, &attr_len, (uint8_t*)&attr_data);
            uint8_t last_bl = fromPtoInteger(attr_len, attr_data);

            if (battery_level != last_bl) {
                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_REMAINING_BATTERY_LIFE, (uint8_t*)&battery_level);
            }

#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
            printf("Resource battery: %d.%d\r\n", (worktime*100)/lifetime, ((worktime*100)%lifetime)*100/lifetime);
#endif

        }
    }
}

static void get_date_release_data() {

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand get date release\r\n");
#endif

    set_command(cmd_date_release);

    if (send_command(&request_pkt)) {
        if (response_meter(cmd_date_release) == PKT_OK) {
            pkt_release_t *pkt_release = (pkt_release_t*)&response_pkt;
            uint8_t release_day = from_bcd_to_dec(pkt_release->date[0]);
            uint8_t release_month = from_bcd_to_dec(pkt_release->date[1]);
            uint8_t release_year = from_bcd_to_dec(pkt_release->date[2]);
            uint8_t date_release[DATA_MAX_LEN+2] = {0};
            uint8_t dr[11] = {0};
            uint8_t dr_len = 0;


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

            uint16_t year = 2000 + release_year;

            itoa(year, year_str);

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


static void get_serial_number_data() {

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand get serial number\r\n");
#endif

    set_command(cmd_serial_number);

    if (send_command(&request_pkt)) {
        if (response_meter(cmd_serial_number) == PKT_OK) {

            pkt_serial_num_t *pkt_serial_num = (pkt_serial_num_t*)&response_pkt;

            uint32_t addr = reverse32(pkt_serial_num->addr);
            uint8_t serial_number[SE_ATTR_SN_SIZE+1] = {0};
            uint8_t sn[SE_ATTR_SN_SIZE];

            itoa(addr, sn);

            if (set_zcl_str(sn, serial_number, SE_ATTR_SN_SIZE)) {
                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_METER_SERIAL_NUMBER, (uint8_t*)&serial_number);
#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
            printf("Serial Number: %s, len: %d\r\n", serial_number+1, *serial_number);
#endif
            }
        }
    }
}

static uint8_t get_timeout_data() {

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand ping of device\r\n");
#endif

    set_command(cmd_timeout);

    if (send_command(&request_pkt)) {
        if (response_meter(cmd_timeout) == PKT_OK) {
            return true;
        }
    }

    return false;
}

uint8_t measure_meter_mercury_206() {

    get_timeout_data();     /* does not respond to the first command after a pause. fake command */

    uint8_t ret = get_timeout_data();

    if (ret) {
        if (new_start) {
            get_serial_number_data();
            get_date_release_data();
            new_start = false;
        }
        get_net_params_data();
        get_tariffs_data();
        get_resbat_data();

        fault_measure_flag = false;
    } else {
        fault_measure_flag = true;
        if (!timerFaultMeasurementEvt) {
            timerFaultMeasurementEvt = TL_ZB_TIMER_SCHEDULE(fault_measure_meterCb, NULL, TIMEOUT_10MIN);
        }
    }

    return ret;
}

