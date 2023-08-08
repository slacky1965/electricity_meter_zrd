#ifndef SRC_INCLUDE_ELECTRICITYMETER_H_
#define SRC_INCLUDE_ELECTRICITYMETER_H_

#include "app_utility.h"
#include "device.h"

/**********************************************************************
 * CONSTANT
 */
#define ELECTRICITY_METER_EP1 0x01
#define ELECTRICITY_METER_EP2 0x02
#define ELECTRICITY_METER_EP3 0x03

/**********************************************************************
 * TYPEDEFS
 */
typedef struct{
    u8 keyType; /* CERTIFICATION_KEY or MASTER_KEY key for touch-link or distribute network
                   SS_UNIQUE_LINK_KEY or SS_GLOBAL_LINK_KEY for distribute network */
    u8 key[16]; /* the key used */
}app_linkKey_info_t;

typedef struct {
    ev_timer_event_t *timerReportMinEvt;
    ev_timer_event_t *timerReportMaxEvt;
    reportCfgInfo_t  *pEntry;
    u32               time_posted;
} app_reporting_t;

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

/**
 *  @brief Defined for basic cluster attributes
 */
typedef struct {
    u8  zclVersion;
    u8  appVersion;
    u8  stackVersion;
    u8  hwVersion;
    u8  manuName[ZCL_BASIC_MAX_LENGTH];
    u8  modelId[ZCL_BASIC_MAX_LENGTH];
    u8  dateCode[ZCL_BASIC_MAX_LENGTH];
    u8  powerSource;
    u8  genDevClass;                        //attr 8
    u8  genDevType;                         //attr 9
    u8  deviceEnable;
    u8  swBuildId[ZCL_BASIC_MAX_LENGTH];    //attr 4000
} zcl_basicAttr_t;


/**
 *  @brief Defined for identify cluster attributes
 */
typedef struct{
    u16 identifyTime;
}zcl_identifyAttr_t;

typedef struct {
    u32 time_utc;
} zcl_timeAttr_t;

typedef struct {
    u64 tariff_1;
    u64 tariff_2;
    u64 tariff_3;
    u64 tariff_4;
    u8  unit_of_measure;        // 0x00 - kWh
    u32 multiplier;
    u32 divisor;
    u8  summation_formatting;   // Bits 0 to 2: Number of Digits to the right of the Decimal Point
                                // Bits 3 to 6: Number of Digits to the left of the Decimal Point
                                // Bit  7:      If set, suppress leading zeros
    u8  battery_percentage;
    u8  serial_number[DATA_MAX_LEN+1];
    u8  device_type;
    u8  device_model;
    u32 device_address;
    u8  measurement_period;
} zcl_seAttr_t;

typedef struct {
    u32 type;
    u16 current;
    u16 voltage;
    u16 voltage_multiplier;
    u16 voltage_divisor;
    u16 current_multiplier;
    u16 current_divisor;
} zcl_msAttr_t;

typedef struct {
    s16 temperature;
    u8  alarm_mask;         /* bit0 = 0 (low alarm is disabled), bit1 = 1 (high alarm is enabled) */
    s16 high_threshold;     /* 70 in degrees Celsius                                              */
} zcl_tempAttr_t;


/**********************************************************************
 * GLOBAL VARIABLES
 */
extern app_reporting_t app_reporting[ZCL_REPORTING_TABLE_NUM];
extern app_ctx_t g_electricityMeterCtx;

extern bdb_commissionSetting_t g_bdbCommissionSetting;
extern bdb_appCb_t g_zbBdbCb;


extern u8 ELECTRICITY_METER_CB_CLUSTER_NUM;
extern const zcl_specClusterInfo_t g_electricityMeterClusterList[];
extern const af_simple_descriptor_t electricityMeter_simpleDesc;
#if AF_TEST_ENABLE
extern const af_simple_descriptor_t sampleTestDesc;
#endif

/* Attributes */
extern zcl_basicAttr_t g_zcl_basicAttrs;
extern zcl_identifyAttr_t g_zcl_identifyAttrs;
extern zcl_seAttr_t g_zcl_seAttrs;

/**********************************************************************
 * FUNCTIONS
 */
void electricityMeter_zclProcessIncomingMsg(zclIncoming_t *pInHdlrMsg);

status_t electricityMeter_basicCb(zclIncomingAddrInfo_t *pAddrInfo, u8 cmdId, void *cmdPayload);
status_t electricityMeter_identifyCb(zclIncomingAddrInfo_t *pAddrInfo, u8 cmdId, void *cmdPayload);
//status_t electricityMeter_sceneCb(zclIncomingAddrInfo_t *pAddrInfo, u8 cmdId, void *cmdPayload);
//status_t electricityMeter_onOffCb(zclIncomingAddrInfo_t *pAddrInfo, u8 cmdId, void *cmdPayload);
//status_t electricityMeter_levelCb(zclIncomingAddrInfo_t *pAddrInfo, u8 cmdId, void *cmdPayload);
//status_t electricityMeter_colorCtrlCb(zclIncomingAddrInfo_t *pAddrInfo, u8 cmdId, void *cmdPayload);
status_t electricityMeter_timeCb(zclIncomingAddrInfo_t *pAddrInfo, u8 cmdId, void *cmdPayload);
status_t electricityMeter_meteringCb(zclIncomingAddrInfo_t *pAddrInfo, u8 cmdId, void *cmdPayload);

void electricityMeter_leaveCnfHandler(nlme_leave_cnf_t *pLeaveCnf);
void electricityMeter_leaveIndHandler(nlme_leave_ind_t *pLeaveInd);
void electricityMeter_otaProcessMsgHandler(u8 evt, u8 status);
bool electricityMeter_nwkUpdateIndicateHandler(nwkCmd_nwkUpdate_t *pNwkUpdate);

void electricityMeter_onoff(u8 cmd);

void zcl_electricityMeterAttrsInit(void);
nv_sts_t zcl_onOffAttr_save(void);
nv_sts_t zcl_levelAttr_save(void);
nv_sts_t zcl_colorCtrlAttr_save(void);

#if AF_TEST_ENABLE
void afTest_rx_handler(void *arg);
void afTest_dataSendConfirm(void *arg);
#endif



#endif /* SRC_INCLUDE_ELECTRICITYMETER_H_ */
