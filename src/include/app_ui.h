/********************************************************************************************************
 * @file    app_ui.h
 *
 * @brief   This is the header file for app_ui
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

#ifndef _APP_UI_H_
#define _APP_UI_H_

/**********************************************************************
 * CONSTANT
 */
/* for clock_time_exceed() */
#define TIMEOUT_TICK_1SEC   1000*1000       /* timeout 1 sec    */
#define TIMEOUT_TICK_5SEC   5*1000*1000     /* timeout 5 sec    */
#define TIMEOUT_TICK_10SEC  10*1000*1000    /* timeout 10 sec   */
#define TIMEOUT_TICK_15SEC  15*1000*1000    /* timeout 15 sec   */
#define TIMEOUT_TICK_30SEC  30*1000*1000    /* timeout 30 sec   */
//#define TIMEOUT_TICK_5MIN   300*1000*1000   /* timeout 5  min   */
//#define TIMEOUT_TICK_30MIN  1800*1000*1000  /* timeout 30 min   */

/* for TL_ZB_TIMER_SCHEDULE() */
#define TIMEOUT_1SEC        1    * 1000     /* timeout 1 sec    */
#define TIMEOUT_2SEC        2    * 1000     /* timeout 2 sec    */
#define TIMEOUT_3SEC        3    * 1000     /* timeout 3 sec    */
#define TIMEOUT_4SEC        4    * 1000     /* timeout 4 sec    */
#define TIMEOUT_5SEC        5    * 1000     /* timeout 5 sec    */
#define TIMEOUT_10SEC       10   * 1000     /* timeout 10 sec   */
#define TIMEOUT_15SEC       15   * 1000     /* timeout 15 sec   */
#define TIMEOUT_30SEC       30   * 1000     /* timeout 30 sec   */
#define TIMEOUT_1MIN        60   * 1000     /* timeout 1 min    */
#define TIMEOUT_2MIN        120  * 1000     /* timeout 2 min    */
#define TIMEOUT_5MIN        300  * 1000     /* timeout 5 min    */
#define TIMEOUT_10MIN       600  * 1000     /* timeout 10 min   */
#define TIMEOUT_15MIN       900  * 1000     /* timeout 15 min   */
#define TIMEOUT_30MIN       1800 * 1000     /* timeout 30 min   */

#define LED_ON                      1
#define LED_OFF                     0



/**********************************************************************
 * TYPEDEFS
 */
enum{
	APP_STATE_NORMAL,
	APP_FACTORY_NEW_SET_CHECK,
	APP_FACTORY_NEW_DOING,
};

typedef struct {
    u8  released :1;
    u8  pressed  :1;
    u8  counter  :6;
    u8  debounce;
    u32 pressed_time;
    u32 released_time;
} button_t;

/**********************************************************************
 * GLOBAL VARIABLES
 */

extern u8 device_online;
extern u8 resp_time;

/**********************************************************************
 * FUNCTIONS
 */
void led_init(void);
void led_on(u32 pin);
void led_off(u32 pin);
void localPermitJoinState(void);
void app_key_handler(void);

void light_blink_start(u8 times, u16 ledOnTime, u16 ledOffTime);
void light_blink_stop(void);

void light_init(void);
void light_on(void);
void light_off(void);
s32 flashLedStatusCb(void *arg);

void init_button();
void button_handler();
u8 button_idle();

s32 getTimeCb(void *arg);
s32 getTemperatureCb(void *arg);

#if defined(MCU_CORE_8258)
void adc_temp_init();
s16 adc_temp_result();
#endif

#endif	/* _APP_UI_H_ */
