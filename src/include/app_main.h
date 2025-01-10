#ifndef SRC_INCLUDE_APP_MAIN_H_
#define SRC_INCLUDE_APP_MAIN_H_

#include "tl_common.h"
#include "zcl_include.h"
#include "bdb.h"
#include "ota.h"
#include "gp.h"

#include "se_custom_attr.h"
#include "app_reporting.h"
#include "app_uart.h"
#include "app_endpoint_cfg.h"
#include "app_button.h"
#include "app_led.h"
#include "app_dev_config.h"
#include "app_temperature.h"
#include "app_utility.h"
#include "app_tamper.h"

typedef struct{
    uint8_t keyType; /* CERTIFICATION_KEY or MASTER_KEY key for touch-link or distribute network
                   SS_UNIQUE_LINK_KEY or SS_GLOBAL_LINK_KEY for distribute network */
    uint8_t key[16]; /* the key used */
}app_linkKey_info_t;

typedef struct{
    ev_timer_event_t *timerLedEvt;
    ev_timer_event_t *timerLedStatusEvt;
//    ev_timer_event_t *timerForcedReportEvt;
    ev_timer_event_t *timerStopReportEvt;
    ev_timer_event_t *timerMeasurementEvt;
    ev_timer_event_t *timerNoJoinedEvt;

    button_t button;

    uint16_t ledOnTime;
    uint16_t ledOffTime;
    uint8_t  oriSta;     //original state before blink
    uint8_t  sta;        //current state in blink
    uint8_t  times;      //blink times
    uint8_t  state;

    bool bdbFindBindFlg;
//    bool lightAttrsChanged;

    app_linkKey_info_t tcLinkKey;
}app_ctx_t;


extern app_ctx_t g_appCtx;
extern bdb_commissionSetting_t g_bdbCommissionSetting;
extern bdb_appCb_t g_zbBdbCb;

extern uint8_t device_online;
extern uint8_t resp_time;

int32_t getTimeCb(void *arg);

#endif /* SRC_INCLUDE_APP_MAIN_H_ */
