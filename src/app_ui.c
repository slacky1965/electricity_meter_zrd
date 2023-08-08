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
#include "electricityMeter.h"

/**********************************************************************
 * LOCAL CONSTANTS
 */

#define ID_CONFIG   0x0FED141A
#define TOP_MASK    0xFFFFFFFF

#define DEBOUNCE_BUTTON     16                          /* number of polls for debounce       */

#define COUNT_FACTORY_RESET 5                           /* number of clicks for factory reset */

/**********************************************************************
 * TYPEDEFS
 */


/**********************************************************************
 * LOCAL VARIABLES
 */

static u8  default_config = false;
static u32 config_addr_start = 0;
static u32 config_addr_end = 0;

/**********************************************************************
 * GLOBAL VARIABLES
 */

u8 mcuBootAddrGet(void);

em_config_t em_config;
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

s32 measure_meterCb(void *arg) {

    s32 period = DEFAULT_MEASUREMENT_PERIOD * 1000;

    if (em_config.device_model && measure_meter) {
        if (measure_meter()) {
            period = em_config.measurement_period * 1000;
        }
    }

    return period;
}

static s32 delayedMcuResetCb(void *arg) {

    //printf("mcu reset\r\n");
    zb_resetDevice();
    return -1;
}

static s32 delayedFactoryResetCb(void *arg) {

    //printf("factory reset\r\n");
    zb_factoryReset();
    zb_resetDevice();
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
 *  For config electricity meter
 */

static u16 checksum(const u8 *src_buffer, u8 len) {

    const u16 generator = 0xa010;

    u16 crc = 0xffff;

    len -= 2;

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

static void get_user_data_addr(u8 print) {
#ifdef ZCL_OTA
    if (mcuBootAddrGet()) {
        config_addr_start = BEGIN_USER_DATA1;
        config_addr_end = END_USER_DATA1;
#if UART_PRINTF_MODE
        if (print) printf("OTA mode enabled. MCU boot from address: 0x%x\r\n", BEGIN_USER_DATA2);
#endif /* UART_PRINTF_MODE */
    } else {
        config_addr_start = BEGIN_USER_DATA2;
        config_addr_end = END_USER_DATA2;
#if UART_PRINTF_MODE
        if (print) printf("OTA mode enabled. MCU boot from address: 0x%x\r\n", BEGIN_USER_DATA1);
#endif /* UART_PRINTF_MODE */
    }
#else
    config_addr_start = BEGIN_USER_DATA2;
    config_addr_end = END_USER_DATA2;

#if UART_PRINTF_MODE
    if (print) printf("OTA mode desabled. MCU boot from address: 0x%x\r\n", BEGIN_USER_DATA1);
#endif /* UART_PRINTF_MODE */

#endif
}

static void clear_user_data(u32 flash_addr) {

    u32 flash_data_size = flash_addr + USER_DATA_SIZE;

    while(flash_addr < flash_data_size) {
        flash_erase_sector(flash_addr);
        flash_addr += FLASH_SECTOR_SIZE;
    }
}

static void init_default_config() {
    memset(&em_config, 0, sizeof(em_config_t));
    em_config.id = ID_CONFIG;
    em_config.top = 0;
    em_config.new_ota = 0;
    em_config.flash_addr_start = config_addr_start;
    em_config.flash_addr_end = config_addr_end;
    em_config.measurement_period = DEFAULT_MEASUREMENT_PERIOD;
    default_config = true;
    write_config();
}

static void write_restore_config() {
    em_config.crc = checksum((u8*)&(em_config), sizeof(em_config_t));
    nv_flashWriteNew(1, NV_MODULE_APP,  NV_ITEM_APP_USER_CFG, sizeof(em_config_t), (u8*)&em_config);

#if UART_PRINTF_MODE && DEBUG_LEVEL
    printf("Save restored config to nv_ram in module NV_MODULE_APP (%d) item NV_ITEM_APP_USER_CFG (%d)\r\n",
            NV_MODULE_APP,  NV_ITEM_APP_USER_CFG);
#endif /* UART_PRINTF_MODE */

}

void init_config(u8 print) {
    em_config_t config_curr, config_next, config_restore;
    u8 find_config = false;
    nv_sts_t st = NV_SUCC;

    get_user_data_addr(print);

#if !NV_ENABLE
#error "NV_ENABLE must be enable in "stack_cfg.h" file!"
#endif

    st = nv_flashReadNew(1, NV_MODULE_APP,  NV_ITEM_APP_USER_CFG, sizeof(em_config_t), (u8*)&config_restore);

    u16 crc = checksum((u8*)&config_restore, sizeof(em_config_t));

    if (st != NV_SUCC || config_restore.id != ID_CONFIG || crc != config_restore.crc) {
#if UART_PRINTF_MODE
        printf("No saved config! Init.\r\n");
#endif /* UART_PRINTF_MODE */
        clear_user_data(config_addr_start);
        init_default_config();
        return;

    }

    if (config_restore.new_ota) {
        config_restore.new_ota = false;
        config_restore.flash_addr_start = config_addr_start;
        config_restore.flash_addr_end = config_addr_end;
        memcpy(&em_config, &config_restore, sizeof(em_config_t));
        default_config = true;
        write_config();
        return;
    }

    u32 flash_addr = config_addr_start;

    flash_read_page(flash_addr, sizeof(em_config_t), (u8*)&config_curr);

    if (config_curr.id != ID_CONFIG) {
#if UART_PRINTF_MODE
        printf("No saved config! Init.\r\n");
#endif /* UART_PRINTF_MODE */
        clear_user_data(config_addr_start);
        init_default_config();
        return;
    }

    flash_addr += FLASH_PAGE_SIZE;

    while(flash_addr < config_addr_end) {
        flash_read_page(flash_addr, sizeof(em_config_t), (u8*)&config_next);
        crc = checksum((u8*)&config_next, sizeof(em_config_t));
        if (config_next.id == ID_CONFIG && crc == config_next.crc) {
            if ((config_curr.top + 1) == config_next.top || (config_curr.top == TOP_MASK && config_next.top == 0)) {
                memcpy(&config_curr, &config_next, sizeof(em_config_t));
                flash_addr += FLASH_PAGE_SIZE;
                continue;
            }
            find_config = true;
            break;
        }
        find_config = true;
        break;
    }

    if (find_config) {
        memcpy(&em_config, &config_curr, sizeof(em_config_t));
        em_config.flash_addr_start = flash_addr-FLASH_PAGE_SIZE;
#if UART_PRINTF_MODE
        printf("Read config from flash address - 0x%x\r\n", em_config.flash_addr_start);
#endif /* UART_PRINTF_MODE */
    } else {
#if UART_PRINTF_MODE
        printf("No active saved config! Reinit.\r\n");
#endif /* UART_PRINTF_MODE */
        clear_user_data(config_addr_start);
        init_default_config();
    }
}

void write_config() {
    if (default_config) {
        write_restore_config();
        flash_erase(em_config.flash_addr_start);
        flash_write(em_config.flash_addr_start, sizeof(em_config_t), (u8*)&(em_config));
        default_config = false;
#if UART_PRINTF_MODE && DEBUG_LEVEL
        printf("Save config to flash address - 0x%x\r\n", em_config.flash_addr_start);
#endif /* UART_PRINTF_MODE */
    } else {
        if (!em_config.new_ota) {
            em_config.flash_addr_start += FLASH_PAGE_SIZE;
            if (em_config.flash_addr_start == config_addr_end) {
                em_config.flash_addr_start = config_addr_start;
            }
            if (em_config.flash_addr_start % FLASH_SECTOR_SIZE == 0) {
                flash_erase(em_config.flash_addr_start);
            }
            em_config.top++;
            em_config.top &= TOP_MASK;
            em_config.crc = checksum((u8*)&(em_config), sizeof(em_config_t));
            flash_write(em_config.flash_addr_start, sizeof(em_config_t), (u8*)&(em_config));
#if UART_PRINTF_MODE && DEBUG_LEVEL
            printf("Save config to flash address - 0x%x\r\n", em_config.flash_addr_start);
#endif /* UART_PRINTF_MODE */
        } else {
            write_restore_config();
        }
    }

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
