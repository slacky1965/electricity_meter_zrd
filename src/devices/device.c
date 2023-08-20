#include "tl_common.h"
#include "zb_api.h"
#include "zcl_include.h"


#include "device.h"
#include "config.h"
#include "app_uart.h"

u16 attr_len;
u8 attr_data[8];
u8 new_start = true;
//u8 tariff_changed = true;
//u8 pva_changed = true;
pkt_error_t pkt_error_no;
measure_meter_f measure_meter = NULL;
u8 fault_measure_counter = 0;

u8 device_model[DEVICE_MAX][32] = {
    {"No Device"},
    {"KASKAD-1-MT"},
    {"KASKAD-11-C1"},
    {"MERCURY-206"},
    {"ENERGOMERA-CE102M"},
};

//u16 get_divisor(const u8 division_factor) {
//
//    switch (division_factor & 0x03) {
//        case 0: return 1;
//        case 1: return 10;
//        case 2: return 100;
//        case 3: return 1000;
//    }
//
//    return 1;
//}

u8 set_device_model(device_model_t model) {
    u8 save = false;
    new_start = true;

    switch (model) {
        case DEVICE_KASKAD_1_MT:
            measure_meter = measure_meter_kaskad1mt;
            break;
        case DEVICE_KASKAD_11:
//                measure_meter = measure_meter_kaskad11;
            measure_meter = NULL;
            break;
        case DEVICE_MERCURY_206:
            measure_meter = measure_meter_mercury206;
            break;
        case DEVICE_ENERGOMERA_CE102M:
            measure_meter = measure_meter_energomera_ce102m;
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

    app_uart_init();

    return save;
}

s32 measure_meterCb(void *arg) {

    s32 period = DEFAULT_MEASUREMENT_PERIOD * 1000;

    if (em_config.device_model && measure_meter) {
        if (measure_meter()) {
            period = em_config.measurement_period * 1000;
        }
    }

    return period;
}

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
