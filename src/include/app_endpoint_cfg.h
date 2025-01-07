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
    uint8_t  zclVersion;
    uint8_t  appVersion;
    uint8_t  stackVersion;
    uint8_t  hwVersion;
    uint8_t  manuName[ZCL_BASIC_MAX_LENGTH];
    uint8_t  modelId[ZCL_BASIC_MAX_LENGTH];
    uint8_t  dateCode[ZCL_BASIC_MAX_LENGTH];
    uint8_t  powerSource;
    uint8_t  genDevClass;                        //attr 8
    uint8_t  genDevType;                         //attr 9
    uint8_t  deviceEnable;
    uint8_t  swBuildId[ZCL_BASIC_MAX_LENGTH];    //attr 4000
} zcl_basicAttr_t;


/**
 *  @brief Defined for identify cluster attributes
 */
typedef struct{
    uint16_t identifyTime;
}zcl_identifyAttr_t;

typedef struct {
    uint32_t time_utc;
} zcl_timeAttr_t;

/*
 *     zcl_seAttr_t.status
 *
 *     bit7 - Reserved
 *     bit6 - Service Disconnect Open
 *     bit5 - Leak Detect
 *     bit4 - Power Quality
 *     bit3 - Power Failure
 *     bit2 - Tamper Detect
 *     bit1 - Low Battery
 *     bit0 - Check Meter
 *
 */

typedef struct {
    uint64_t tariff_1;
    uint64_t tariff_2;
    uint64_t tariff_3;
    uint64_t tariff_4;
    uint64_t cur_sum_delivered;     // tariff_1+tariff_2+tariff_3+tariff_4
    uint8_t  unit_of_measure;       // 0x00 - kWh
    uint32_t multiplier;
    uint32_t divisor;
    uint8_t  status;
    uint8_t  summation_formatting;  // Bits 0 to 2: Number of Digits to the right of the Decimal Point
                                    // Bits 3 to 6: Number of Digits to the left of the Decimal Point
                                    // Bit  7:      If set, suppress leading zeros
    uint8_t  battery_percentage;
    uint8_t  serial_number[1+24];
    uint8_t  date_release[1+DATA_MAX_LEN];
    uint8_t  device_type;
    uint8_t  device_model;
    uint32_t device_address;
    uint8_t  device_name[1+DEVICE_NAME_LEN];
    uint8_t  device_password[9];    // [0] - size [1]...[8] - Password
    uint8_t  measurement_period;
} zcl_seAttr_t;

typedef struct {
    uint32_t type;
    uint16_t current;
    uint16_t current_multiplier;
    uint16_t current_divisor;
    uint16_t voltage;
    uint16_t voltage_multiplier;
    uint16_t voltage_divisor;
    uint16_t power;
    uint16_t power_multiplier;
    uint16_t power_divisor;
} zcl_msAttr_t;

typedef struct {
    int16_t temperature;
    uint8_t  alarm_mask;        /* bit0 = 0 (low alarm is disabled), bit1 = 1 (high alarm is enabled) */
    int16_t high_threshold;     /* 70 in degrees Celsius                                              */
} zcl_tempAttr_t;

extern uint8_t APP_CB_CLUSTER_NUM;
extern const zcl_specClusterInfo_t g_appClusterList[];
extern const af_simple_descriptor_t app_simpleDesc;

/* Attributes */
extern zcl_basicAttr_t      g_zcl_basicAttrs;
extern zcl_identifyAttr_t   g_zcl_identifyAttrs;
extern zcl_seAttr_t         g_zcl_seAttrs;
extern zcl_msAttr_t         g_zcl_msAttrs;

#define zcl_iasZoneAttrGet()    &g_zcl_iasZoneAttrs
#define zcl_pollCtrlAttrGet()   &g_zcl_pollCtrlAttrs
#define zcl_seAttrs()           &g_zcl_seAttrs
#define zcl_msAttrs()           &g_zcl_msAttrs


void app_zclProcessIncomingMsg(zclIncoming_t *pInHdlrMsg);

status_t app_basicCb(zclIncomingAddrInfo_t *pAddrInfo, uint8_t cmdId, void *cmdPayload);
status_t app_identifyCb(zclIncomingAddrInfo_t *pAddrInfo, uint8_t cmdId, void *cmdPayload);
status_t app_sceneCb(zclIncomingAddrInfo_t *pAddrInfo, uint8_t cmdId, void *cmdPayload);
status_t app_timeCb(zclIncomingAddrInfo_t *pAddrInfo, uint8_t cmdId, void *cmdPayload);
status_t app_meteringCb(zclIncomingAddrInfo_t *pAddrInfo, uint8_t cmdId, void *cmdPayload);

void app_leaveCnfHandler(nlme_leave_cnf_t *pLeaveCnf);
void app_leaveIndHandler(nlme_leave_ind_t *pLeaveInd);
void app_otaProcessMsgHandler(uint8_t evt, uint8_t status);
bool app_nwkUpdateIndicateHandler(nwkCmd_nwkUpdate_t *pNwkUpdate);



#endif /* SRC_INCLUDE_APP_ENDPOINT_CFG_H_ */
