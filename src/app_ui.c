/********************************************************************************************************
 * @file    app_ui.c
 *
 * @brief   This is the source file for app_ui
 *
 * @author  Zigbee Group
 * @date    2021
 *
 * @par     Copyright (c) 2021, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *			All rights reserved.
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/

/**********************************************************************
 * INCLUDES
 */
#include "tl_common.h"
#include "zb_api.h"
#include "zcl_include.h"
#include "gp.h"
#include "app_uart.h"

#include "app_ui.h"
#include "config.h"
#include "electricityMeter.h"

/**********************************************************************
 * LOCAL CONSTANTS
 */

#define DEBOUNCE_BUTTON     16                          /* number of polls for debounce       */

#define COUNT_FACTORY_RESET 5                           /* number of clicks for factory reset */

/**********************************************************************
 * TYPEDEFS
 */


/**********************************************************************
 * LOCAL VARIABLES
 */

/**********************************************************************
 * GLOBAL VARIABLES
 */

u8 device_online = false;
u8 resp_time = false;


/**********************************************************************
 *  Timers callback
 */

s32 flashLedStatusCb(void *arg) {
    if (zb_isDeviceJoinedNwk() && device_online) {
        light_blink_stop();
        if (em_config.new_ota) {
            light_blink_start(2, 30, 250);
        } else {
            light_blink_start(1, 30, 30);
        }
    } else {
        light_blink_start(3, 30, 250);
    }

    return 0;
}

static s32 checkRespTimeCb(void *arg) {

    if (device_online) {
        if (!resp_time) {
            device_online = false;
#if UART_PRINTF_MODE && DEBUG_LEVEL
            printf("No service!\r\n");
#endif
        }
    } else {
        if (resp_time) {
            device_online = true;
#if UART_PRINTF_MODE && DEBUG_LEVEL
            printf("Device online\r\n");
#endif
        }
    }

    resp_time = false;

    return -1;
}

s32 getTimeCb(void *arg) {

    if(zb_isDeviceJoinedNwk()){
        epInfo_t dstEpInfo;
        TL_SETSTRUCTCONTENT(dstEpInfo, 0);

        dstEpInfo.profileId = HA_PROFILE_ID;
#if FIND_AND_BIND_SUPPORT
        dstEpInfo.dstAddrMode = APS_DSTADDR_EP_NOTPRESETNT;
#else
        dstEpInfo.dstAddrMode = APS_SHORT_DSTADDR_WITHEP;
        dstEpInfo.dstEp = ELECTRICITY_METER_EP1;
        dstEpInfo.dstAddr.shortAddr = 0x0;
#endif
        zclAttrInfo_t *pAttrEntry;
        pAttrEntry = zcl_findAttribute(ELECTRICITY_METER_EP1, ZCL_CLUSTER_GEN_TIME, ZCL_ATTRID_TIME);

        zclReadCmd_t *pReadCmd = (zclReadCmd_t *)ev_buf_allocate(sizeof(zclReadCmd_t) + sizeof(u16));
        if(pReadCmd){
            pReadCmd->numAttr = 1;
            pReadCmd->attrID[0] = ZCL_ATTRID_TIME;

            zcl_read(ELECTRICITY_METER_EP1, &dstEpInfo, ZCL_CLUSTER_GEN_TIME, MANUFACTURER_CODE_NONE, 0, 0, 0, pReadCmd);

            ev_buf_free((u8 *)pReadCmd);

            TL_ZB_TIMER_SCHEDULE(checkRespTimeCb, NULL, TIMEOUT_2SEC);
        }
    }

    return 0;
}

s32 getTemperatureCb(void *arg) {

    u16 temperature = adc_temp_result();

    zcl_setAttrVal(ELECTRICITY_METER_EP1, ZCL_CLUSTER_GEN_DEVICE_TEMP_CONFIG, ZCL_ATTRID_DEV_TEMP_CURR_TEMP, (u8*)&temperature);

    return 0;
}

static s32 delayedMcuResetCb(void *arg) {

    //printf("mcu reset\r\n");
    zb_resetDevice();
    return -1;
}

static s32 delayedFactoryResetCb(void *arg) {

    //printf("factory reset\r\n");
    zb_factoryReset();
//    zb_resetDevice();
    return -1;
}

static s32 delayedFullResetCb(void *arg) {

    //printf("full reset\r\n");
    return -1;
}

/**********************************************************************
 * LOCAL FUNCTIONS
 */

void led_on(u32 pin){
	drv_gpio_write(pin, LED_ON);
}

void led_off(u32 pin){
	drv_gpio_write(pin, LED_OFF);
}

void led_init(void){
	led_off(LED_POWER);
	led_off(LED_STATUS);
}

void light_on(void)
{
    led_on(LED_STATUS);
}

void light_off(void)
{
    led_off(LED_STATUS);
}

void light_init(void)
{
    led_init();
    led_on(LED_POWER);

}


void localPermitJoinState(void){
	static bool assocPermit = 0;
	if(assocPermit != zb_getMacAssocPermit()){
		assocPermit = zb_getMacAssocPermit();
		if(assocPermit){
			led_on(LED_STATUS);
		}else{
			led_off(LED_STATUS);
		}
	}
}

s32 zclLightTimerCb(void *arg)
{
    u32 interval = 0;

    if(g_electricityMeterCtx.sta == g_electricityMeterCtx.oriSta){
        g_electricityMeterCtx.times--;
        if(g_electricityMeterCtx.times <= 0){
            g_electricityMeterCtx.timerLedEvt = NULL;
            return -1;
        }
    }

    g_electricityMeterCtx.sta = !g_electricityMeterCtx.sta;
    if(g_electricityMeterCtx.sta){
        light_on();
        interval = g_electricityMeterCtx.ledOnTime;
    }else{
        light_off();
        interval = g_electricityMeterCtx.ledOffTime;
    }

    return interval;
}

void light_blink_start(u8 times, u16 ledOnTime, u16 ledOffTime)
{
    u32 interval = 0;
    g_electricityMeterCtx.times = times;

    if(!g_electricityMeterCtx.timerLedEvt){
        if(g_electricityMeterCtx.oriSta){
            light_off();
            g_electricityMeterCtx.sta = 0;
            interval = ledOffTime;
        }else{
            light_on();
            g_electricityMeterCtx.sta = 1;
            interval = ledOnTime;
        }
        g_electricityMeterCtx.ledOnTime = ledOnTime;
        g_electricityMeterCtx.ledOffTime = ledOffTime;

        g_electricityMeterCtx.timerLedEvt = TL_ZB_TIMER_SCHEDULE(zclLightTimerCb, NULL, interval);
    }
}

void light_blink_stop(void)
{
    if(g_electricityMeterCtx.timerLedEvt){
        TL_ZB_TIMER_CANCEL(&g_electricityMeterCtx.timerLedEvt);

        g_electricityMeterCtx.times = 0;
        if(g_electricityMeterCtx.oriSta){
            light_on();
        }else{
            light_off();
        }
    }
}

/**********************************************************************
 *  For button
 */

void init_button() {

    memset(&g_electricityMeterCtx.button, 0, sizeof(button_t));

    g_electricityMeterCtx.button.debounce = 1;
    g_electricityMeterCtx.button.released_time = clock_time();

}

void button_handler() {

    if (!drv_gpio_read(BUTTON)) {
        if (g_electricityMeterCtx.button.debounce != DEBOUNCE_BUTTON) {
            g_electricityMeterCtx.button.debounce++;
            if (g_electricityMeterCtx.button.debounce == DEBOUNCE_BUTTON) {
                g_electricityMeterCtx.button.pressed = true;
                g_electricityMeterCtx.button.pressed_time = clock_time();
                if (!clock_time_exceed(g_electricityMeterCtx.button.released_time, TIMEOUT_TICK_1SEC)) {
                    g_electricityMeterCtx.button.counter++;
                } else {
                    g_electricityMeterCtx.button.counter = 1;
                }
            }
        }
    } else {
        if (g_electricityMeterCtx.button.debounce != 1) {
            g_electricityMeterCtx.button.debounce--;
            if (g_electricityMeterCtx.button.debounce == 1 && g_electricityMeterCtx.button.pressed) {
                g_electricityMeterCtx.button.released = true;
                g_electricityMeterCtx.button.released_time = clock_time();
            }
        }
    }

    if (g_electricityMeterCtx.button.pressed && g_electricityMeterCtx.button.released) {
        g_electricityMeterCtx.button.pressed = g_electricityMeterCtx.button.released = false;
        if (clock_time_exceed(g_electricityMeterCtx.button.pressed_time, TIMEOUT_TICK_10SEC)) {
            /* long pressed > 10 sec. */
            /* TODO: full clean (factory reset and clean config) */
            light_blink_start(3, 30, 250);
#if UART_PRINTF_MODE
            printf("Full reset of the device!\r\n");
#endif
            TL_ZB_TIMER_SCHEDULE(delayedFullResetCb, NULL, TIMEOUT_1SEC);
        } else if (clock_time_exceed(g_electricityMeterCtx.button.pressed_time, TIMEOUT_TICK_5SEC)) {
            /* long pressed > 5 sec. */
            light_blink_start(3, 30, 250);
#if UART_PRINTF_MODE
            printf("MCU reset!\r\n");
#endif
            TL_ZB_TIMER_SCHEDULE(delayedMcuResetCb, NULL, TIMEOUT_1SEC);
        } else { /* short pressed < 5 sec. */
            light_blink_start(1, 30, 30);

//            if (!g_electricityMeterCtx.timerForcedReportEvt) {
//                g_electricityMeterCtx.timerForcedReportEvt = TL_ZB_TIMER_SCHEDULE(forcedReportCb, NULL, TIMEOUT_5SEC);
//            }
        }
    } else if (!g_electricityMeterCtx.button.pressed) {
        if (clock_time_exceed(g_electricityMeterCtx.button.released_time, TIMEOUT_TICK_1SEC)) {
            if (g_electricityMeterCtx.button.counter == COUNT_FACTORY_RESET) {
                light_blink_start(3, 30, 250);
                g_electricityMeterCtx.button.counter = 0;
#if UART_PRINTF_MODE
                printf("Factory reset!\r\n");
#endif
                TL_ZB_TIMER_SCHEDULE(delayedFactoryResetCb, NULL, TIMEOUT_1SEC);
            } else {
                g_electricityMeterCtx.button.counter = 0;
            }
        }

    }
}

u8 button_idle() {
    if ((g_electricityMeterCtx.button.debounce != 1 && g_electricityMeterCtx.button.debounce != DEBOUNCE_BUTTON)
            || g_electricityMeterCtx.button.pressed
            || g_electricityMeterCtx.button.counter) {
        return true;
    }
    return false;
}



/**********************************************************************
 *  For internal temperature sensor. Only TLSR8258
 */

#if defined(MCU_CORE_8258)

void adc_temp_init() {

    drv_adc_init();
    adc_set_resolution(ADC_MISC_CHN, RES14);
    adc_set_vref_chn_misc(ADC_VREF_1P2V);
    adc_set_ain_channel_differential_mode(ADC_MISC_CHN, TEMSENSORP, TEMSENSORN);
    adc_set_vref_vbat_divider(ADC_VBAT_DIVIDER_OFF);//set Vbat divider select,
    adc_set_ain_pre_scaler(ADC_PRESCALER_1F8);

    drv_adc_enable(ON);

    //enable temperature sensor
//    analog_write(0x00, (analog_read(0x00)&0xef));
    analog_write(0x07, analog_read(0x07)&0xEF);
}


s16 adc_temp_result() {
    s16 adc_temp_value;
    u32 adc_data;

    adc_data = drv_get_adc_data();

    if (adc_data > 255) adc_data = 255;

    //printf("adc_data: %d\r\n", adc_data);

    adc_temp_value = ((adc_data*100 - 130*100)/54 - 40); // * 100 ;
//    adc_temp_value |= (adc_data*100 - 130*100)%54;

    return adc_temp_value;
}

#endif
