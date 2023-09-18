#include "tl_common.h"
#include "zb_api.h"
#include "zcl_include.h"

#include "device.h"
#include "app_main.h"
#include "app_uart.h"

u16 attr_len;
u8 attr_data[8];
u8 new_start = true;
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

u8 set_device_model(device_model_t model) {
    u8 save = false;
    new_start = true;
    u32 energy_divisor = 1, energy_multiplier = 1;
    u16 voltage_multiplier = 1, voltage_divisor = 1;
    u16 current_multiplier = 1, current_divisor = 1;
    u16 power_multiplier = 1, power_divisor = 1;



    switch (model) {
        case DEVICE_KASKAD_1_MT:
            measure_meter = measure_meter_kaskad1mt;
            energy_divisor = 100;
            voltage_divisor = 100;
            current_divisor = 1000;
            power_divisor = 100;
            break;
        case DEVICE_KASKAD_11:
//                measure_meter = measure_meter_kaskad11;
            measure_meter = NULL;
            break;
        case DEVICE_MERCURY_206:
            measure_meter = measure_meter_mercury206;
            energy_divisor = 100;
            voltage_divisor = 10;
            current_divisor = 100;
            break;
        case DEVICE_ENERGOMERA_CE102M:
            measure_meter = measure_meter_energomera_ce102m;
            break;
        default:
            measure_meter = NULL;
            break;
    }

    if (dev_config.device_model != model) {
        dev_config.device_model = model;
        save = true;
        write_config();
#if UART_PRINTF_MODE
        printf("New device model '%s'\r\n", device_model[model]);
#endif
    }

    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_MULTIPLIER, (u8*)&energy_multiplier);
    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_DIVISOR, (u8*)&energy_divisor);
    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_AC_VOLTAGE_MULTIPLIER, (u8*)&voltage_multiplier);
    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_AC_VOLTAGE_DIVISOR, (u8*)&voltage_divisor);
    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_AC_CURRENT_MULTIPLIER, (u8*)&current_multiplier);
    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_AC_CURRENT_DIVISOR, (u8*)&current_divisor);
    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_AC_POWER_MULTIPLIER, (u8*)&power_multiplier);
    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_AC_POWER_DIVISOR, (u8*)&power_divisor);

    app_uart_init();

    return save;
}

s32 measure_meterCb(void *arg) {

    s32 period = DEFAULT_MEASUREMENT_PERIOD * 1000;

    if (dev_config.device_model && measure_meter) {
        if (measure_meter()) {
            period = dev_config.measurement_period * 1000;
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
