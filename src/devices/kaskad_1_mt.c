#include "tl_common.h"

#include "device.h"
#include "kaskad_1_mt.h"
#include "app_uart.h"
#include "app_ui.h"

#define START       0x73
#define BOUNDARY    0x55
#define PROG_ADDR   0xffff
#define PASSWORD    0x00000000
#define PARAMS_LEN  0x20
#define STUFF_55    0x11
#define STUFF_73    0x22

static package_t request_pkt;
static package_t response_pkt;
static u8   package_buff[PKT_BUFF_MAX_LEN];

static u8 checksum(const u8 *src_buffer, u8 len) {
  // skip 73 55 header (and 55 footer is beyond checksum anyway)
  const u8* table = &src_buffer[2];
  const u8 packet_len = len - 4;

  const u8 generator = 0xA9;

  u8 crc = 0;
  for(const u8* ptr = table; ptr < table + packet_len; ptr++){
    crc ^= *ptr;
    for (u8 bit = 8; bit > 0; bit--)
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
    request_pkt.header.address_to = em_config.device_address; // = 20109;
    request_pkt.header.address_from = PROG_ADDR;
    request_pkt.header.command = command & 0xff;
    request_pkt.header.password_status = PASSWORD;

    switch (command) {
        case cmd_open_channel:
        case cmd_tariffs_data:
        case cmd_power_data:
        case cmd_read_configure:
        case cmd_get_info:
        case cmd_test_error:
        case cmd_resource_battery:
            request_pkt.pkt_len = 2 + sizeof(package_header_t) + 2;
            request_pkt.data[0] = checksum((u8*)&request_pkt, request_pkt.pkt_len);
            request_pkt.data[1] = BOUNDARY;
            break;
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
            request_pkt.data[1] = checksum((u8*)&request_pkt, request_pkt.pkt_len);
            request_pkt.data[2] = BOUNDARY;
            break;
        default:
            break;
    }
}

static size_t byte_stuffing() {

    u8 *source, *receiver;
    size_t len = 0;

    source = (u8*)&request_pkt;
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

static size_t byte_unstuffing(u8 load_len) {

    size_t   len = 0;
    u8 *source = package_buff;
    u8 *receiver = (u8*)&response_pkt;

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

static u8 send_command(command_t command) {

    u8 buff_len, len = 0;

    set_command(command);

    buff_len = byte_stuffing();

    /* three attempts to write to uart */
    for (u8 attempt = 0; attempt < 3; attempt++) {
        len = write_bytes_to_uart(package_buff, buff_len);
        if (len == buff_len) {
#if UART_PRINTF_MODE && UART_DEBUG
        printf("send bytes: %d\r\n", len);
#endif
            break;
        } else {
            len = 0;
        }
#if UART_PRINTF_MODE
        printf("Attempt to send data to uart: %d\r\n", attempt+1);
#endif
        sleep_ms(250);
    }

    if (len == 0) {
#if UART_PRINTF_MODE
        printf("Can't send a request pkt\r\n");
#endif
    } else {
        sleep_ms(100);
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
    }

    return len;
}

static pkt_error_t response_meter(command_t command) {

    size_t len, load_size = 0;
    u8 err = 0, ch, complete = false;

    pkt_error_no = PKT_ERR_TIMEOUT;

    memset(package_buff, 0, sizeof(package_buff));

    for (u8 attempt = 0; attempt < 3; attempt ++) {
        load_size = 0;
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
                attempt = 3;
                pkt_error_no = PKT_OK;
                break;
            }
        }
        sleep_ms(250);
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
        printf("\r\n");
#endif
        if (complete) {
            len = byte_unstuffing(load_size);
            if (len) {
                response_pkt.pkt_len = len;
                u8 crc = checksum((u8*)&response_pkt, response_pkt.pkt_len);
                if (crc == response_pkt.data[(response_pkt.header.data_len)]) {
                    response_status_t *status = (response_status_t*)&response_pkt.header.password_status;
                    if (status->error == PKT_OK) {
                        if (response_pkt.header.address_from == em_config.device_address) {
                            printf("response_pkt.header.command: 0x%x, command: 0x%x\r\n", response_pkt.header.command, (command & 0xff));
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
    }

#if UART_PRINTF_MODE
    if (pkt_error_no != PKT_OK) print_error(pkt_error_no);
#endif

    return pkt_error_no;
}

static package_t *get_pkt_data(command_t command) {

    flush_buff_uart();
    if (send_command(command)) {
        if (response_meter(command) == PKT_OK) {
            return &response_pkt;
        }
    }
    return NULL;
}

static u8 ping_start_data() {

#if UART_PRINTF_MODE
    printf("Start of the ping command\r\n");
#endif

    package_t *pkt = get_pkt_data(cmd_open_channel);

    if (pkt) {
        return true;
    }

    return false;
}

static void get_tariffs_data() {

#if UART_PRINTF_MODE
    printf("Start command to receive tariffs\r\n");
#endif

    package_t *pkt = get_pkt_data(cmd_tariffs_data);

    if (pkt) {

        pkt_tariffs_t *tariffs_response = (pkt_tariffs_t*)pkt->data;

        u32 tariff = tariffs_response->tariff_1;

//        if (meter.tariff_1 < tariff) {
//            meter.tariff_1 = tariff;
//            tariff_changed = true;
//            tariff1_notify = NOTIFY_MAX;
//        }

        tariff = tariffs_response->tariff_2;

//        if (meter.tariff_2 < tariff) {
//            meter.tariff_2 = tariff;
//            tariff_changed = true;
//            tariff2_notify = NOTIFY_MAX;
//        }

        tariff = tariffs_response->tariff_3;

//        if (meter.tariff_3 < tariff) {
//            meter.tariff_3 = tariff;
//            tariff_changed = true;
//            tariff3_notify = NOTIFY_MAX;
//        }

#if UART_PRINTF_MODE
//        printf("tariff1: %d\r\n", meter.tariff_1);
//        printf("tariff2: %d\r\n", meter.tariff_2);
//        printf("tariff3: %d\r\n", meter.tariff_3);
#endif

    }
}

static void get_amps_data() {

    u32 amps;

#if UART_PRINTF_MODE
    printf("Start command to receive current\r\n");
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

//        if (meter.amps != amps) {
//            meter.amps = amps;
//            pva_changed = true;
//            ampere_notify = NOTIFY_MAX;
//        }

#if UART_PRINTF_MODE
        printf("phase: %d, amps: %d\r\n", amps_response->phase_num, amps);
#endif

    }
}

static void get_voltage_data() {

#if UART_PRINTF_MODE
    printf("Start command to receive voltage\r\n");
#endif

    package_t *pkt = get_pkt_data(cmd_volts_data);

    if (pkt) {

        pkt_volts_t *volts_response = (pkt_volts_t*)pkt->data;

//        if (meter.voltage != volts_response->volts) {
//            meter.voltage = volts_response->volts;
//            pva_changed = true;
//            voltage_notify = NOTIFY_MAX;
//        }

#if UART_PRINTF_MODE
        printf("phase: %d, volts: %d\r\n", volts_response->phase_num, volts_response->volts);
#endif

    }
}

static void get_power_data() {

    u32 power;

#if UART_PRINTF_MODE
    printf("Start command to receive power\r\n");
#endif

    package_t *pkt = get_pkt_data(cmd_power_data);


    if (pkt) {

        pkt_power_t *power_response = (pkt_power_t*)pkt->data;

        power = from24to32(power_response->power);

//        if (meter.power != power) {
//            meter.power = power;
//            pva_changed = true;
//            power_notify = NOTIFY_MAX;
//        }

#if UART_PRINTF_MODE
        printf("power: %d\r\n", power);
#endif
    }
}

void get_serial_number_data_kaskad1mt() {

#if UART_PRINTF_MODE
    printf("Start command to receive serial number\r\n");
#endif

    package_t *pkt = get_pkt_data(cmd_serial_number);

    if (pkt) {

        pkt_data31_t *serial_number_response = (pkt_data31_t*)pkt;

#if UART_PRINTF_MODE
        printf("Serial Number: %s\r\n", serial_number_response->data);
#endif

//        if (memcmp(meter.serial_number, serial_number_response->data, DATA_MAX_LEN) != 0) {
//            meter.serial_number_len = sprintf((char*)meter.serial_number, "%s", serial_number_response->data);
//            memcpy(serial_number_notify.serial_number, meter.serial_number,
//                   meter.serial_number_len > sizeof(serial_number_notify.serial_number)?
//                   sizeof(serial_number_notify.serial_number):meter.serial_number_len);
//            sn_notify = NOTIFY_MAX;
//        }
    }
}

void get_date_release_data_kaskad1mt() {

#if UART_PRINTF_MODE
    printf("Start command to receive date of release\r\n");
#endif

    package_t *pkt = get_pkt_data(cmd_date_release);

    if (pkt) {

        pkt_data31_t *date_release_response = (pkt_data31_t*)pkt;

#if UART_PRINTF_MODE
        printf("Date of release: %s", date_release_response->data);
#endif

//        if (memcpy(meter.date_release, date_release_response->data, DATA_MAX_LEN) != 0) {
//            meter.date_release_len = sprintf((char*)meter.date_release, "%s", date_release_response->data);
//            memcpy(date_release_notify.date_release, meter.date_release,
//                   meter.date_release_len > sizeof(date_release_notify.date_release)?
//                   sizeof(date_release_notify.date_release):meter.date_release_len);
//            dr_notify = NOTIFY_MAX;
//        }
    }

}

static void get_resbat_data() {

#if UART_PRINTF_MODE
    printf("Start command to receive resource of battery\r\n");
#endif

    package_t *pkt = get_pkt_data(cmd_resource_battery);

    if (pkt) {

        pkt_resbat_t *resbat = (pkt_resbat_t*)pkt->data;

#if UART_PRINTF_MODE
        printf("Resource battery: %d.%d\r\n", (resbat->worktime*100)/resbat->lifetime,
                                             ((resbat->worktime*100)%resbat->lifetime)*100/resbat->lifetime);
#endif

        u8 battery_level = (resbat->worktime*100)/resbat->lifetime;

        if (((resbat->worktime*100)%resbat->lifetime) >= (resbat->lifetime/2)) {
            battery_level++;
        }

//        if (meter.battery_level != battery_level) {
//            meter.battery_level = battery_level;
//            pva_changed = true;
//        }

    }
}



void pkt_test(command_t command) {
    package_t *pkt;
    pkt = get_pkt_data(command);

    if (pkt) {
    } else {
        printf("pkt = NULL\r\n");
    }
}

u8 measure_meter_kaskad1mt() {

    u8 ret = false;

    if (ping_start_data()) {           /* ping to device       */
        if (new_start) {               /* after reset          */
            get_serial_number_data_kaskad1mt();
            get_date_release_data_kaskad1mt();
            new_start = false;
        }
        get_tariffs_data();            /* get 3 tariffs        */
        get_resbat_data();             /* get resource battery */
        get_voltage_data();            /* get voltage net ~220 */
        get_power_data();              /* get power            */
        get_amps_data();               /* get amps             */

        ret = true;
    }

    return ret;
}

