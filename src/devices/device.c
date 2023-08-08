#include "tl_common.h"

#include "device.h"
#include "app_uart.h"
#include "app_ui.h"

//u8 release_month;
//u8 release_year;
u8 new_start = true;
u8 tariff_changed = true;
u8 pva_changed = true;
pkt_error_t pkt_error_no;
measure_meter_f measure_meter = NULL;

u8 device_model[DEVICE_MAX][32] = {
    {"No Device"},
    {"KASKAD-1-MT"},
    {"KASKAD-11-C1"},
    {"MERCURY-206"},
    {"ENERGOMERA-CE102M"},
};

u16 get_divisor(const u8 division_factor) {

    switch (division_factor & 0x03) {
        case 0: return 1;
        case 1: return 10;
        case 2: return 100;
        case 3: return 1000;
    }

    return 1;
}

u32 from24to32(const u8 *str) {

    u32 value;

    value = str[0] & 0xff;
    value |= (str[1] << 8) & 0xff00;
    value |= (str[2] << 16) & 0xff0000;

    return value;
}

u8 set_device_model(device_model_t model) {
    u8 save = false;

    switch (model) {
        case DEVICE_KASKAD_1_MT:
            measure_meter = measure_meter_kaskad1mt;
            break;
        case DEVICE_KASKAD_11:
//                measure_meter = measure_meter_kaskad11;
            measure_meter = NULL;
            break;
        case DEVICE_MERCURY_206:
//                measure_meter = measure_meter_mercury206;
            measure_meter = NULL;
            break;
        case DEVICE_ENERGOMERA_CE102M:
//                measure_meter = measure_meter_energomera_ce102m;
            measure_meter = NULL;
            break;
        default:
            measure_meter = NULL;
            break;
    }

    if (em_config.device_model != model) {
        em_config.device_model = model;
        save = true;
        write_config();
#if UART_PRINTF_MODE
        printf("New device model '%s'\r\n", device_model[model]);
#endif
    }

    return save;
}

//u8 set_device_type(device_type_t type) {

//    memset(&meter, 0, sizeof(meter_t));
//    new_start = true;
//    u8 save = false;
//    u16 divisor;
//
//    switch (type) {
//        case device_kaskad_1_mt:
//            if (config.save_data.device_type != device_kaskad_1_mt) {
//                config.save_data.device_type = device_kaskad_1_mt;
//                divisor = 0x0a0f;   /* power 1000, voltage 0.1, amps 1, tariffs 10 */
//                memcpy(&config.save_data.divisor, &divisor, sizeof(divisor_t));
//                write_config();
//                save = true;
//#if UART_PRINT_DEBUG_ENABLE
//                printf("New device type KACKAD-1-MT\r\n");
//#endif
//            }
//            meter.measure_meter = measure_meter_kaskad1mt;
//            meter.get_date_release_data = get_date_release_data_kaskad1mt;
//            meter.get_serial_number_data = get_serial_number_data_kaskad1mt;
//            break;
//        case device_kaskad_11:
//            if (config.save_data.device_type != device_kaskad_11) {
//                config.save_data.device_type = device_kaskad_11;
//                divisor = 0x0a05;   /* power 10, voltage 1, amps 1, tariffs 10 */
//                memcpy(&config.save_data.divisor, &divisor, sizeof(divisor_t));
//                write_config();
//                save = true;
//#if UART_PRINT_DEBUG_ENABLE
//                printf("New device type KACKAD-11\r\n");
//#endif
//            }
//            meter.measure_meter = measure_meter_kaskad11;
//            meter.get_date_release_data = get_date_release_data_kaskad11;
//            meter.get_serial_number_data = get_serial_number_data_kaskad11;
//            break;
//        case device_mercury_206:
//            if (config.save_data.device_type != device_mercury_206) {
//                config.save_data.device_type = device_mercury_206;
//                memset(&config.save_data.divisor, 0, sizeof(divisor_t));
//                divisor = 0x0b46;   /* power 100, voltage 1, amps 10, tariffs 10 */
//                memcpy(&config.save_data.divisor, &divisor, sizeof(divisor_t));
//                write_config();
//                save = true;
//#if UART_PRINT_DEBUG_ENABLE
//                printf("New device type Mercury-206\r\n");
//#endif
//            }
//            meter.measure_meter = measure_meter_mercury206;
//            meter.get_date_release_data = get_date_release_data_mercury206;
//            meter.get_serial_number_data = get_serial_number_data_mercury206;
//            break;
//        case device_energomera_ce102m:
//            if (config.save_data.device_type != device_energomera_ce102m) {
//                config.save_data.device_type = device_energomera_ce102m;
//                divisor =  0x0a09;   /* power 0.1, voltage 0.1, amps 1, tariffs 10 */
//                memset(&config.save_data.divisor, 0, sizeof(divisor_t));
//                write_config();
//                save = true;
//#if UART_PRINT_DEBUG_ENABLE
//                printf("New device type Energomera CE-102M\r\n");
//#endif
//            }
//            meter.measure_meter = measure_meter_energomera_ce102m;
//            meter.get_date_release_data = get_date_release_data_energomera_ce102m;
//            meter.get_serial_number_data = get_serial_number_data_energomera_ce102m;
//            break;
//        default:
//            config.save_data.device_type = device_kaskad_1_mt;
//            divisor = 0x0a0f;   /* power 1000, voltage 0.1, amps 1, tariffs 10 */
//            memcpy(&config.save_data.divisor, &divisor, sizeof(divisor_t));
//            meter.measure_meter = measure_meter_kaskad1mt;
//            meter.get_date_release_data = get_date_release_data_kaskad1mt;
//            meter.get_serial_number_data = get_serial_number_data_kaskad1mt;
//            write_config();
//            save = true;
//#if UART_PRINT_DEBUG_ENABLE
//            printf("Default device type KACKAD-1-MT\r\n");
//#endif
//            break;
//    }
//    return save;
//}

#if UART_PRINTF_MODE
void print_error(pkt_error_t err_no) {

    switch (err_no) {
        case PKT_ERR_TIMEOUT:
            printf("Response timed out\r\n");
            break;
        case PKT_ERR_RESPONSE:
            printf("Response error\r\n");
            break;
        case PKT_ERR_UNKNOWN_FORMAT:
        case PKT_ERR_NO_PKT:
            printf("Unknown response format\r\n");
            break;
        case PKT_ERR_DIFFERENT_COMMAND:
            printf("Request and response command are different\r\n");
            break;
        case PKT_ERR_INCOMPLETE:
            printf("Not a complete response\r\n");
            break;
        case PKT_ERR_UNSTUFFING:
            printf("Wrong unstuffing\r\n");
            break;
        case PKT_ERR_ADDRESS:
            printf("Invalid device address\r\n");
            break;
        case PKT_ERR_CRC:
            printf("Wrong CRC\r\n");
            break;
        case PKT_ERR_UART:
            printf("UART is busy\r\n");
            break;
        default:
            break;
    }
}
#endif
