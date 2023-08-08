/********************************************************************************************************
 * @file    zcl_electricityMeterCb.c
 *
 * @brief   This is the source file for zcl_electricityMeterCb
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
#include "ota.h"

#include "app_ui.h"
#include "electricityMeter.h"
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
static void electricityMeter_zclReadRspCmd(zclReadRspCmd_t *pReadRspCmd);
#endif
#ifdef ZCL_WRITE
static void electricityMeter_zclWriteReqCmd(u16 clusterId, zclWriteCmd_t *pWriteReqCmd);
static void electricityMeter_zclWriteRspCmd(zclWriteRspCmd_t *pWriteRspCmd);
#endif
#ifdef ZCL_REPORT
static void electricityMeter_zclCfgReportCmd(zclCfgReportCmd_t *pCfgReportCmd);
static void electricityMeter_zclCfgReportRspCmd(zclCfgReportRspCmd_t *pCfgReportRspCmd);
static void electricityMeter_zclReportCmd(zclReportCmd_t *pReportCmd);
#endif
static void electricityMeter_zclDfltRspCmd(zclDefaultRspCmd_t *pDftRspCmd);


/**********************************************************************
 * GLOBAL VARIABLES
 */


/**********************************************************************
 * LOCAL VARIABLES
 */
#ifdef ZCL_IDENTIFY
static ev_timer_event_t *identifyTimerEvt = NULL;
#endif


/**********************************************************************
 * FUNCTIONS
 */

/*********************************************************************
 * @fn      electricityMeter_zclProcessIncomingMsg
 *
 * @brief   Process ZCL Foundation incoming message.
 *
 * @param   pInMsg - pointer to the received message
 *
 * @return  None
 */
void electricityMeter_zclProcessIncomingMsg(zclIncoming_t *pInHdlrMsg)
{
//	printf("electricityMeter_zclProcessIncomingMsg\n");

	switch(pInHdlrMsg->hdr.cmd)
	{
#ifdef ZCL_READ
		case ZCL_CMD_READ_RSP:
			electricityMeter_zclReadRspCmd(pInHdlrMsg->attrCmd);
			break;
#endif
#ifdef ZCL_WRITE
		case ZCL_CMD_WRITE:
			electricityMeter_zclWriteReqCmd(pInHdlrMsg->msg->indInfo.cluster_id, pInHdlrMsg->attrCmd);
			break;
		case ZCL_CMD_WRITE_RSP:
			electricityMeter_zclWriteRspCmd(pInHdlrMsg->attrCmd);
			break;
#endif
#ifdef ZCL_REPORT
		case ZCL_CMD_CONFIG_REPORT:
			electricityMeter_zclCfgReportCmd(pInHdlrMsg->attrCmd);
			break;
		case ZCL_CMD_CONFIG_REPORT_RSP:
			electricityMeter_zclCfgReportRspCmd(pInHdlrMsg->attrCmd);
			break;
		case ZCL_CMD_REPORT:
			electricityMeter_zclReportCmd(pInHdlrMsg->attrCmd);
			break;
#endif
		case ZCL_CMD_DEFAULT_RSP:
			electricityMeter_zclDfltRspCmd(pInHdlrMsg->attrCmd);
			break;
		default:
			break;
	}
}

#ifdef ZCL_READ
/*********************************************************************
 * @fn      electricityMeter_zclReadRspCmd
 *
 * @brief   Handler for ZCL Read Response command.
 *
 * @param   pInHdlrMsg - incoming message to process
 *
 * @return  None
 */
static void electricityMeter_zclReadRspCmd(zclReadRspCmd_t *pReadRspCmd)
{
//    printf("electricityMeter_zclReadRspCmd\n");

    u8 numAttr = pReadRspCmd->numAttr;
    zclReadRspStatus_t *attrList = pReadRspCmd->attrList;

    for (u8 i = 0; i < numAttr; i++) {
        if (attrList[i].attrID == ZCL_ATTRID_TIME && attrList[i].status == ZCL_STA_SUCCESS) {
            resp_time = true;
        }
    }

}
#endif

#ifdef ZCL_WRITE
/*********************************************************************
 * @fn      electricityMeter_zclWriteReqCmd
 *
 * @brief   Handler for ZCL Write Request command.
 *
 * @param
 *
 * @return  None
 */
static void electricityMeter_zclWriteReqCmd(u16 clusterId, zclWriteCmd_t *pWriteReqCmd)
{

//    printf("electricityMeter_zclWriteReqCmd()\r\n");



	u8 numAttr = pWriteReqCmd->numAttr;
	zclWriteRec_t *attr = pWriteReqCmd->attrList;

	if (clusterId == ZCL_CLUSTER_SE_METERING) {
	    for (u8 i = 0; i < numAttr; i++) {
	        if (attr[i].attrID == ZCL_ATTRID_CUSTOM_DEVICE_ADDRESS && attr[i].dataType == ZCL_DATA_TYPE_UINT32) {
	            u32 device_address = BUILD_U32(attr->attrData[0], attr->attrData[1], attr->attrData[2], attr->attrData[3]);
	            if (em_config.device_address != device_address) {
	                em_config.device_address = device_address;
	                write_config();
#if UART_PRINTF_MODE && DEBUG_LEVEL
	                printf("New device address: %d\r\n", em_config.device_address);
#endif
	                zcl_setAttrVal(ELECTRICITY_METER_EP1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CUSTOM_DEVICE_ADDRESS, (u8*)&device_address);
	            }
	        } else if (attr[i].attrID == ZCL_ATTRID_CUSTOM_DEVICE_MANUFACTURER && attr[i].attrData) {
	            device_model_t model = *attr[i].attrData;
	            set_device_model(model);
                zcl_setAttrVal(ELECTRICITY_METER_EP1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CUSTOM_DEVICE_MANUFACTURER, (u8*)&model);
	        } else if (attr[i].attrID == ZCL_ATTRID_CUSTOM_MEASUREMENT_PERIOD && attr[i].dataType == ZCL_DATA_TYPE_UINT8) {
	            u8 period_in_min = *attr[i].attrData;
	            if (period_in_min == 0) period_in_min = 1;
	            u16 period_in_sec = period_in_min * 60;
	            if (em_config.measurement_period != period_in_sec) {
	                em_config.measurement_period = period_in_sec;
	                write_config();
#if UART_PRINTF_MODE && DEBUG_LEVEL
                    printf("New measurement period: %d sec\r\n", em_config.measurement_period);
#endif
                    zcl_setAttrVal(ELECTRICITY_METER_EP1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CUSTOM_MEASUREMENT_PERIOD, (u8*)&period_in_min);
                    if (g_electricityMeterCtx.timerMeasurementEvt) {
                        TL_ZB_TIMER_CANCEL(&g_electricityMeterCtx.timerMeasurementEvt);
                    }
                    g_electricityMeterCtx.timerMeasurementEvt = TL_ZB_TIMER_SCHEDULE(measure_meterCb, NULL, em_config.measurement_period * 1000);
	            }
	        }
	    }
	}

//
//	if(clusterId == ZCL_CLUSTER_GEN_ON_OFF){
//		for(u8 i = 0; i < numAttr; i++){
//			if(attr[i].attrID == ZCL_ATTRID_START_UP_ONOFF){
//				zcl_onOffAttr_save();
//			}
//		}
//	}
}

/*********************************************************************
 * @fn      electricityMeter_zclWriteRspCmd
 *
 * @brief   Handler for ZCL Write Response command.
 *
 * @param   pInHdlrMsg - incoming message to process
 *
 * @return  None
 */
static void electricityMeter_zclWriteRspCmd(zclWriteRspCmd_t *pWriteRspCmd)
{
//    printf("electricityMeter_zclWriteRspCmd\n");

}
#endif


/*********************************************************************
 * @fn      electricityMeter_zclDfltRspCmd
 *
 * @brief   Handler for ZCL Default Response command.
 *
 * @param   pInHdlrMsg - incoming message to process
 *
 * @return  None
 */
static void electricityMeter_zclDfltRspCmd(zclDefaultRspCmd_t *pDftRspCmd)
{
//  printf("electricityMeter_zclDfltRspCmd\n");
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
 * @fn      electricityMeter_zclCfgReportCmd
 *
 * @brief   Handler for ZCL Configure Report command.
 *
 * @param   pInHdlrMsg - incoming message to process
 *
 * @return  None
 */
static void electricityMeter_zclCfgReportCmd(zclCfgReportCmd_t *pCfgReportCmd)
{
//    printf("electricityMeter_zclCfgReportCmd\n");

}

/*********************************************************************
 * @fn      electricityMeter_zclCfgReportRspCmd
 *
 * @brief   Handler for ZCL Configure Report Response command.
 *
 * @param   pInHdlrMsg - incoming message to process
 *
 * @return  None
 */
static void electricityMeter_zclCfgReportRspCmd(zclCfgReportRspCmd_t *pCfgReportRspCmd)
{
//    printf("electricityMeter_zclCfgReportRspCmd\n");

}

/*********************************************************************
 * @fn      electricityMeter_zclReportCmd
 *
 * @brief   Handler for ZCL Report command.
 *
 * @param   pInHdlrMsg - incoming message to process
 *
 * @return  None
 */
static void electricityMeter_zclReportCmd(zclReportCmd_t *pReportCmd)
{
//    printf("electricityMeter_zclReportCmd\n");

}
#endif

#ifdef ZCL_BASIC
/*********************************************************************
 * @fn      electricityMeter_basicCb
 *
 * @brief   Handler for ZCL Basic Reset command.
 *
 * @param   pAddrInfo
 * @param   cmdId
 * @param   cmdPayload
 *
 * @return  status_t
 */
status_t electricityMeter_basicCb(zclIncomingAddrInfo_t *pAddrInfo, u8 cmdId, void *cmdPayload)
{
	if(cmdId == ZCL_CMD_BASIC_RESET_FAC_DEFAULT){
		//Reset all the attributes of all its clusters to factory defaults
		//zcl_nv_attr_reset();
	}

	return ZCL_STA_SUCCESS;
}
#endif

#ifdef ZCL_IDENTIFY
s32 electricityMeter_zclIdentifyTimerCb(void *arg)
{
	if(g_zcl_identifyAttrs.identifyTime <= 0){
		light_blink_stop();

		identifyTimerEvt = NULL;
		return -1;
	}
	g_zcl_identifyAttrs.identifyTime--;
	return 0;
}

void electricityMeter_zclIdentifyTimerStop(void)
{
	if(identifyTimerEvt){
		TL_ZB_TIMER_CANCEL(&identifyTimerEvt);
	}
}

/*********************************************************************
 * @fn      electricityMeter_zclIdentifyCmdHandler
 *
 * @brief   Handler for ZCL Identify command. This function will set blink LED.
 *
 * @param   endpoint
 * @param   srcAddr
 * @param   identifyTime - identify time
 *
 * @return  None
 */
void electricityMeter_zclIdentifyCmdHandler(u8 endpoint, u16 srcAddr, u16 identifyTime)
{
	g_zcl_identifyAttrs.identifyTime = identifyTime;

	if(identifyTime == 0){
		electricityMeter_zclIdentifyTimerStop();
		light_blink_stop();
	}else{
		if(!identifyTimerEvt){
			light_blink_start(identifyTime, 500, 500);
			identifyTimerEvt = TL_ZB_TIMER_SCHEDULE(electricityMeter_zclIdentifyTimerCb, NULL, 1000);
		}
	}
}

/*********************************************************************
 * @fn      electricityMeter_zcltriggerCmdHandler
 *
 * @brief   Handler for ZCL trigger command.
 *
 * @param   pTriggerEffect
 *
 * @return  None
 */
static void electricityMeter_zcltriggerCmdHandler(zcl_triggerEffect_t *pTriggerEffect)
{
	u8 effectId = pTriggerEffect->effectId;
//	u8 effectVariant = pTriggerEffect->effectVariant;

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
 * @fn      electricityMeter_identifyCb
 *
 * @brief   Handler for ZCL Identify command.
 *
 * @param   pAddrInfo
 * @param   cmdId
 * @param   cmdPayload
 *
 * @return  status_t
 */
status_t electricityMeter_identifyCb(zclIncomingAddrInfo_t *pAddrInfo, u8 cmdId, void *cmdPayload)
{
	if(pAddrInfo->dstEp == ELECTRICITY_METER_EP1){
		if(pAddrInfo->dirCluster == ZCL_FRAME_CLIENT_SERVER_DIR){
			switch(cmdId){
				case ZCL_CMD_IDENTIFY:
					electricityMeter_zclIdentifyCmdHandler(pAddrInfo->dstEp, pAddrInfo->srcAddr, ((zcl_identifyCmd_t *)cmdPayload)->identifyTime);
					break;
				case ZCL_CMD_TRIGGER_EFFECT:
					electricityMeter_zcltriggerCmdHandler((zcl_triggerEffect_t *)cmdPayload);
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
 * @fn      electricityMeter_timeCb
 *
 * @brief   Handler for ZCL Identify command.
 *
 * @param   pAddrInfo
 * @param   cmdId
 * @param   cmdPayload
 *
 * @return  status_t
 */
status_t electricityMeter_timeCb(zclIncomingAddrInfo_t *pAddrInfo, u8 cmdId, void *cmdPayload) {

    printf("electricityMeter_timeCb. cmd: 0x%x\r\n", cmdId);

    return ZCL_STA_SUCCESS;
}


/*********************************************************************
 * @fn      electricityMeter_meteringCb
 *
 * @brief   Handler for ZCL Identify command.
 *
 * @param   pAddrInfo
 * @param   cmdId
 * @param   cmdPayload
 *
 * @return  status_t
 */
status_t electricityMeter_meteringCb(zclIncomingAddrInfo_t *pAddrInfo, u8 cmdId, void *cmdPayload)
{

    return ZCL_STA_SUCCESS;
}


