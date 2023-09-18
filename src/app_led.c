#include "tl_common.h"
#include "zb_api.h"
#include "zcl_include.h"
#include "gp.h"

#include "app_led.h"
#include "app_main.h"
#include "app_dev_config.h"

s32 flashLedStatusCb(void *arg) {
    if (zb_isDeviceJoinedNwk() && device_online) {
        light_blink_stop();
        if (dev_config.new_ota) {
            light_blink_start(2, 30, 250);
        } else {
            light_blink_start(1, 30, 30);
        }
    } else {
        light_blink_start(3, 30, 250);
    }

    return 0;
}

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

s32 zclLightTimerCb(void *arg)
{
    u32 interval = 0;

    if(g_appCtx.sta == g_appCtx.oriSta){
        g_appCtx.times--;
        if(g_appCtx.times <= 0){
            g_appCtx.timerLedEvt = NULL;
            return -1;
        }
    }

    g_appCtx.sta = !g_appCtx.sta;
    if(g_appCtx.sta){
        light_on();
        interval = g_appCtx.ledOnTime;
    }else{
        light_off();
        interval = g_appCtx.ledOffTime;
    }

    return interval;
}

void light_blink_start(u8 times, u16 ledOnTime, u16 ledOffTime)
{
    u32 interval = 0;
    g_appCtx.times = times;

    if(!g_appCtx.timerLedEvt){
        if(g_appCtx.oriSta){
            light_off();
            g_appCtx.sta = 0;
            interval = ledOffTime;
        }else{
            light_on();
            g_appCtx.sta = 1;
            interval = ledOnTime;
        }
        g_appCtx.ledOnTime = ledOnTime;
        g_appCtx.ledOffTime = ledOffTime;

        g_appCtx.timerLedEvt = TL_ZB_TIMER_SCHEDULE(zclLightTimerCb, NULL, interval);
    }
}

void light_blink_stop(void)
{
    if(g_appCtx.timerLedEvt){
        TL_ZB_TIMER_CANCEL(&g_appCtx.timerLedEvt);

        g_appCtx.times = 0;
        if(g_appCtx.oriSta){
            light_on();
        }else{
            light_off();
        }
    }
}

