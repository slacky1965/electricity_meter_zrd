#ifndef SRC_INCLUDE_APP_ENDPOINT_CFG_H_
#define SRC_INCLUDE_APP_ENDPOINT_CFG_H_

#include "device.h"

#define APP_ENDPOINT_1 0x01
#define APP_ENDPOINT_2 0x02
#define APP_ENDPOINT_3 0x03

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
    u8  serial_number[1+24];
    u8  date_release[1+DATA_MAX_LEN];
    u8  device_type;
    u8  device_model;
    u32 device_address;
    u8  measurement_period;
    u32 current;
    u32 current_multiplier;
    u32 current_divisor;
    u32 voltage;
    u32 voltage_multiplier;
    u32 voltage_divisor;
    u32 power;
    u32 power_multiplier;
    u32 power_divisor;
} zcl_seAttr_t;

typedef struct {
    u32 type;
    u16 current;
    u16 current_multiplier;
    u16 current_divisor;
    u16 voltage;
    u16 voltage_multiplier;
    u16 voltage_divisor;
    u16 power;
    u16 power_multiplier;
    u16 power_divisor;
} zcl_msAttr_t;

typedef struct {
    s16 temperature;
    u8  alarm_mask;         /* bit0 = 0 (low alarm is disabled), bit1 = 1 (high alarm is enabled) */
    s16 high_threshold;     /* 70 in degrees Celsius                                              */
} zcl_tempAttr_t;

extern u8 APP_CB_CLUSTER_NUM;
extern const zcl_specClusterInfo_t g_appClusterList[];
extern const af_simple_descriptor_t app_simpleDesc;

/* Attributes */
extern zcl_basicAttr_t g_zcl_basicAttrs;
extern zcl_identifyAttr_t g_zcl_identifyAttrs;

void app_zclProcessIncomingMsg(zclIncoming_t *pInHdlrMsg);

status_t app_basicCb(zclIncomingAddrInfo_t *pAddrInfo, u8 cmdId, void *cmdPayload);
status_t app_identifyCb(zclIncomingAddrInfo_t *pAddrInfo, u8 cmdId, void *cmdPayload);
status_t app_sceneCb(zclIncomingAddrInfo_t *pAddrInfo, u8 cmdId, void *cmdPayload);
status_t app_timeCb(zclIncomingAddrInfo_t *pAddrInfo, u8 cmdId, void *cmdPayload);
status_t app_meteringCb(zclIncomingAddrInfo_t *pAddrInfo, u8 cmdId, void *cmdPayload);

void app_leaveCnfHandler(nlme_leave_cnf_t *pLeaveCnf);
void app_leaveIndHandler(nlme_leave_ind_t *pLeaveInd);
void app_otaProcessMsgHandler(u8 evt, u8 status);
bool app_nwkUpdateIndicateHandler(nwkCmd_nwkUpdate_t *pNwkUpdate);



#endif /* SRC_INCLUDE_APP_ENDPOINT_CFG_H_ */
