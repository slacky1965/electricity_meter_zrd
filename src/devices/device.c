#include "tl_common.h"
#include "zb_api.h"
#include "zcl_include.h"

#include "device.h"
#include "se_custom_attr.h"
#include "app_main.h"
#include "app_uart.h"

uint16_t attr_len;
uint8_t attr_data[8];
uint8_t new_start = true;
pkt_error_t pkt_error_no;
measure_meter_f measure_meter = NULL;
uint8_t fault_measure_flag = 0;
ev_timer_event_t *timerFaultMeasurementEvt = NULL;

uint8_t device_model[DEVICE_MAX][32] = {
    {"No Device"},
    {"KASKAD-1-MT"},
    {"KASKAD-11-C1"},
    {"MERCURY-206"},
    {"ENERGOMERA-CE102M"},
    {"ENERGOMERA-CE208BY"},
    {"NEVA-MT124"},
    {"NARTIS-100"},
};

uint8_t set_device_model(device_model_t model) {

    uint8_t save = false;
    new_start = true;
    uint32_t baudrate = BAUDRATE_UART;

    uint32_t energy_divisor = 1, energy_multiplier = 1;
    uint16_t voltage_multiplier = 1, voltage_divisor = 1;
    uint16_t current_multiplier = 1, current_divisor = 1;
    uint16_t power_multiplier = 1, power_divisor = 1;
    uint8_t name[DEVICE_NAME_LEN] = {0};

    uint64_t tariff = 0; //xffffffffffff;
    uint16_t power = 0; //xffff;
    uint16_t volts = 0; //xffff;
    uint16_t current = 0; //xffff;
    uint8_t sn[] = "11111111";
    uint8_t serial_number[SE_ATTR_SN_SIZE] = {0};
    uint8_t dr[] = "xx.xx.xxxx";
    uint8_t date_release[DATA_MAX_LEN+2] = {0};

    measure_meter = NULL;

    fault_measure_flag = false;

    switch (model) {
        case DEVICE_KASKAD_1_MT: {
            measure_meter = measure_meter_kaskad_1_mt;
            baudrate = 9600;
            energy_divisor = 100;
            voltage_divisor = 100;
            current_divisor = 1000;
            power_divisor = 100;
            if (set_zcl_str(device_model[DEVICE_KASKAD_1_MT], name, DEVICE_NAME_LEN)) {
                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CUSTOM_DEVICE_MODEL, (uint8_t*)&name);
            }
            break;
        }
        case DEVICE_KASKAD_11: {
            measure_meter = measure_meter_energomera_ce208by;
            baudrate = 9600;
            if (set_zcl_str(device_model[DEVICE_ENERGOMERA_CE208BY], name, DEVICE_NAME_LEN)) {
                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CUSTOM_DEVICE_MODEL, (uint8_t*)&name);
            }
            break;
            measure_meter = measure_meter_kaskad_11;
            baudrate = 2400;
            if (set_zcl_str(device_model[DEVICE_KASKAD_11], name, DEVICE_NAME_LEN)) {
                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CUSTOM_DEVICE_MODEL, (uint8_t*)&name);
            }
            break;
        }
        case DEVICE_MERCURY_206: {
            measure_meter = measure_meter_mercury_206;
            baudrate = 9600;
            energy_divisor = 100;
            voltage_divisor = 10;
            current_divisor = 100;
            if (set_zcl_str(device_model[DEVICE_MERCURY_206], name, DEVICE_NAME_LEN)) {
                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CUSTOM_DEVICE_MODEL, (uint8_t*)&name);
            }
            break;
        }
        case DEVICE_ENERGOMERA_CE102M: {
            measure_meter = measure_meter_energomera_ce102m;
            baudrate = 9600;
            if (set_zcl_str(device_model[DEVICE_ENERGOMERA_CE102M], name, DEVICE_NAME_LEN)) {
                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CUSTOM_DEVICE_MODEL, (uint8_t*)&name);
            }
            break;
        }
        case DEVICE_ENERGOMERA_CE208BY: {
            measure_meter = measure_meter_energomera_ce208by;
            baudrate = 9600;
            energy_divisor = 10000;
            voltage_divisor = 100;
            current_divisor = 100;
            power_divisor = 100;
            if (set_zcl_str(device_model[DEVICE_ENERGOMERA_CE208BY], name, DEVICE_NAME_LEN)) {
                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CUSTOM_DEVICE_MODEL, (uint8_t*)&name);
            }
            break;
        }
        case DEVICE_NEVA_MT124: {
            measure_meter = measure_meter_neva_mt124;
            baudrate = 300;
            if (set_zcl_str(device_model[DEVICE_NEVA_MT124], name, DEVICE_NAME_LEN)) {
                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CUSTOM_DEVICE_MODEL, (uint8_t*)&name);
            }
            break;
        }
        case DEVICE_NARTIS_100: {
            nartis100_init();
            measure_meter = measure_meter_nartis_100;
            baudrate = 9600;
            energy_divisor = 1000;
            voltage_divisor = 100;
            current_divisor = 1000;
            power_divisor = 1000;

            if (set_zcl_str(device_model[DEVICE_NARTIS_100], name, DEVICE_NAME_LEN)) {
                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CUSTOM_DEVICE_MODEL, (uint8_t*)&name);
            }
            break;
        }
        default:
            if (set_zcl_str(device_model[DEVICE_UNDEFINED], name, DEVICE_NAME_LEN)) {
                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CUSTOM_DEVICE_MODEL, (uint8_t*)&name);
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

    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_1_SUMMATION_DELIVERD, (uint8_t*)&tariff);
    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_2_SUMMATION_DELIVERD, (uint8_t*)&tariff);
    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_3_SUMMATION_DELIVERD, (uint8_t*)&tariff);
    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_4_SUMMATION_DELIVERD, (uint8_t*)&tariff);
    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_APPARENT_POWER, (uint8_t*)&power);
    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_RMS_VOLTAGE, (uint8_t*)&volts);
    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_LINE_CURRENT, (uint8_t*)&current);
    if (set_zcl_str(sn, serial_number, SE_ATTR_SN_SIZE)) {
        zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_METER_SERIAL_NUMBER, (uint8_t*)&serial_number);
    }
    if (set_zcl_str(dr, date_release, DATA_MAX_LEN+1)) {
        zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CUSTOM_DATE_RELEASE, (uint8_t*)&date_release);
    }

    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_MULTIPLIER, (uint8_t*)&energy_multiplier);
    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_DIVISOR, (uint8_t*)&energy_divisor);
    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_AC_VOLTAGE_MULTIPLIER, (uint8_t*)&voltage_multiplier);
    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_AC_VOLTAGE_DIVISOR, (uint8_t*)&voltage_divisor);
    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_AC_CURRENT_MULTIPLIER, (uint8_t*)&current_multiplier);
    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_AC_CURRENT_DIVISOR, (uint8_t*)&current_divisor);
    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_AC_POWER_MULTIPLIER, (uint8_t*)&power_multiplier);
    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_AC_POWER_DIVISOR, (uint8_t*)&power_divisor);

    app_uart_init(baudrate);

    /* start timer get data from device */
    if(g_appCtx.timerMeasurementEvt) TL_ZB_TIMER_CANCEL(&g_appCtx.timerMeasurementEvt);
    g_appCtx.timerMeasurementEvt = TL_ZB_TIMER_SCHEDULE(measure_meterCb, NULL, TIMEOUT_1SEC);

    return save;
}

int32_t measure_meterCb(void *arg) {

    int32_t period = DEFAULT_MEASUREMENT_PERIOD * 1000;


    if (dev_config.device_model && measure_meter) {
        if (measure_meter()) {
            period = dev_config.measurement_period * 1000;
//            for test
            period = 15 * 1000;
        } else {
            period = FAULT_MEASUREMENT_PERIOD * 1000;
        }
    }

    return period;
}

int32_t fault_measure_meterCb(void *arg) {

    if (fault_measure_flag) {
#if UART_PRINTF_MODE
        printf("Fault get data from device. Reload electricity!!!\r\n");
#endif
//        zb_resetDevice();
        set_device_model(dev_config.device_model);
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
        case PKT_ERR_DEST_ADDRESS:
            printf("Invalid destination address\r\n");
            break;
        case PKT_ERR_SRC_ADDRESS:
            printf("Invalid source address\r\n");
            break;
        case PKT_ERR_CRC:
            printf("Wrong CRC\r\n");
            break;
        case PKT_ERR_UART:
            printf("UART is busy\r\n");
            break;
        case PKT_ERR_TYPE:
            printf("Package type not type 3\r\n");
            break;
        case PKT_ERR_SEGMENTATION:
            printf("Segmentation not complete\r\n");
            break;
        default:
            printf("Unknown error\r\n");
            break;
    }
}

#if DEBUG_PACKAGE
void print_package(uint8_t *head, uint8_t *buff, size_t len) {

    uint8_t ch;

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
