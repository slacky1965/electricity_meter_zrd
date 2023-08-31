#include "tl_common.h"
#include "zcl_include.h"

#include "config.h"
#include "device.h"
#include "mercury_206.h"
#include "app_uart.h"
#include "app_ui.h"
#include "se_custom_attr.h"
#include "electricityMeter.h"

static package_t request_pkt;
static package_t response_pkt;

static u16 checksum(const u8 *src_buffer, u8 len) {

    const u16 generator = 0xa001;

    u16 crc = 0xffff;

    for (const u8 *ptr = src_buffer; ptr < src_buffer + len; ptr++) {
        crc ^= *ptr;

        for (u8 bit = 8; bit > 0; bit--) {
            if (crc & 1)
                crc = (crc >> 1) ^ generator;
            else
                crc >>= 1;
        }
    }

    return crc;
}

static u32 reverse32(u32 in) {
    u32 out;
    u8 *source = (u8*)&in;
    u8 *destination = (u8*)&out;

    destination[3] = source[0];
    destination[2] = source[1];
    destination[1] = source[2];
    destination[0] = source[3];

    return out;
}

static u8 from_bcd_to_dec(u8 bcd) {

    u8 dec = ((bcd >> 4) & 0x0f) * 10 + (bcd & 0x0f);

    return dec;
}

static u32 get_running_time(u8 *str) {

    u32 tl = from_bcd_to_dec(str[0]) * 10000;
    tl += from_bcd_to_dec(str[1]) * 100;
    tl += from_bcd_to_dec(str[2]);

    return tl;
}

static u8 send_command(package_t *pkt) {

    size_t len;

    /* three attempts to write to uart */
    for (u8 attempt = 0; attempt < 3; attempt++) {
        len = write_bytes_to_uart((u8*)pkt, pkt->pkt_len);
        if (len == pkt->pkt_len) {
#if UART_PRINTF_MODE && UART_DEBUG
            printf("send bytes: %u\r\n", len);
#endif
            break;
        } else {
            len = 0;
        }
#if UART_PRINTF_MODE && UART_DEBUG
        printf("Attempt to send data to uart: %u\r\n", attempt+1);
#endif
        sleep_ms(250);
    }

    if (len == 0) {
#if UART_PRINTF_MODE && UART_DEBUG
        printf("Can't send a request pkt\r\n");
#endif
    } else {
        sleep_ms(200);
#if UART_PRINTF_MODE && UART_DEBUG
        printf("request pkt: 0x");
        for (int i = 0; i < len; i++) {
            printf("%02x", ((u8*)pkt)[i]);
        }
        printf("\r\n");
#endif
    }

    return len;
}

static pkt_error_t response_meter(u8 command) {

    u8 load_size, load_len = 0;
    pkt_error_no = PKT_ERR_TIMEOUT;
    u8 *buff = (u8*)&response_pkt;

    memset(buff, 0, sizeof(package_t));

    for (u8 attempt = 0; attempt < 3; attempt ++) {
        load_size = 0;
        while(available_buff_uart() && load_size < PKT_BUFF_MAX_LEN) {
            buff[load_size++] = read_byte_from_buff_uart();
            load_len++;
        }

        if (load_size > 6) break;

        sleep_ms(250);
    }

#if UART_PRINTF_MODE && UART_DEBUG
    printf("read bytes: %u\r\n", load_len);
#endif


    if (load_size) {
#if UART_PRINTF_MODE && UART_DEBUG
        printf("response pkt: 0x");
        for (int i = 0; i < load_size; i++) {
            printf("%02x", buff[i]);
        }
        printf("\r\n");
#endif
        if (load_size > 6) {
            response_pkt.pkt_len = load_size;
            u16 crc = checksum((u8*)&response_pkt, load_size-2);
            u16 crc_pkt = ((u8*)&response_pkt)[load_size-2] & 0xff;
            crc_pkt |= (((u8*)&response_pkt)[load_size-1] << 8) & 0xff00;
            if (crc == crc_pkt) {
                if (reverse32(response_pkt.address) == em_config.device_address) {
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
    }


#if UART_PRINTF_MODE
    if (pkt_error_no != PKT_OK) print_error(pkt_error_no);
#endif

    return pkt_error_no;
}

static void set_command(u8 cmd) {

    memset(&request_pkt, 0, sizeof(package_t));

    request_pkt.address = reverse32(em_config.device_address);
    request_pkt.cmd = cmd;
    request_pkt.pkt_len = 5;
    u16 crc = checksum((u8*)&request_pkt, request_pkt.pkt_len);
    request_pkt.data[0] = crc & 0xff;
    request_pkt.data[1] = (crc >> 8) & 0xff;
    request_pkt.pkt_len += 2;

}

static u32 tariff_from_bcd(u32 tariff_bcd) {

    u32 tariff_dec = 0;
    u8 *p_tariff = (u8*)&tariff_bcd;

    tariff_dec += from_bcd_to_dec(p_tariff[0]) * 1000000;
    tariff_dec += from_bcd_to_dec(p_tariff[1]) * 10000;
    tariff_dec += from_bcd_to_dec(p_tariff[2]) * 100;
    tariff_dec += from_bcd_to_dec(p_tariff[3]);

    return tariff_dec;
}

static void get_tariffs_data() {

#if UART_PRINTF_MODE
    printf("Start command to receive tariffs\r\n");
#endif

    set_command(cmd_tariffs_data);

    if (send_command(&request_pkt)) {
        if (response_meter(cmd_tariffs_data) == PKT_OK) {
            pkt_tariffs_t *pkt_tariffs = (pkt_tariffs_t*)&response_pkt;

            u64 tariff = tariff_from_bcd(pkt_tariffs->tariff_1) & 0xffffffffffff;
            u64 last_tariff;

            zcl_getAttrVal(ELECTRICITY_METER_EP1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_1_SUMMATION_DELIVERD, &attr_len, (u8*)&attr_data);
            last_tariff = fromPtoInteger(attr_len, attr_data);

            if (tariff > last_tariff) {
                zcl_setAttrVal(ELECTRICITY_METER_EP1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_1_SUMMATION_DELIVERD, (u8*)&tariff);
            }
#if UART_PRINTF_MODE
            printf("tariff1: %d\r\n", tariff);
#endif

            tariff = tariff_from_bcd(pkt_tariffs->tariff_2) & 0xffffffffffff;

            zcl_getAttrVal(ELECTRICITY_METER_EP1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_2_SUMMATION_DELIVERD, &attr_len, (u8*)&attr_data);
            last_tariff = fromPtoInteger(attr_len, attr_data);

            if (tariff > last_tariff) {
                zcl_setAttrVal(ELECTRICITY_METER_EP1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_2_SUMMATION_DELIVERD, (u8*)&tariff);
            }

#if UART_PRINTF_MODE
            printf("tariff2: %d\r\n", tariff);
#endif

            tariff = tariff_from_bcd(pkt_tariffs->tariff_3) & 0xffffffffffff;

            zcl_getAttrVal(ELECTRICITY_METER_EP1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_3_SUMMATION_DELIVERD, &attr_len, (u8*)&attr_data);
            last_tariff = fromPtoInteger(attr_len, attr_data);

            if (tariff > last_tariff) {
                zcl_setAttrVal(ELECTRICITY_METER_EP1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_3_SUMMATION_DELIVERD, (u8*)&tariff);
            }
#if UART_PRINTF_MODE
            printf("tariff3: %d\r\n", tariff);
#endif

            tariff = tariff_from_bcd(pkt_tariffs->tariff_4) & 0xffffffffffff;

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

static void get_net_params_data() {

#if UART_PRINTF_MODE
    printf("Start command to receive net parameters (power, voltage, current)\r\n");
#endif

    set_command(cmd_net_params);

    if (send_command(&request_pkt)) {
        if (response_meter(cmd_net_params) == PKT_OK) {
            pkt_net_params_t *pkt_net_params = (pkt_net_params_t*)&response_pkt;

            u16 volts = 0;
            u8 *volts_bcd = (u8*)&pkt_net_params->volts;
            volts += from_bcd_to_dec(volts_bcd[0]) * 100;
            volts += from_bcd_to_dec(volts_bcd[1]);

            zcl_getAttrVal(ELECTRICITY_METER_EP1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_RMS_VOLTAGE, &attr_len, (u8*)&attr_data);
            u16 last_volts = fromPtoInteger(attr_len, attr_data);

            if (volts != last_volts) {
                zcl_setAttrVal(ELECTRICITY_METER_EP1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_RMS_VOLTAGE, (u8*)&volts);
            }

#if UART_PRINTF_MODE
            printf("voltage: %d\r\n", volts);
#endif

            u16 amps = 0;
            u8 *amps_bcd = (u8*)&pkt_net_params->amps;
            amps += from_bcd_to_dec(amps_bcd[0]) * 100;
            amps += from_bcd_to_dec(amps_bcd[1]);

            //amps = 1050;

            zcl_getAttrVal(ELECTRICITY_METER_EP1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_LINE_CURRENT, &attr_len, (u8*)&attr_data);
            u16 last_amps = fromPtoInteger(attr_len, attr_data);

            if (amps != last_amps) {
                zcl_setAttrVal(ELECTRICITY_METER_EP1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_LINE_CURRENT, (u8*)&amps);
            }
#if UART_PRINTF_MODE
            printf("amps:    %d\r\n", amps);
#endif

            u32 power = 0;
            power += from_bcd_to_dec(pkt_net_params->power[0]) * 10000;
            power += from_bcd_to_dec(pkt_net_params->power[1]) * 100;
            power += from_bcd_to_dec(pkt_net_params->power[2]);

            while (power > 0xffff) power /= 10;

            u16 pwr = power & 0xffff;

            pwr = 3012;

            zcl_getAttrVal(ELECTRICITY_METER_EP1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_APPARENT_POWER, &attr_len, (u8*)&attr_data);
            u16 last_pwr = fromPtoInteger(attr_len, attr_data);

            if (pwr != last_pwr) {
                zcl_setAttrVal(ELECTRICITY_METER_EP1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_APPARENT_POWER, (u8*)&pwr);
            }

#if UART_PRINTF_MODE
            printf("power:   %d\r\n", pwr);
#endif


        }
    }
}

static void get_resbat_data() {

#if UART_PRINTF_MODE
    printf("Start command to receive running time (to get battery life)\r\n");
#endif

    u8 lifetime = RESOURCE_BATTERY, worktime;

    set_command(cmd_running_time);

    if (send_command(&request_pkt)) {
        if (response_meter(cmd_running_time) == PKT_OK) {
            pkt_running_time_t *pkt_running_time = (pkt_running_time_t*)&response_pkt;

            u32 rt = get_running_time(pkt_running_time->tl);
            rt += get_running_time(pkt_running_time->tlb);

            worktime = lifetime - (rt / 24 / 30);

            u8 battery_level = (worktime*100)/lifetime;

            if (((worktime*100)%lifetime) >= (lifetime/2)) {
                battery_level++;
            }

            zcl_getAttrVal(ELECTRICITY_METER_EP1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_REMAINING_BATTERY_LIFE, &attr_len, (u8*)&attr_data);
            u8 last_bl = fromPtoInteger(attr_len, attr_data);

            if (battery_level != last_bl) {
                zcl_setAttrVal(ELECTRICITY_METER_EP1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_REMAINING_BATTERY_LIFE, (u8*)&battery_level);
            }

#if UART_PRINTF_MODE
            printf("Resource battery: %d.%d\r\n", (worktime*100)/lifetime, ((worktime*100)%lifetime)*100/lifetime);
#endif

        }
    }
}

static void get_date_release_data_mercury206() {

#if UART_PRINTF_MODE
    printf("Start command to receive date of release\r\n");
#endif

    set_command(cmd_date_release);

    if (send_command(&request_pkt)) {
        if (response_meter(cmd_date_release) == PKT_OK) {
            pkt_release_t *pkt_release = (pkt_release_t*)&response_pkt;
            u8 release_day = from_bcd_to_dec(pkt_release->date[0]);
            u8 release_month = from_bcd_to_dec(pkt_release->date[1]);
            u8 release_year = from_bcd_to_dec(pkt_release->date[2]);
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
                zcl_setAttrVal(ELECTRICITY_METER_EP1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CUSTOM_DATE_RELEASE, (u8*)&date_release);
#if UART_PRINTF_MODE
                printf("Date of release: %s\r\n", date_release+1);
#endif
            }
        }
    }
}


static void get_serial_number_data_mercury206() {

#if UART_PRINTF_MODE
    printf("Start command to receive serial number\r\n");
#endif

    set_command(cmd_serial_number);

    if (send_command(&request_pkt)) {
        if (response_meter(cmd_serial_number) == PKT_OK) {

            pkt_serial_num_t *pkt_serial_num = (pkt_serial_num_t*)&response_pkt;

            u32 addr = reverse32(pkt_serial_num->addr);
            u8 serial_number[SE_ATTR_SN_SIZE+1] = {0};
            u8 sn[SE_ATTR_SN_SIZE];

            itoa(addr, sn);

            if (set_zcl_str(sn, serial_number, SE_ATTR_SN_SIZE)) {
                zcl_setAttrVal(ELECTRICITY_METER_EP1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_METER_SERIAL_NUMBER, (u8*)&serial_number);
#if UART_PRINTF_MODE
                printf("Serial Number: %s\r\n", serial_number+1);
#endif
            }
        }
    }
}

static u8 get_timeout_data() {

    set_command(cmd_timeout);

    if (send_command(&request_pkt)) {
        if (response_meter(cmd_timeout) == PKT_OK) {
            return true;
        }
    }

    return false;
}

u8 measure_meter_mercury206() {

    u8 ret = false;

    get_timeout_data();     /* does not respond to the first command after a pause. fake command */
    sleep_ms(500);

    if (get_timeout_data()) {
        if (new_start) {
            get_serial_number_data_mercury206();
            get_date_release_data_mercury206();
            new_start = false;
        }
        get_net_params_data();
        get_tariffs_data();
        get_resbat_data();

        fault_measure_counter = 0;
        ret = true;
    } else {
        fault_measure_counter++;

        if (fault_measure_counter == 10) {
#if UART_PRINTF_MODE
            printf("Fault get data from device. Restart!!!\r\n");
#endif
            zb_resetDevice();
        }
    }

    return ret;
}

