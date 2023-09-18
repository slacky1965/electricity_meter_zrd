#ifndef SRC_INCLUDE_APP_MAIN_H_
#define SRC_INCLUDE_APP_MAIN_H_

#include "app_endpoint_cfg.h"
#include "app_button.h"
#include "app_led.h"
#include "app_dev_config.h"
#include "app_temperature.h"
#include "app_utility.h"

typedef struct{
    u8 keyType; /* CERTIFICATION_KEY or MASTER_KEY key for touch-link or distribute network
                   SS_UNIQUE_LINK_KEY or SS_GLOBAL_LINK_KEY for distribute network */
    u8 key[16]; /* the key used */
}app_linkKey_info_t;

typedef struct{
    ev_timer_event_t *timerLedEvt;
    ev_timer_event_t *timerLedStatusEvt;
//    ev_timer_event_t *timerForcedReportEvt;
    ev_timer_event_t *timerStopReportEvt;
    ev_timer_event_t *timerMeasurementEvt;
    ev_timer_event_t *timerNoJoinedEvt;

    button_t button;

    u16 ledOnTime;
    u16 ledOffTime;
    u8  oriSta;     //original state before blink
    u8  sta;        //current state in blink
    u8  times;      //blink times
    u8  state;

    bool bdbFindBindFlg;
//    bool lightAttrsChanged;

    app_linkKey_info_t tcLinkKey;
}app_ctx_t;


extern app_ctx_t g_appCtx;
extern bdb_commissionSetting_t g_bdbCommissionSetting;
extern bdb_appCb_t g_zbBdbCb;

extern u8 device_online;
extern u8 resp_time;

s32 getTimeCb(void *arg);

#endif /* SRC_INCLUDE_APP_MAIN_H_ */
