#include "tl_common.h"
#include "zcl_include.h"

#include "se_custom_attr.h"
#include "app_uart.h"
#include "app_endpoint_cfg.h"
#include "app_dev_config.h"
#include "device.h"
#include "kaskad_1_mt.h"

#define START       0x73
#define BOUNDARY    0x55
#define PROG_ADDR   0xffff
#define PASSWORD    0x00000000
#define PARAMS_LEN  0x20
#define STUFF_55    0x11
#define STUFF_73    0x22

static package_t    request_pkt;
static package_t    response_pkt;
static uint8_t      package_buff[PKT_BUFF_MAX_LEN];
static mirtek_version_t mirtek_version = version_unknown;
static uint8_t serial_number[SE_ATTR_SN_SIZE+1] = {0};
static uint8_t date_release[DATA_MAX_LEN+2] = {0};


static uint8_t checksum(const uint8_t *src_buffer, uint8_t len) {
  // skip 73 55 header (and 55 footer is beyond checksum anyway)
  const uint8_t* table = &src_buffer[2];
  const uint8_t packet_len = len - 4;

  const uint8_t generator = 0xA9;

  uint8_t crc = 0;
  for(const uint8_t* ptr = table; ptr < table + packet_len; ptr++){
    crc ^= *ptr;
    for (uint8_t bit = 8; bit > 0; bit--)
      if (crc & 0x80)
        crc = (crc << 1) ^ generator;
      else
        crc <<= 1;
  }

  return crc;
}

static void set_command(command_t command) {

    memset(&request_pkt, 0, sizeof(package_t));

    request_pkt.start = START;
    request_pkt.boundary = BOUNDARY;
    request_pkt.header.from_to = 1; // to device
    request_pkt.header.address_to = dev_config.device_address; // = 20109;
    request_pkt.header.address_from = PROG_ADDR;
    request_pkt.header.command = command & 0xff;
    if (dev_config.device_password.size != 0) {
        request_pkt.header.password_status = atoi(dev_config.device_password.size, dev_config.device_password.data) & 0xffffffff;
    } else {
        request_pkt.header.password_status = PASSWORD;
    }

    printf("password: %d\r\n", request_pkt.header.password_status);

    switch (command) {
        case cmd_open_channel:
        case cmd_tariffs_data_v1:
        case cmd_power_data:
        case cmd_read_configure:
        case cmd_get_info:
        case cmd_test_error:
        case cmd_resource_battery:
            request_pkt.pkt_len = 2 + sizeof(package_header_t) + 2;
            request_pkt.data[0] = checksum((uint8_t*)&request_pkt, request_pkt.pkt_len);
            request_pkt.data[1] = BOUNDARY;
            break;
        case cmd_tariffs_data_v3:
        case cmd_amps_data:
        case cmd_volts_data:
        case cmd_serial_number:
        case cmd_date_release:
        case cmd_factory_manufacturer:
        case cmd_name_device:
        case cmd_name_device2:
            request_pkt.header.data_len = 1;
            request_pkt.pkt_len = 2 + sizeof(package_header_t) + 3;
            request_pkt.data[0] = (command >> 8) & 0xff;   // sub command
            request_pkt.data[1] = checksum((uint8_t*)&request_pkt, request_pkt.pkt_len);
            request_pkt.data[2] = BOUNDARY;
            break;
        default:
            break;
    }
}

static size_t byte_stuffing() {

    uint8_t *source, *receiver;
    size_t len = 0;

    source = (uint8_t*)&request_pkt;
    receiver = package_buff;

    *(receiver++) = *(source++);
    len++;
    *(receiver++) = *(source++);
    len++;

    for (int i = 0; i < (request_pkt.pkt_len-3); i++) {
        if (*source == BOUNDARY) {
            *(receiver++) = START;
            len++;
            *receiver = STUFF_55;
        } else if (*source == START) {
            *(receiver++) = START;
            len++;
            *receiver = STUFF_73;
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
    uint8_t *receiver = (uint8_t*)&response_pkt;

    *(receiver++) = *(source++);
    len++;
    *(receiver++) = *(source++);
    len++;

    for (int i = 0; i < (load_len-3); i++) {
        if (*source == START) {
            source++;
            len--;
            if (*source == STUFF_55) {
                *receiver = BOUNDARY;
            } else if (*source == STUFF_73) {
                *receiver = START;
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

    set_command(command);

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
    uint8_t err = 0, ch, complete = false;

    pkt_error_no = PKT_ERR_TIMEOUT;

    memset(package_buff, 0, sizeof(package_buff));

    /* trying to read for 1 seconds */
    for(uint8_t i = 0; i < 100; i++ ) {
        load_size = 0;
        if (available_buff_uart()) {
            while (available_buff_uart() && load_size < PKT_BUFF_MAX_LEN) {

                ch = read_byte_from_buff_uart();

                if (load_size == 0) {
                    if (ch != START) {
                        pkt_error_no = PKT_ERR_NO_PKT;
                        continue;
                    }
                } else if (load_size == 1) {
                    if (ch != BOUNDARY) {
                        load_size = 0;
                        pkt_error_no = PKT_ERR_UNKNOWN_FORMAT;
                        continue;
                    }
                } else if (ch == BOUNDARY) {
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
                response_pkt.pkt_len = len;
                uint8_t crc = checksum((uint8_t*)&response_pkt, response_pkt.pkt_len);
                if (crc == response_pkt.data[(response_pkt.header.data_len)]) {
                    response_status_t *status = (response_status_t*)&response_pkt.header.password_status;
                    if (status->error == PKT_OK) {
                        if (response_pkt.header.address_from == dev_config.device_address) {
                            if (response_pkt.header.command == (command & 0xff)) {
                                pkt_error_no = PKT_OK;
                            } else {
                                pkt_error_no = PKT_ERR_DIFFERENT_COMMAND;
                            }
                        } else {
                            pkt_error_no = PKT_ERR_ADDRESS;
                        }
                    } else {
                        pkt_error_no = PKT_ERR_RESPONSE;
                        err = status->error;
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

static package_t *get_pkt_data(command_t command) {

    if (send_command(command)) {
        if (response_meter(command) == PKT_OK) {
            return &response_pkt;
        }
    }
    return NULL;
}

static uint8_t ping_start_data() {

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand ping of device\r\n");
#endif

    package_t *pkt = get_pkt_data(cmd_open_channel);

    if (pkt) {
        return true;
    }

    return false;
}

static void get_tariffs_data() {

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand get tariffs\r\n");
#endif

    package_t *pkt = NULL;

    uint64_t tariff   = 0;
    uint64_t tariff_1 = 0;
    uint64_t tariff_2 = 0;
    uint64_t tariff_3 = 0;
    uint64_t tariff_4 = 0;

    switch (mirtek_version) {
        case version_1:
            pkt = get_pkt_data(cmd_tariffs_data_v1);
            break;
        case version_3:
            pkt = get_pkt_data(cmd_tariffs_data_v3);
            break;
        default:
            break;
    }

    if (pkt) {

        pkt_tariffs_v1_t *tariffs_response_v1 = (pkt_tariffs_v1_t*)pkt->data;
        pkt_tariffs_v3_t *tariffs_response_v3 = (pkt_tariffs_v3_t*)pkt->data;

        switch (mirtek_version) {
            case version_1:
                tariff_1 = tariffs_response_v1->tariff_1 & 0xffffffffffff;
                tariff_2 = tariffs_response_v1->tariff_2 & 0xffffffffffff;
                tariff_3 = tariffs_response_v1->tariff_3 & 0xffffffffffff;
                tariff_4 = tariffs_response_v1->tariff_4 & 0xffffffffffff;
                break;
            case version_3:
                tariff_1 = tariffs_response_v3->tariff_1 & 0xffffffffffff;
                tariff_2 = tariffs_response_v3->tariff_2 & 0xffffffffffff;
                tariff_3 = tariffs_response_v3->tariff_3 & 0xffffffffffff;
                tariff_4 = tariffs_response_v3->tariff_4 & 0xffffffffffff;
                break;
            default:
                break;
        }




//        uint64_t last_tariff;
//
//        zcl_getAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_1_SUMMATION_DELIVERD, &attr_len, (uint8_t*)&attr_data);
//        last_tariff = fromPtoInteger(attr_len, attr_data);
//
//        if (tariff > last_tariff) {
            zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_1_SUMMATION_DELIVERD, (uint8_t*)&tariff_1);
//        }

#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
        printf("tariff1: %d\r\n", tariff_1);
#endif


//        zcl_getAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_2_SUMMATION_DELIVERD, &attr_len, (uint8_t*)&attr_data);
//        last_tariff = fromPtoInteger(attr_len, attr_data);
//
//        if (tariff > last_tariff) {
            zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_2_SUMMATION_DELIVERD, (uint8_t*)&tariff_2);
//        }

#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
        printf("tariff2: %d\r\n", tariff_2);
#endif


//        zcl_getAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_3_SUMMATION_DELIVERD, &attr_len, (uint8_t*)&attr_data);
//        last_tariff = fromPtoInteger(attr_len, attr_data);
//
//        if (tariff > last_tariff) {
            zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_3_SUMMATION_DELIVERD, (uint8_t*)&tariff_3);
//        }

#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
        printf("tariff3: %d\r\n", tariff_3);
#endif


//        zcl_getAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_4_SUMMATION_DELIVERD, &attr_len, (uint8_t*)&attr_data);
//        last_tariff = fromPtoInteger(attr_len, attr_data);
//
//        if (tariff > last_tariff) {
            zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_4_SUMMATION_DELIVERD, (uint8_t*)&tariff_4);
//        }

#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
        printf("tariff4: %d\r\n", tariff_4);
#endif

        tariff = tariff_1 + tariff_2 + tariff_3 + tariff_4;
        zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_SUMMATION_DELIVERD, (uint8_t*)&tariff);

    }
}

static void get_amps_data() {

    uint32_t amps;

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand get current\r\n");
#endif

    package_t *pkt = get_pkt_data(cmd_amps_data);

    if (pkt) {

        pkt_amps_t *amps_response = (pkt_amps_t*)pkt->data;

        /* pkt->header.data_len == 3 -> amps 2 bytes
         * pkt->header.data_len == 4 -> amps 3 bytes
         */
        if (pkt->header.data_len == 3) {
            amps = amps_response->amps[0];
            amps |= (amps_response->amps[1] << 8) & 0xff00;
        } else {
            amps = from24to32(amps_response->amps);
        }

        /* current has 2 bytes in the home assistant */
        while (amps > 0xffff) amps /= 10;

        uint16_t current = amps & 0xffff;

//        zcl_getAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_LINE_CURRENT, &attr_len, (uint8_t*)&attr_data);
//        uint16_t last_amps = fromPtoInteger(attr_len, attr_data);
//
//        if (current != last_amps) {
            zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_LINE_CURRENT, (uint8_t*)&current);
//        }


#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
        printf("phase: %d, amps: %d\r\n", amps_response->phase_num, current);
#endif

    }
}

static void get_voltage_data() {

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand get voltage\r\n");
#endif

    package_t *pkt = get_pkt_data(cmd_volts_data);

    if (pkt) {

        pkt_volts_t *volts_response = (pkt_volts_t*)pkt->data;

//        zcl_getAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_RMS_VOLTAGE, &attr_len, (uint8_t*)&attr_data);
//        uint16_t last_volts = fromPtoInteger(attr_len, attr_data);
//
//        if (volts_response->volts != last_volts) {
            zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_RMS_VOLTAGE, (uint8_t*)&volts_response->volts);
//        }

#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
        printf("phase: %d, volts: %d\r\n", volts_response->phase_num, volts_response->volts);
#endif

    }
}

static void get_power_data() {

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand get power\r\n");
#endif

    package_t *pkt = get_pkt_data(cmd_power_data);


    if (pkt) {

        pkt_power_t *power_response = (pkt_power_t*)pkt->data;

        uint32_t power = from24to32(power_response->power);

        while (power > 0xffff) power /= 10;

        uint16_t pwr = power & 0xffff;

//        zcl_getAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_APPARENT_POWER, &attr_len, (uint8_t*)&attr_data);
//        uint16_t last_pwr = fromPtoInteger(attr_len, attr_data);
//
//        if (pwr != last_pwr) {
            zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_APPARENT_POWER, (uint8_t*)&pwr);
//        }

#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
        printf("power: %d\r\n", pwr);
#endif
    }
}

void get_serial_number_data() {

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand get serial number\r\n");
#endif

    package_t *pkt = get_pkt_data(cmd_serial_number);

    if (pkt) {

        pkt_data31_t *serial_number_response = (pkt_data31_t*)pkt;

        if (set_zcl_str(serial_number_response->data, serial_number, SE_ATTR_SN_SIZE)) {
            zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_METER_SERIAL_NUMBER, (uint8_t*)&serial_number);
#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
            printf("Serial Number: %s, len: %d\r\n", serial_number+1, *serial_number);
#endif
        }
    }
}

void get_date_release_data() {

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
    printf("\r\nCommand get date release\r\n");
#endif

    package_t *pkt = get_pkt_data(cmd_date_release);

    if (pkt) {

        pkt_data31_t *date_release_response = (pkt_data31_t*)pkt;

        if (set_zcl_str(date_release_response->data, date_release, DATA_MAX_LEN+1)) {
            zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CUSTOM_DATE_RELEASE, (uint8_t*)&date_release);
#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
        printf("Date of release: %s, len: %d\r\n", date_release+1, *date_release);
#endif
        }
    }
}

static void get_resbat_data() {

#if UART_PRINTF_MODE
    printf("\r\nCommand get resource of battery\r\n");
#endif

    package_t *pkt = get_pkt_data(cmd_resource_battery);

    if (pkt) {

        pkt_resbat_t *resbat = (pkt_resbat_t*)pkt->data;

        uint8_t battery_level = (resbat->worktime*100)/resbat->lifetime;

        if (((resbat->worktime*100)%resbat->lifetime) >= (resbat->lifetime/2)) {
            battery_level++;
        }

//        zcl_getAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_REMAINING_BATTERY_LIFE, &attr_len, (uint8_t*)&attr_data);
//        uint8_t last_bl = fromPtoInteger(attr_len, attr_data);
//
//        if (battery_level != last_bl) {
            zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_REMAINING_BATTERY_LIFE, (uint8_t*)&battery_level);
//        }

#if UART_PRINTF_MODE && DEBUG_DEVICE_DATA
        printf("Resource battery: %d.%d\r\n", (resbat->worktime*100)/resbat->lifetime,
                                             ((resbat->worktime*100)%resbat->lifetime)*100/resbat->lifetime);
#endif
    }
}

static void get_info_data() {

#if UART_PRINTF_MODE
    printf("\r\nCommand get info\r\n");
#endif

    package_t *pkt = get_pkt_data(cmd_get_info);

    if (pkt) {
        printf("board id: %x\r\n", pkt->data[0]);
    }
}

static uint8_t get_cfg_data() {

#if UART_PRINTF_MODE
    printf("\r\nCommand get configuration\r\n");
#endif

    package_t *pkt = get_pkt_data(cmd_read_configure);

    if (pkt) {
        if (pkt->header.data_len == 7) {
#if UART_PRINTF_MODE
            printf("\r\nMirtek protocol v1\r\n");
#endif
            mirtek_version = version_1;
        } else {
#if UART_PRINTF_MODE
            printf("\r\nMirtek protocol v3\r\n");
#endif
            mirtek_version = version_3;
        }
//        pkt_read_cfg_t *cfg = (pkt_read_cfg_t*)pkt->data;
//        printf("cfg role: %x\r\n", cfg->role);
    }

    return mirtek_version;
}

void pkt_test(command_t command) {
    package_t *pkt;
    pkt = get_pkt_data(command);

    if (pkt) {
    } else {
        printf("pkt = NULL\r\n");
    }
}

uint8_t measure_meter_kaskad_1_mt() {

    uint8_t ret = ping_start_data();        /* ping to device       */

    if (ret) {
//        get_info_data();
        get_cfg_data();
        if (new_start) {               /* after reset          */
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

        get_tariffs_data();            /* get 4 tariffs        */
        get_resbat_data();             /* get resource battery */
        get_voltage_data();            /* get voltage net ~220 */
        get_power_data();              /* get power            */
        get_amps_data();               /* get amps             */

        fault_measure_flag = false;
    } else {
        fault_measure_flag = true;
        if (!timerFaultMeasurementEvt) {
            timerFaultMeasurementEvt = TL_ZB_TIMER_SCHEDULE(fault_measure_meterCb, NULL, /*TIMEOUT_30SEC*/TIMEOUT_10MIN);
        }
    }

    return ret;
}

