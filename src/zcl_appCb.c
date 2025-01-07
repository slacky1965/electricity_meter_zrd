/********************************************************************************************************
 * @file    zcl_appCb.c
 *
 * @brief   This is the source file for zcl_appCb
 *
 * @author  Zigbee Group
 * @date    2021
 *
 * @par     Copyright (c) 2021, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *          All rights reserved.
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
#include "ota.h"

#include "app_main.h"
#include "app_endpoint_cfg.h"
#include "app_dev_config.h"
#include "app_reporting.h"
#include "se_custom_attr.h"

/**********************************************************************
 * LOCAL CONSTANTS
 */



/**********************************************************************
 * TYPEDEFS
 */


/**********************************************************************
 * LOCAL FUNCTIONS
 */
#ifdef ZCL_READ
static void app_zclReadRspCmd(zclReadRspCmd_t *pReadRspCmd);
#endif
#ifdef ZCL_WRITE
static void app_zclWriteReqCmd(uint16_t clusterId, zclWriteCmd_t *pWriteReqCmd);
static void app_zclWriteRspCmd(zclWriteRspCmd_t *pWriteRspCmd);
#endif
#ifdef ZCL_REPORT
static void app_zclCfgReportCmd(uint8_t endPoint, uint16_t clusterId, zclCfgReportCmd_t *pCfgReportCmd);
static void app_zclCfgReportRspCmd(zclCfgReportRspCmd_t *pCfgReportRspCmd);
static void app_zclReportCmd(zclReportCmd_t *pReportCmd);
#endif
static void app_zclDfltRspCmd(zclDefaultRspCmd_t *pDftRspCmd);


/**********************************************************************
 * GLOBAL VARIABLES
 */


/**********************************************************************
 * LOCAL VARIABLES
 */
#ifdef ZCL_IDENTIFY
static ev_timer_event_t *identifyTimerEvt = NULL;
#endif

uint8_t count_no_service = 0;


/**********************************************************************
 * FUNCTIONS
 */

/*********************************************************************
 * @fn      app_zclProcessIncomingMsg
 *
 * @brief   Process ZCL Foundation incoming message.
 *
 * @param   pInMsg - pointer to the received message
 *
 * @return  None
 */
void app_zclProcessIncomingMsg(zclIncoming_t *pInHdlrMsg)
{
//  printf("app_zclProcessIncomingMsg\n");

    uint16_t cluster = pInHdlrMsg->msg->indInfo.cluster_id;
    uint8_t endPoint = pInHdlrMsg->msg->indInfo.dst_ep;

    switch(pInHdlrMsg->hdr.cmd)
    {
#ifdef ZCL_READ
        case ZCL_CMD_READ_RSP:
            app_zclReadRspCmd(pInHdlrMsg->attrCmd);
            break;
#endif
#ifdef ZCL_WRITE
        case ZCL_CMD_WRITE:
            app_zclWriteReqCmd(pInHdlrMsg->msg->indInfo.cluster_id, pInHdlrMsg->attrCmd);
            break;
        case ZCL_CMD_WRITE_RSP:
            app_zclWriteRspCmd(pInHdlrMsg->attrCmd);
            break;
#endif
#ifdef ZCL_REPORT
        case ZCL_CMD_CONFIG_REPORT:
            app_zclCfgReportCmd(endPoint, cluster, pInHdlrMsg->attrCmd);
            break;
        case ZCL_CMD_CONFIG_REPORT_RSP:
            app_zclCfgReportRspCmd(pInHdlrMsg->attrCmd);
            break;
        case ZCL_CMD_REPORT:
            app_zclReportCmd(pInHdlrMsg->attrCmd);
            break;
#endif
        case ZCL_CMD_DEFAULT_RSP:
            app_zclDfltRspCmd(pInHdlrMsg->attrCmd);
            break;
        default:
            break;
    }
}

#ifdef ZCL_READ
/*********************************************************************
 * @fn      app_zclReadRspCmd
 *
 * @brief   Handler for ZCL Read Response command.
 *
 * @param   pInHdlrMsg - incoming message to process
 *
 * @return  None
 */
static void app_zclReadRspCmd(zclReadRspCmd_t *pReadRspCmd)
{
//    printf("app_zclReadRspCmd\n");

    uint8_t numAttr = pReadRspCmd->numAttr;
    zclReadRspStatus_t *attrList = pReadRspCmd->attrList;

    for (uint8_t i = 0; i < numAttr; i++) {
        if (attrList[i].attrID == ZCL_ATTRID_TIME && attrList[i].status == ZCL_STA_SUCCESS) {
            resp_time = true;
        }
    }

}
#endif

#ifdef ZCL_WRITE
/*********************************************************************
 * @fn      app_zclWriteReqCmd
 *
 * @brief   Handler for ZCL Write Request command.
 *
 * @param
 *
 * @return  None
 */
static void app_zclWriteReqCmd(uint16_t clusterId, zclWriteCmd_t *pWriteReqCmd)
{

//    printf("app_zclWriteReqCmd()\r\n");



    uint8_t numAttr = pWriteReqCmd->numAttr;
    zclWriteRec_t *attr = pWriteReqCmd->attrList;

    if (clusterId == ZCL_CLUSTER_SE_METERING) {
        for (uint8_t i = 0; i < numAttr; i++) {
            if (attr[i].attrID == ZCL_ATTRID_CUSTOM_DEVICE_ADDRESS && attr[i].dataType == ZCL_DATA_TYPE_UINT32) {
                uint32_t device_address = BUILD_U32(attr[i].attrData[0], attr[i].attrData[1], attr[i].attrData[2], attr[i].attrData[3]);
                if (dev_config.device_address != device_address) {
                    dev_config.device_address = device_address;
                    write_config();
#if UART_PRINTF_MODE // && DEBUG_LEVEL
                    printf("New device address: %d\r\n", dev_config.device_address);
#endif
                    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CUSTOM_DEVICE_ADDRESS, (uint8_t*)&device_address);
                }
            } else if (attr[i].attrID == ZCL_ATTRID_CUSTOM_DEVICE_MANUFACTURER && attr[i].attrData) {
                device_model_t model = *attr[i].attrData;
                set_device_model(model);
                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CUSTOM_DEVICE_MANUFACTURER, (uint8_t*)&model);
            } else if (attr[i].attrID == ZCL_ATTRID_CUSTOM_DEVICE_PASSWORD && attr[i].dataType == ZCL_DATA_TYPE_OCTET_STR) {
                dev_config.device_password.size = attr[i].attrData[0];
                memset(dev_config.device_password.data, 0, sizeof(dev_config.device_password.data));
                memcpy(dev_config.device_password.data, attr[i].attrData+1, attr[i].attrData[0]);
                switch (dev_config.device_model) {
                    case DEVICE_NARTIS_100:
                        nartis100_init();
                        break;
                    case DEVICE_KASKAD_1_MT:
                        if (dev_config.device_password.size > 8) {
                            dev_config.device_password.size = 8;
                        }
                        break;
                    default:
                        break;
                }
                write_config();
#if UART_PRINTF_MODE // && DEBUG_LEVEL
                printf("New device password: %s\r\n", print_str_zcl((uint8_t*)&dev_config.device_password));
#endif
                zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CUSTOM_DEVICE_PASSWORD, (uint8_t*)&dev_config.device_password);
            } else if (attr[i].attrID == ZCL_ATTRID_CUSTOM_MEASUREMENT_PERIOD && attr[i].dataType == ZCL_DATA_TYPE_UINT8) {
                uint8_t period_in_min = *attr[i].attrData;
                if (period_in_min == 0) period_in_min = 1;
                uint16_t period_in_sec = period_in_min * 60;
                if (dev_config.measurement_period != period_in_sec) {
                    dev_config.measurement_period = period_in_sec;
                    write_config();
#if UART_PRINTF_MODE // && DEBUG_LEVEL
                    printf("New measurement period: %d sec\r\n", dev_config.measurement_period);
#endif
                    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CUSTOM_MEASUREMENT_PERIOD, (uint8_t*)&period_in_min);
                    if (g_appCtx.timerMeasurementEvt) {
                        TL_ZB_TIMER_CANCEL(&g_appCtx.timerMeasurementEvt);
                    }
                    g_appCtx.timerMeasurementEvt = TL_ZB_TIMER_SCHEDULE(measure_meterCb, NULL, dev_config.measurement_period * 1000);
                }
            }
        }
    }

//
//  if(clusterId == ZCL_CLUSTER_GEN_ON_OFF){
//      for(uint8_t i = 0; i < numAttr; i++){
//          if(attr[i].attrID == ZCL_ATTRID_START_UP_ONOFF){
//              zcl_onOffAttr_save();
//          }
//      }
//  }
}

/*********************************************************************
 * @fn      app_zclWriteRspCmd
 *
 * @brief   Handler for ZCL Write Response command.
 *
 * @param   pInHdlrMsg - incoming message to process
 *
 * @return  None
 */
static void app_zclWriteRspCmd(zclWriteRspCmd_t *pWriteRspCmd)
{
//    printf("app_zclWriteRspCmd\n");

}
#endif


/*********************************************************************
 * @fn      app_zclDfltRspCmd
 *
 * @brief   Handler for ZCL Default Response command.
 *
 * @param   pInHdlrMsg - incoming message to process
 *
 * @return  None
 */
static void app_zclDfltRspCmd(zclDefaultRspCmd_t *pDftRspCmd)
{
//  printf("app_zclDfltRspCmd\n");
#ifdef ZCL_OTA
    if( (pDftRspCmd->commandID == ZCL_CMD_OTA_UPGRADE_END_REQ) &&
        (pDftRspCmd->statusCode == ZCL_STA_ABORT) ){
        if(zcl_attr_imageUpgradeStatus == IMAGE_UPGRADE_STATUS_DOWNLOAD_COMPLETE){
            ota_upgradeAbort();
        }
    }
#endif
}

#ifdef ZCL_REPORT
/*********************************************************************
 * @fn      app_zclCfgReportCmd
 *
 * @brief   Handler for ZCL Configure Report command.
 *
 * @param   pInHdlrMsg - incoming message to process
 *
 * @return  None
 */
static void app_zclCfgReportCmd(uint8_t endPoint, uint16_t clusterId, zclCfgReportCmd_t *pCfgReportCmd)
{
    //printf("app_zclCfgReportCmd\r\n");
    for(uint8_t i = 0; i < pCfgReportCmd->numAttr; i++) {
        for (uint8_t ii = 0; ii < ZCL_REPORTING_TABLE_NUM; ii++) {
            if (app_reporting[ii].pEntry->used) {
                if (app_reporting[ii].pEntry->endPoint == endPoint &&
                    app_reporting[ii].pEntry->clusterID == clusterId &&
                    app_reporting[ii].pEntry->attrID == pCfgReportCmd->attrList[i].attrID) {
                    if (pCfgReportCmd->attrList[i].attrID == ZCL_ATTRID_MULTIPLIER ||
                        pCfgReportCmd->attrList[i].attrID == ZCL_ATTRID_DIVISOR ||
                        pCfgReportCmd->attrList[i].attrID == ZCL_ATTRID_AC_VOLTAGE_MULTIPLIER ||
                        pCfgReportCmd->attrList[i].attrID == ZCL_ATTRID_AC_VOLTAGE_DIVISOR ||
                        pCfgReportCmd->attrList[i].attrID == ZCL_ATTRID_AC_CURRENT_MULTIPLIER ||
                        pCfgReportCmd->attrList[i].attrID == ZCL_ATTRID_AC_CURRENT_DIVISOR ||
                        pCfgReportCmd->attrList[i].attrID == ZCL_ATTRID_AC_POWER_MULTIPLIER ||
                        pCfgReportCmd->attrList[i].attrID == ZCL_ATTRID_AC_POWER_DIVISOR) {
#if UART_PRINTF_MODE && DEBUG_REPORTING
                        printf("The report in cluster 0x%x of attribute 0x%x cannot be changed\r\n",
                                clusterId, pCfgReportCmd->attrList[i].attrID);
#endif
                        app_reporting[ii].pEntry->maxInterval = 0;
                        app_reporting[ii].pEntry->minInterval = 0;
                        memset(app_reporting[ii].pEntry->reportableChange, 0, sizeof(app_reporting[ii].pEntry->reportableChange));
                    } else {
#if UART_PRINTF_MODE && DEBUG_REPORTING
                    printf("reportCfg: idx: %d, endPoint: 0x%x, clusterId: 0x%x, attrId: 0x%x\r\n",
                            ii, endPoint, clusterId, pCfgReportCmd->attrList[i].attrID);
                    printf("New minInterval: %d, new maxInterval: %d, new reportableChange: 0x",
                            app_reporting[ii].pEntry->minInterval, app_reporting[ii].pEntry->maxInterval);
                    for (uint8_t i = 0; i < REPORTABLE_CHANGE_MAX_ANALOG_SIZE; i++) {
                        if (app_reporting[ii].pEntry->reportableChange[i] < 0x10) {
                            printf("0%x", app_reporting[ii].pEntry->reportableChange[i]);
                        } else {
                            printf("%x", app_reporting[ii].pEntry->reportableChange[i]);
                        }
                    }
                    printf("\r\n");
#endif
                    }
                    if (app_reporting[ii].timerReportMinEvt) {
                        TL_ZB_TIMER_CANCEL(&app_reporting[ii].timerReportMinEvt);
                    }
                    if (app_reporting[ii].timerReportMaxEvt) {
                        TL_ZB_TIMER_CANCEL(&app_reporting[ii].timerReportMaxEvt);
                    }
                    return;
                }
            }
        }
    }
}

/*********************************************************************
 * @fn      app_zclCfgReportRspCmd
 *
 * @brief   Handler for ZCL Configure Report Response command.
 *
 * @param   pInHdlrMsg - incoming message to process
 *
 * @return  None
 */
static void app_zclCfgReportRspCmd(zclCfgReportRspCmd_t *pCfgReportRspCmd)
{
//    printf("app_zclCfgReportRspCmd\n");

}

/*********************************************************************
 * @fn      app_zclReportCmd
 *
 * @brief   Handler for ZCL Report command.
 *
 * @param   pInHdlrMsg - incoming message to process
 *
 * @return  None
 */
static void app_zclReportCmd(zclReportCmd_t *pReportCmd)
{
//    printf("app_zclReportCmd\n");

}
#endif

#ifdef ZCL_BASIC
/*********************************************************************
 * @fn      app_basicCb
 *
 * @brief   Handler for ZCL Basic Reset command.
 *
 * @param   pAddrInfo
 * @param   cmdId
 * @param   cmdPayload
 *
 * @return  status_t
 */
status_t app_basicCb(zclIncomingAddrInfo_t *pAddrInfo, uint8_t cmdId, void *cmdPayload)
{
    if(cmdId == ZCL_CMD_BASIC_RESET_FAC_DEFAULT){
        //Reset all the attributes of all its clusters to factory defaults
        //zcl_nv_attr_reset();
    }

    return ZCL_STA_SUCCESS;
}
#endif

#ifdef ZCL_IDENTIFY
int32_t app_zclIdentifyTimerCb(void *arg)
{
    if(g_zcl_identifyAttrs.identifyTime <= 0){
        light_blink_stop();

        identifyTimerEvt = NULL;
        return -1;
    }
    g_zcl_identifyAttrs.identifyTime--;
    return 0;
}

void app_zclIdentifyTimerStop(void)
{
    if(identifyTimerEvt){
        TL_ZB_TIMER_CANCEL(&identifyTimerEvt);
    }
}

/*********************************************************************
 * @fn      app_zclIdentifyCmdHandler
 *
 * @brief   Handler for ZCL Identify command. This function will set blink LED.
 *
 * @param   endpoint
 * @param   srcAddr
 * @param   identifyTime - identify time
 *
 * @return  None
 */
void app_zclIdentifyCmdHandler(uint8_t endpoint, uint16_t srcAddr, uint16_t identifyTime)
{
    g_zcl_identifyAttrs.identifyTime = identifyTime;

    if(identifyTime == 0){
        app_zclIdentifyTimerStop();
        light_blink_stop();
    }else{
        if(!identifyTimerEvt){
            light_blink_start(identifyTime, 500, 500);
            identifyTimerEvt = TL_ZB_TIMER_SCHEDULE(app_zclIdentifyTimerCb, NULL, 1000);
        }
    }
}

/*********************************************************************
 * @fn      app_zcltriggerCmdHandler
 *
 * @brief   Handler for ZCL trigger command.
 *
 * @param   pTriggerEffect
 *
 * @return  None
 */
static void app_zcltriggerCmdHandler(zcl_triggerEffect_t *pTriggerEffect)
{
    uint8_t effectId = pTriggerEffect->effectId;
//  uint8_t effectVariant = pTriggerEffect->effectVariant;

    switch(effectId){
        case IDENTIFY_EFFECT_BLINK:
            light_blink_start(1, 500, 500);
            break;
        case IDENTIFY_EFFECT_BREATHE:
            light_blink_start(15, 300, 700);
            break;
        case IDENTIFY_EFFECT_OKAY:
            light_blink_start(2, 250, 250);
            break;
        case IDENTIFY_EFFECT_CHANNEL_CHANGE:
            light_blink_start(1, 500, 7500);
            break;
        case IDENTIFY_EFFECT_FINISH_EFFECT:
            light_blink_start(1, 300, 700);
            break;
        case IDENTIFY_EFFECT_STOP_EFFECT:
            light_blink_stop();
            break;
        default:
            break;
    }
}

/*********************************************************************
 * @fn      app_identifyCb
 *
 * @brief   Handler for ZCL Identify command.
 *
 * @param   pAddrInfo
 * @param   cmdId
 * @param   cmdPayload
 *
 * @return  status_t
 */
status_t app_identifyCb(zclIncomingAddrInfo_t *pAddrInfo, uint8_t cmdId, void *cmdPayload)
{
    if(pAddrInfo->dstEp == APP_ENDPOINT_1){
        if(pAddrInfo->dirCluster == ZCL_FRAME_CLIENT_SERVER_DIR){
            switch(cmdId){
                case ZCL_CMD_IDENTIFY:
                    app_zclIdentifyCmdHandler(pAddrInfo->dstEp, pAddrInfo->srcAddr, ((zcl_identifyCmd_t *)cmdPayload)->identifyTime);
                    break;
                case ZCL_CMD_TRIGGER_EFFECT:
                    app_zcltriggerCmdHandler((zcl_triggerEffect_t *)cmdPayload);
                    break;
                default:
                    break;
            }
        }
    }

    return ZCL_STA_SUCCESS;
}
#endif

/*********************************************************************
 * @fn      app_timeCb
 *
 * @brief   Handler for ZCL Identify command.
 *
 * @param   pAddrInfo
 * @param   cmdId
 * @param   cmdPayload
 *
 * @return  status_t
 */
status_t app_timeCb(zclIncomingAddrInfo_t *pAddrInfo, uint8_t cmdId, void *cmdPayload) {

    //printf("app_timeCb. cmd: 0x%x\r\n", cmdId);

    return ZCL_STA_SUCCESS;
}


/*********************************************************************
 * @fn      app_meteringCb
 *
 * @brief   Handler for ZCL Identify command.
 *
 * @param   pAddrInfo
 * @param   cmdId
 * @param   cmdPayload
 *
 * @return  status_t
 */
status_t app_meteringCb(zclIncomingAddrInfo_t *pAddrInfo, uint8_t cmdId, void *cmdPayload)
{

    return ZCL_STA_SUCCESS;
}

static int32_t checkRespTimeCb(void *arg) {

    if (device_online) {
        if (!resp_time) {
            if (count_no_service++ == 3) {
                device_online = false;
#if UART_PRINTF_MODE// && DEBUG_LEVEL
                printf("No service!\r\n");
#endif
            }
        } else {
            count_no_service = 0;
        }
    } else {
        if (resp_time) {
            device_online = true;
            count_no_service = 0;
#if UART_PRINTF_MODE// && DEBUG_LEVEL
            printf("Device online\r\n");
#endif
        }
    }

    resp_time = false;

    return -1;
}


int32_t getTimeCb(void *arg) {

    if(zb_isDeviceJoinedNwk()){
        epInfo_t dstEpInfo;
        TL_SETSTRUCTCONTENT(dstEpInfo, 0);

        dstEpInfo.profileId = HA_PROFILE_ID;
#if FIND_AND_BIND_SUPPORT
        dstEpInfo.dstAddrMode = APS_DSTADDR_EP_NOTPRESETNT;
#else
        dstEpInfo.dstAddrMode = APS_SHORT_DSTADDR_WITHEP;
        dstEpInfo.dstEp = APP_ENDPOINT_1;
        dstEpInfo.dstAddr.shortAddr = 0x0;
#endif
        zclAttrInfo_t *pAttrEntry;
        pAttrEntry = zcl_findAttribute(APP_ENDPOINT_1, ZCL_CLUSTER_GEN_TIME, ZCL_ATTRID_TIME);

        zclReadCmd_t *pReadCmd = (zclReadCmd_t *)ev_buf_allocate(sizeof(zclReadCmd_t) + sizeof(uint16_t));
        if(pReadCmd){
            pReadCmd->numAttr = 1;
            pReadCmd->attrID[0] = ZCL_ATTRID_TIME;

            zcl_read(APP_ENDPOINT_1, &dstEpInfo, ZCL_CLUSTER_GEN_TIME, MANUFACTURER_CODE_NONE, 0, 0, 0, pReadCmd);

            ev_buf_free((uint8_t *)pReadCmd);

            TL_ZB_TIMER_SCHEDULE(checkRespTimeCb, NULL, TIMEOUT_2SEC);
        }
    }

    return 0;
}


