/********************************************************************************************************
 * @file    electricityMeter.c
 *
 * @brief   This is the source file for electricityMeter
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
#include "bdb.h"
#include "ota.h"
#include "gp.h"

#include "app_ui.h"
#include "electricityMeter.h"
#include "app_uart.h"
#include "device.h"
//#include "factory_reset.h"

/**********************************************************************
 * LOCAL CONSTANTS
 */


/**********************************************************************
 * TYPEDEFS
 */

/**********************************************************************
 * LOCAL VARIABLES
 */

/**********************************************************************
 * GLOBAL VARIABLES
 */

app_reporting_t app_reporting[ZCL_REPORTING_TABLE_NUM];

app_ctx_t g_electricityMeterCtx = {
        .timerLedStatusEvt = NULL,
};


#ifdef ZCL_OTA
extern ota_callBack_t electricityMeter_otaCb;

//running code firmware information
ota_preamble_t electricityMeter_otaInfo = {
	.fileVer 			= FILE_VERSION,
	.imageType 			= IMAGE_TYPE,
	.manufacturerCode 	= MANUFACTURER_CODE_TELINK,
};
#endif


//Must declare the application call back function which used by ZDO layer
const zdo_appIndCb_t appCbLst = {
	bdb_zdoStartDevCnf,//start device cnf cb
	NULL,//reset cnf cb
	NULL,//device announce indication cb
	electricityMeter_leaveIndHandler,//leave ind cb
	electricityMeter_leaveCnfHandler,//leave cnf cb
	electricityMeter_nwkUpdateIndicateHandler,//nwk update ind cb
	NULL,//permit join ind cb
	NULL,//nlme sync cnf cb
	NULL,//tc join ind cb
	NULL,//tc detects that the frame counter is near limit
};


/**
 *  @brief Definition for bdb commissioning setting
 */
bdb_commissionSetting_t g_bdbCommissionSetting = {
	.linkKey.tcLinkKey.keyType = SS_GLOBAL_LINK_KEY,
	.linkKey.tcLinkKey.key = (u8 *)tcLinkKeyCentralDefault,       		//can use unique link key stored in NV

	.linkKey.distributeLinkKey.keyType = MASTER_KEY,
	.linkKey.distributeLinkKey.key = (u8 *)linkKeyDistributedMaster,  	//use linkKeyDistributedCertification before testing

	.linkKey.touchLinkKey.keyType = MASTER_KEY,
	.linkKey.touchLinkKey.key = (u8 *)touchLinkKeyMaster,   			//use touchLinkKeyCertification before testing

#if TOUCHLINK_SUPPORT
	.touchlinkEnable = 1,												/* enable touch-link */
#else
	.touchlinkEnable = 0,												/* disable touch-link */
#endif
	.touchlinkChannel = DEFAULT_CHANNEL, 								/* touch-link default operation channel for target */
	.touchlinkLqiThreshold = 0xA0,			   							/* threshold for touch-link scan req/resp command */
};

/**********************************************************************
 * LOCAL VARIABLES
 */
ev_timer_event_t *electricityMeterAttrsStoreTimerEvt = NULL;


/**********************************************************************
 * FUNCTIONS
 */

extern void reportAttr(reportCfgInfo_t *pEntry);

/**********************************************************************
 * Custom reporting application
 */

static void app_reporting_init() {

    TL_SETSTRUCTCONTENT(app_reporting, 0);

    for (u8 i = 0; i < ZCL_REPORTING_TABLE_NUM; i++) {
        reportCfgInfo_t *pEntry = &reportingTab.reportCfgInfo[i];
//        printf("(%d) used: %s\r\n", i, pEntry->used?"true":"false");
        if (pEntry->used) {
//            printf("(%d) endPoint: %d\r\n", i, pEntry->endPoint);
//            printf("(%d) clusterID: 0x%x\r\n", i, pEntry->clusterID);
//            printf("(%d) attrID: 0x%x\r\n", i, pEntry->attrID);
//            printf("(%d) dataType: 0x%x\r\n", i, pEntry->dataType);
//            printf("(%d) minInterval: %d\r\n", i, pEntry->minInterval);
//            printf("(%d) maxInterval: %d\r\n", i, pEntry->maxInterval);
            app_reporting[i].pEntry = pEntry;
        }
    }
}

static u8 app_reportableChangeValueChk(u8 dataType, u8 *curValue, u8 *prevValue, u8 *reportableChange) {
    u8 needReport = false;

    switch(dataType) {
        case ZCL_DATA_TYPE_UINT48:
        {
            u64 P = BUILD_U48(prevValue[0], prevValue[1], prevValue[2], prevValue[3], prevValue[4], prevValue[5]);
            u64 C = BUILD_U48(curValue[0], curValue[1], curValue[2], curValue[3], curValue[4], curValue[5]);
            u64 R = BUILD_U48(reportableChange[0], reportableChange[1], reportableChange[2], reportableChange[3], reportableChange[4], reportableChange[5]);
            if(P >= C){
                needReport = ((P - C) > R) ? true : false;
            }else{
                needReport = ((C - P) > R) ? true : false;
            }
            break;
        }
        default:
            break;
    }

    return needReport;
}

static s32 app_reportMinAttrTimerCb(void *arg) {
    app_reporting_t *app_reporting = (app_reporting_t*)arg;
    reportCfgInfo_t *pEntry = app_reporting->pEntry;

    zclAttrInfo_t *pAttrEntry = zcl_findAttribute(pEntry->endPoint, pEntry->clusterID, pEntry->attrID);
    if(!pAttrEntry){
        //should not happen.
        ZB_EXCEPTION_POST(SYS_EXCEPTTION_ZB_ZCL_ENTRY);
        app_reporting->timerReportMinEvt = NULL;
        return -1;
    }

    if (pEntry->minInterval == pEntry->maxInterval) {
        reportAttr(pEntry);
        app_reporting->time_posted = clock_time();
#if UART_PRINTF_MODE && DEBUG_LEVEL
        printf("Report has been sent. endPoint: %d, clusterID: 0x%x, attrID: 0x%x, minInterval: %d, maxInterval: %d\r\n",
                pEntry->endPoint, pEntry->clusterID, pEntry->attrID, pEntry->minInterval, pEntry->maxInterval);
#endif
        return 0;
    }

    u8 len = zcl_getAttrSize(pAttrEntry->type, pAttrEntry->data);


    len = (len>8) ? (8):(len);

    if( (!zcl_analogDataType(pAttrEntry->type) && (memcmp(pEntry->prevData, pAttrEntry->data, len) != SUCCESS)) ||
                    ((zcl_analogDataType(pAttrEntry->type) && reportableChangeValueChk(pAttrEntry->type,
                    pAttrEntry->data, pEntry->prevData, pEntry->reportableChange))) ||
                    ((zcl_analogDataType(pAttrEntry->type) && pAttrEntry->type == ZCL_DATA_TYPE_UINT48 &&
                    app_reportableChangeValueChk(pAttrEntry->type, pAttrEntry->data, pEntry->prevData, pEntry->reportableChange)))) {
        reportAttr(pEntry);
        app_reporting->time_posted = clock_time();
#if UART_PRINTF_MODE && DEBUG_LEVEL
        printf("Report has been sent. endPoint: %d, clusterID: 0x%x, attrID: 0x%x, minInterval: %d, maxInterval: %d\r\n",
                pEntry->endPoint, pEntry->clusterID, pEntry->attrID, pEntry->minInterval, pEntry->maxInterval);
#endif
    }

    return 0;
}

static s32 app_reportMaxAttrTimerCb(void *arg) {
    app_reporting_t *app_reporting = (app_reporting_t*)arg;
    reportCfgInfo_t *pEntry = app_reporting->pEntry;

    if (clock_time_exceed(app_reporting->time_posted, pEntry->minInterval*1000*1000)) {
        if (app_reporting->timerReportMinEvt) {
            TL_ZB_TIMER_CANCEL(&app_reporting->timerReportMinEvt);
        }
#if UART_PRINTF_MODE && DEBUG_LEVEL
        printf("Report has been sent. endPoint: %d, clusterID: 0x%x, attrID: 0x%x, minInterval: %d, maxInterval: %d\r\n",
                pEntry->endPoint, pEntry->clusterID, pEntry->attrID, pEntry->minInterval, pEntry->maxInterval);
#endif
        reportAttr(pEntry);
    }

    return 0;
}

static void app_reportAttrTimerStart() {
    if(zcl_reportingEntryActiveNumGet()) {
        for(u8 i = 0; i < ZCL_REPORTING_TABLE_NUM; i++) {
            reportCfgInfo_t *pEntry = &reportingTab.reportCfgInfo[i];
            app_reporting[i].pEntry = pEntry;
            if(pEntry->used && (pEntry->maxInterval != 0xFFFF) && (pEntry->minInterval || pEntry->maxInterval)){
                if(zb_bindingTblSearched(pEntry->clusterID, pEntry->endPoint)) {
                    if (!app_reporting[i].timerReportMinEvt) {
                        if (pEntry->minInterval && pEntry->maxInterval && pEntry->minInterval <= pEntry->maxInterval) {
#if UART_PRINTF_MODE && DEBUG_LEVEL
                            printf("Start minTimer. endPoint: %d, clusterID: 0x%x, attrID: 0x%x, min: %d, max: %d\r\n", pEntry->endPoint, pEntry->clusterID, pEntry->attrID, pEntry->minInterval, pEntry->maxInterval);
#endif
                            app_reporting[i].timerReportMinEvt = TL_ZB_TIMER_SCHEDULE(app_reportMinAttrTimerCb, &app_reporting[i], pEntry->minInterval*1000);
                        }
                    }
                    if (!app_reporting[i].timerReportMaxEvt) {
                        if (pEntry->maxInterval) {
                            if (pEntry->minInterval < pEntry->maxInterval) {
                                if (pEntry->maxInterval != pEntry->minInterval && pEntry->maxInterval > pEntry->minInterval) {
#if UART_PRINTF_MODE && DEBUG_LEVEL
                                    printf("Start maxTimer. endPoint: %d, clusterID: 0x%x, attrID: 0x%x, min: %d, max: %d\r\n", pEntry->endPoint, pEntry->clusterID, pEntry->attrID, pEntry->minInterval, pEntry->maxInterval);
#endif
                                    app_reporting[i].timerReportMaxEvt = TL_ZB_TIMER_SCHEDULE(app_reportMaxAttrTimerCb, &app_reporting[i], pEntry->maxInterval*1000);
                                }
                            }
                        } else {
                            app_reportMinAttrTimerCb(&app_reporting[i]);
                        }

                    }
                }
            }
        }
    }
}

void app_reportNoMinLimit(void)
{
    if(zcl_reportingEntryActiveNumGet()){
        zclAttrInfo_t *pAttrEntry = NULL;
        u16 len = 0;

        for(u8 i = 0; i < ZCL_REPORTING_TABLE_NUM; i++){
            reportCfgInfo_t *pEntry = &reportingTab.reportCfgInfo[i];
            if(pEntry->used && (pEntry->maxInterval != 0xFFFF) && (pEntry->minInterval == 0)){
                //there is no minimum limit
                pAttrEntry = zcl_findAttribute(pEntry->endPoint, pEntry->clusterID, pEntry->attrID);
                if(!pAttrEntry){
                    //should not happen.
                    ZB_EXCEPTION_POST(SYS_EXCEPTTION_ZB_ZCL_ENTRY);
                    return;
                }

                len = zcl_getAttrSize(pAttrEntry->type, pAttrEntry->data);
                len = (len>8) ? (8):(len);

                if( (!zcl_analogDataType(pAttrEntry->type) && (memcmp(pEntry->prevData, pAttrEntry->data, len) != SUCCESS)) ||
                        ((zcl_analogDataType(pAttrEntry->type) && reportableChangeValueChk(pAttrEntry->type,
                        pAttrEntry->data, pEntry->prevData, pEntry->reportableChange))) ||
                        ((zcl_analogDataType(pAttrEntry->type) && pAttrEntry->type == ZCL_DATA_TYPE_UINT48 &&
                        app_reportableChangeValueChk(pAttrEntry->type, pAttrEntry->data, pEntry->prevData, pEntry->reportableChange)))) {

                    reportAttr(pEntry);
                    app_reporting->time_posted = clock_time();
#if UART_PRINTF_MODE && DEBUG_LEVEL
                    printf("Report has been sent. endPoint: %d, clusterID: 0x%x, attrID: 0x%x, minInterval: %d, maxInterval: %d\r\n",
                            pEntry->endPoint, pEntry->clusterID, pEntry->attrID, pEntry->minInterval, pEntry->maxInterval);
#endif
                    if (app_reporting[i].timerReportMaxEvt) {
                        TL_ZB_TIMER_CANCEL(&app_reporting[i].timerReportMaxEvt);
                    }
                    app_reporting[i].timerReportMaxEvt = TL_ZB_TIMER_SCHEDULE(app_reportMaxAttrTimerCb, &app_reporting[i], pEntry->maxInterval*1000);
#if UART_PRINTF_MODE && DEBUG_LEVEL
                    printf("Start maxTimer. endPoint: %d, clusterID: 0x%x, attrID: 0x%x, min: %d, max: %d\r\n", pEntry->endPoint, pEntry->clusterID, pEntry->attrID, pEntry->minInterval, pEntry->maxInterval);
#endif
                }
            }
        }
    }
}


/*********************************************************************
 * @fn      stack_init
 *
 * @brief   This function initialize the ZigBee stack and related profile. If HA/ZLL profile is
 *          enabled in this application, related cluster should be registered here.
 *
 * @param   None
 *
 * @return  None
 */
void stack_init(void)
{
	/* Initialize ZB stack */
	zb_init();

	/* Register stack CB */
    zb_zdoCbRegister((zdo_appIndCb_t *)&appCbLst);
}

/*********************************************************************
 * @fn      user_app_init
 *
 * @brief   This function initialize the application(Endpoint) information for this node.
 *
 * @param   None
 *
 * @return  None
 */
void user_app_init(void)
{
	af_nodeDescManuCodeUpdate(MANUFACTURER_CODE_TELINK);

    /* Initialize ZCL layer */
	/* Register Incoming ZCL Foundation command/response messages */
    zcl_init(electricityMeter_zclProcessIncomingMsg);

	/* Register endPoint */
	af_endpointRegister(ELECTRICITY_METER_EP1, (af_simple_descriptor_t *)&electricityMeter_simpleDesc, zcl_rx_handler, NULL);

	zcl_reportingTabInit();

	/* Register ZCL specific cluster information */
	zcl_register(ELECTRICITY_METER_EP1, ELECTRICITY_METER_CB_CLUSTER_NUM, (zcl_specClusterInfo_t *)g_electricityMeterClusterList);

#if ZCL_GP_SUPPORT
	/* Initialize GP */
	gp_init(ELECTRICITY_METER_EP1);
#endif

#if ZCL_OTA_SUPPORT
	/* Initialize OTA */
    ota_init(OTA_TYPE_CLIENT, (af_simple_descriptor_t *)&electricityMeter_simpleDesc, &electricityMeter_otaInfo, &electricityMeter_otaCb);
#endif

    app_uart_init();
    init_config(true);
    init_button();

    adc_temp_init();

    g_electricityMeterCtx.timerLedStatusEvt = TL_ZB_TIMER_SCHEDULE(flashLedStatusCb, NULL, TIMEOUT_5SEC);
    TL_ZB_TIMER_SCHEDULE(getTimeCb, NULL, TIMEOUT_5MIN);
    getTemperatureCb(NULL);
    TL_ZB_TIMER_SCHEDULE(getTemperatureCb, NULL, TIMEOUT_15SEC);

    set_device_model(em_config.device_model);

    g_electricityMeterCtx.timerMeasurementEvt = TL_ZB_TIMER_SCHEDULE(measure_meterCb, NULL, TIMEOUT_5SEC /*DEFAULT_MEASUREMENT_PERIOD * 1000*/);
}


static void report_handler(void) {
    if(zb_isDeviceJoinedNwk()) {

        if (g_electricityMeterCtx.timerStopReportEvt) return;

        if(zcl_reportingEntryActiveNumGet()) {

            app_reportNoMinLimit();

            //start report timers
            app_reportAttrTimerStart();
        }
    }
}


void app_task(void)
{

    button_handler();

//	localPermitJoinState();
	if(BDB_STATE_GET() == BDB_STATE_IDLE){
		//factroyRst_handler();

		report_handler();
	}
}

static void electricityMeterSysException(void)
{

#if 1
	SYSTEM_RESET();
#else
	led_on(LED_STATUS);
	while(1);
#endif
}

/*********************************************************************
 * @fn      user_init
 *
 * @brief   User level initialization code.
 *
 * @param   isRetention - if it is waking up with ram retention.
 *
 * @return  None
 */
void user_init(bool isRetention)
{
	(void)isRetention;

	/* Initialize LEDs*/
	light_init();

	//factroyRst_init();

	/* Initialize Stack */
	stack_init();

	/* Initialize user application */
	user_app_init();

	/* Register except handler for test */
	sys_exceptHandlerRegister(electricityMeterSysException);

	/* User's Task */
#if ZBHCI_EN
	zbhciInit();
	ev_on_poll(EV_POLL_HCI, zbhciTask);
#endif
	ev_on_poll(EV_POLL_IDLE, app_task);

    /* Read the pre-install code from NV */
	if(bdb_preInstallCodeLoad(&g_electricityMeterCtx.tcLinkKey.keyType, g_electricityMeterCtx.tcLinkKey.key) == RET_OK){
		g_bdbCommissionSetting.linkKey.tcLinkKey.keyType = g_electricityMeterCtx.tcLinkKey.keyType;
		g_bdbCommissionSetting.linkKey.tcLinkKey.key = g_electricityMeterCtx.tcLinkKey.key;
	}

    u8 reportableChange = 0x00;
    bdb_defaultReportingCfg(ELECTRICITY_METER_EP1, HA_PROFILE_ID, ZCL_CLUSTER_GEN_DEVICE_TEMP_CONFIG, ZCL_ATTRID_DEV_TEMP_CURR_TEMP,
            REPORTING_MIN, REPORTING_MAX, (u8 *)&reportableChange);
//    bdb_defaultReportingCfg(ELECTRICITY_METER_EP1, HA_PROFILE_ID, ZCL_CLUSTER_GEN_POWER_CFG, ZCL_ATTRID_BATTERY_PERCENTAGE_REMAINING,
//            REPORTING_MIN, REPORTING_MAX, (u8 *)&reportableChange);
//    bdb_defaultReportingCfg(ELECTRICITY_METER_EP1, HA_PROFILE_ID, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_SUMMATION_DELIVERD,
//            0, REPORTING_MIN, (u8 *)&reportableChange);

    /* custom reporting application (non SDK) */
    app_reporting_init();


    /* Initialize BDB */
	bdb_init((af_simple_descriptor_t *)&electricityMeter_simpleDesc, &g_bdbCommissionSetting, &g_zbBdbCb, 1);
}


