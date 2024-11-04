#include "tl_common.h"
#include "zcl_include.h"

#include "app_main.h"

#define DEBOUNCE_COUNTER    32                          /* number of polls for debounce       */

static uint8_t tamper_debounce1 = 1;
static uint8_t tamper_debounce2 = 1;

enum status_t {
    CHECK_METER = 0,
    LOW_BATTERY,
    TAMPER_DETECT,
    POWER_FAILURE,
    POWER_QUALITY,
    LEAK_DETECT,
    SERVICE_DISCONNECT,
    RESERVED
};


void tamper_handler() {

//    zcl_seAttr_t *seAttrs = zcl_seAttrs();

    uint16_t attr_len;
    uint8_t attr_data;

    if (!drv_gpio_read(TAMPER)) {
        if (tamper_debounce1 != DEBOUNCE_COUNTER) {
            tamper_debounce1++;
            if (tamper_debounce1 == DEBOUNCE_COUNTER) {
#if UART_PRINTF_MODE && DEBUG_TAMPER
                printf("Tamper low\r\n");
#endif /* UART_PRINTF_MODE */
                zcl_getAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_STATUS, &attr_len, (uint8_t*)&attr_data);
                attr_data &= ~(1 << TAMPER_DETECT);
                printf("attr_data: 0x%x\r\n", attr_data);
                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_STATUS, (uint8_t*)&attr_data);
            }
        }
    } else {
        if (tamper_debounce1 != 1) {
            tamper_debounce1--;
            if (tamper_debounce1 == 1) {
#if UART_PRINTF_MODE && DEBUG_TAMPER
                printf("Tamper high\r\n");
#endif /* UART_PRINTF_MODE */
                zcl_getAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_STATUS, &attr_len, (uint8_t*)&attr_data);
                attr_data |= (1 << TAMPER_DETECT);
                printf("attr_data: 0x%x\r\n", attr_data);
                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_STATUS, (uint8_t*)&attr_data);
            }
        }
    }

    if (tamper_idle()) {
        sleep_ms(1);
    }
}

uint8_t tamper_idle() {
    if ((tamper_debounce1 != 1 && tamper_debounce1 != DEBOUNCE_COUNTER) ||
        (tamper_debounce2 != 1 && tamper_debounce2 != DEBOUNCE_COUNTER)) {
        return true;
    }
    return false;
}

