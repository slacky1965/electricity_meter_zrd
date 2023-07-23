#ifndef SRC_INCLUDE_ELECTRICITYMETER_H_
#define SRC_INCLUDE_ELECTRICITYMETER_H_

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

typedef struct{
    ev_timer_event_t *timerLedEvt;
    ev_timer_event_t *timerLedStatusEvt;

    button_t button;

    u16 ledOnTime;
    u16 ledOffTime;
    u8  oriSta;     //original state before blink
    u8  sta;        //current state in blink
    u8  times;      //blink times
    u8  state;

    bool bdbFindBindFlg;
    bool lightAttrsChanged;

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

/**
 *  @brief Defined for power configuration cluster attributes
 */
typedef struct{
    u16 mainsVoltage;
    u8  mainsFrequency;
}zcl_powerAttr_t;

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
    u8  serial_number[ZCL_BASIC_MAX_LENGTH];
    u8  device_type;
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

///**
// *  @brief Defined for group cluster attributes
// */
//typedef struct{
//    u8  nameSupport;
//}zcl_groupAttr_t;
//
///**
// *  @brief Defined for scene cluster attributes
// */
//typedef struct{
//    u8   sceneCount;
//    u8   currentScene;
//    u8   nameSupport;
//    bool sceneValid;
//    u16  currentGroup;
//}zcl_sceneAttr_t;
//
///**
// *  @brief Defined for on/off cluster attributes
// */
//typedef struct{
//    u16  onTime;
//    u16  offWaitTime;
//    u8   startUpOnOff;
//    bool onOff;
//    bool globalSceneControl;
//}zcl_onOffAttr_t;
//
///**
// *  @brief Defined for level cluster attributes
// */
//typedef struct{
//    u16 remainingTime;
//    u8  curLevel;
//    u8  startUpCurrentLevel;
//}zcl_levelAttr_t;
//
///**
// *  @brief Defined for color control cluster attributes
// */
//typedef struct{
//    u8  colorMode;
//    u8  options;
//    u8  enhancedColorMode;
//    u8  numOfPrimaries;
//    u16 colorCapabilities;
//#if COLOR_RGB_SUPPORT
//    u8  currentHue;
//    u8  currentSaturation;
//    u8  colorLoopActive;
//    u8  colorLoopDirection;
//    u16 colorLoopTime;
//    u16 colorLoopStartEnhancedHue;
//    u16 colorLoopStoredEnhancedHue;
//#elif COLOR_CCT_SUPPORT
//    u16 colorTemperatureMireds;
//    u16 colorTempPhysicalMinMireds;
//    u16 colorTempPhysicalMaxMireds;
//    u16 startUpColorTemperatureMireds;
//#endif
//}zcl_lightColorCtrlAttr_t;
//
///**
// *  @brief Defined for saving on/off attributes
// */
//typedef struct {
//    u8  onOff;
//    u8  startUpOnOff;
//}zcl_nv_onOff_t;
//
///**
// *  @brief Defined for saving level attributes
// */
//typedef struct {
//    u8  curLevel;
//    u8  startUpCurLevel;
//}zcl_nv_level_t;
//
///**
// *  @brief Defined for saving color control attributes
// */
//typedef struct {
//#if COLOR_RGB_SUPPORT
//    u8  currentHue;
//    u8  currentSaturation;
//#elif COLOR_CCT_SUPPORT
//    u16 colorTemperatureMireds;
//    u16 startUpColorTemperatureMireds;
//#endif
//}zcl_nv_colorCtrl_t;

/**********************************************************************
 * GLOBAL VARIABLES
 */
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
//extern zcl_groupAttr_t g_zcl_groupAttrs;
//extern zcl_sceneAttr_t g_zcl_sceneAttrs;
//extern zcl_onOffAttr_t g_zcl_onOffAttrs;
//extern zcl_levelAttr_t g_zcl_levelAttrs;
//extern zcl_lightColorCtrlAttr_t g_zcl_colorCtrlAttrs;

#define zcl_sceneAttrGet()      &g_zcl_sceneAttrs
#define zcl_onoffAttrGet()      &g_zcl_onOffAttrs
#define zcl_levelAttrGet()      &g_zcl_levelAttrs
#define zcl_colorAttrGet()      &g_zcl_colorCtrlAttrs

/**********************************************************************
 * FUNCTIONS
 */
void electricityMeter_zclProcessIncomingMsg(zclIncoming_t *pInHdlrMsg);

status_t electricityMeter_basicCb(zclIncomingAddrInfo_t *pAddrInfo, u8 cmdId, void *cmdPayload);
status_t electricityMeter_identifyCb(zclIncomingAddrInfo_t *pAddrInfo, u8 cmdId, void *cmdPayload);
status_t electricityMeter_sceneCb(zclIncomingAddrInfo_t *pAddrInfo, u8 cmdId, void *cmdPayload);
status_t electricityMeter_onOffCb(zclIncomingAddrInfo_t *pAddrInfo, u8 cmdId, void *cmdPayload);
status_t electricityMeter_levelCb(zclIncomingAddrInfo_t *pAddrInfo, u8 cmdId, void *cmdPayload);
status_t electricityMeter_colorCtrlCb(zclIncomingAddrInfo_t *pAddrInfo, u8 cmdId, void *cmdPayload);
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
