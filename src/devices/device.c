#include "tl_common.h"
#include "zb_api.h"
#include "zcl_include.h"

#include "device.h"
#include "se_custom_attr.h"
#include "app_main.h"
#include "app_uart.h"

u16 attr_len;
u8 attr_data[8];
u8 new_start = true;
pkt_error_t pkt_error_no;
measure_meter_f measure_meter = NULL;
u8 fault_measure_flag = 0;
ev_timer_event_t *timerFaultMeasurementEvt = NULL;

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
    u8 name[DEVICE_NAME_LEN] = {0};

    u64 tariff = 0;
    u16 power = 0;
    u16 volts = 0;
    u16 current = 0;
    u8 sn[] = "11111111";
    u8 serial_number[SE_ATTR_SN_SIZE] = {0};
    u8 dr[] = "xx.xx.xxxx";
    u8 date_release[DATA_MAX_LEN+2] = {0};



    measure_meter = _measure_meter;

    switch (model) {
        case DEVICE_KASKAD_1_MT: {
#if (METER_MODEL == KASKAD_1_MT)
            energy_divisor = 100;
            voltage_divisor = 100;
            current_divisor = 1000;
            power_divisor = 100;
            if (set_zcl_str(device_model[DEVICE_KASKAD_1_MT], name, DEVICE_NAME_LEN)) {
                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CUSTOM_DEVICE_MODEL, (u8*)&name);
            }
#else
            measure_meter = NULL;
            u8 ns[] = "Firmware not include KASKADE-1-MT";
            if (set_zcl_str(ns, name, DEVICE_NAME_LEN)) {
                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CUSTOM_DEVICE_MODEL, (u8*)&name);
            }
#endif
            break;
        }
        case DEVICE_KASKAD_11: {
#if (METER_MODEL == KASKAD_11_C1)
//                measure_meter = measure_meter_kaskad11;
            measure_meter = NULL;
            if (set_zcl_str(device_model[DEVICE_KASKAD_11], name, DEVICE_NAME_LEN)) {
                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CUSTOM_DEVICE_MODEL, (u8*)&name);
            }
#else
            measure_meter = NULL;
            u8 ns[] = "Firmware not include KASKADE-11-C1";
            if (set_zcl_str(ns, name, DEVICE_NAME_LEN)) {
                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CUSTOM_DEVICE_MODEL, (u8*)&name);
            }
#endif
            break;
        }
        case DEVICE_MERCURY_206: {
#if (METER_MODEL == MERCURY_206)
            energy_divisor = 100;
            voltage_divisor = 10;
            current_divisor = 100;
            if (set_zcl_str(device_model[DEVICE_MERCURY_206], name, DEVICE_NAME_LEN)) {
                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CUSTOM_DEVICE_MODEL, (u8*)&name);
            }
#else
            measure_meter = NULL;
            u8 ns[] = "Firmware not include MERCURY-206";
            if (set_zcl_str(ns, name, DEVICE_NAME_LEN)) {
                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CUSTOM_DEVICE_MODEL, (u8*)&name);
            }
#endif
            break;
        }
        case DEVICE_ENERGOMERA_CE102M: {
#if (METER_MODEL == ENERGOMERA_CE102M)
            if (set_zcl_str(device_model[DEVICE_ENERGOMERA_CE102M], name, DEVICE_NAME_LEN)) {
                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CUSTOM_DEVICE_MODEL, (u8*)&name);
            }
#else
            measure_meter = NULL;
            u8 ns[] = "Firmware not include ENERGOMERA-CE102M";
            if (set_zcl_str(ns, name, DEVICE_NAME_LEN)) {
                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CUSTOM_DEVICE_MODEL, (u8*)&name);
            }
#endif
            break;
        }
        default:
            measure_meter = NULL;
            if (set_zcl_str(device_model[DEVICE_UNDEFINED], name, DEVICE_NAME_LEN)) {
                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CUSTOM_DEVICE_MODEL, (u8*)&name);
            }
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

    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_1_SUMMATION_DELIVERD, (u8*)&tariff);
    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_2_SUMMATION_DELIVERD, (u8*)&tariff);
    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_3_SUMMATION_DELIVERD, (u8*)&tariff);
    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_4_SUMMATION_DELIVERD, (u8*)&tariff);
    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_APPARENT_POWER, (u8*)&power);
    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_RMS_VOLTAGE, (u8*)&volts);
    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_LINE_CURRENT, (u8*)&current);
    if (set_zcl_str(sn, serial_number, SE_ATTR_SN_SIZE)) {
        zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_METER_SERIAL_NUMBER, (u8*)&serial_number);
    }
    if (set_zcl_str(dr, date_release, DATA_MAX_LEN+1)) {
        zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CUSTOM_DATE_RELEASE, (u8*)&date_release);
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

    /* start timer get data from device */
    if(g_appCtx.timerMeasurementEvt) TL_ZB_TIMER_CANCEL(&g_appCtx.timerMeasurementEvt);
    g_appCtx.timerMeasurementEvt = TL_ZB_TIMER_SCHEDULE(measure_meterCb, NULL, TIMEOUT_1SEC);

    return save;
}

s32 measure_meterCb(void *arg) {

    s32 period = DEFAULT_MEASUREMENT_PERIOD * 1000;

    if (dev_config.device_model && measure_meter) {
        if (measure_meter()) {
            period = dev_config.measurement_period * 1000;
//            period = 15 * 1000;
        } else {
            period = FAULT_MEASUREMENT_PERIOD * 1000;
        }
    }

    return period;
}

s32 fault_measure_meterCb(void *arg) {

    if (fault_measure_flag && !dev_config.new_ota) {
#if UART_PRINTF_MODE
        printf("Fault get data from device. Restart!!!\r\n");
#endif
        zb_resetDevice();
    }

    timerFaultMeasurementEvt = NULL;
    return -1;
}

#if UART_PRINTF_MODE && (DEBUG_DEVICE_DATA || DEBUG_PACKAGE)
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

#if DEBUG_PACKAGE
void print_package(u8 *head, u8 *buff, size_t len) {

    u8 ch;

    if (len) {
        printf("%s. len: %d, data: 0x", head, len);

        for (int i = 0; i < len; i++) {
            ch = buff[i];
            if (ch < 0x10) {
                printf("0%x", ch);
            } else {
                printf("%x", ch);
            }
        }
    } else {
        printf("%s. len: %d", head, len);
    }
    printf("\r\n");

}
#endif

#endif
