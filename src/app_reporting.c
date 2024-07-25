#include "tl_common.h"
#include "zb_api.h"
#include "zcl_include.h"
#include "bdb.h"
#include "ota.h"
#include "gp.h"

#include "app_reporting.h"
#include "app_main.h"
#include "se_custom_attr.h"

#define BUILD_U48(b0, b1, b2, b3, b4, b5)   ( (uint64_t)((((uint64_t)(b5) & 0x0000000000ff) << 40) + (((uint64_t)(b4) & 0x0000000000ff) << 32) + (((uint64_t)(b3) & 0x0000000000ff) << 24) + (((uint64_t)(b2) & 0x0000000000ff) << 16) + (((uint64_t)(b1) & 0x0000000000ff) << 8) + ((uint64_t)(b0) & 0x00000000FF)) )

app_reporting_t app_reporting[ZCL_REPORTING_TABLE_NUM];

extern void reportAttr(reportCfgInfo_t *pEntry);
//void report_divisor_multiplier(reportCfgInfo_t *pEntry);

/**********************************************************************
 * Custom reporting application
 *
 * Local function
 */

void report_divisor_multiplier(reportCfgInfo_t *pEntry) {

    //force report for multiplier and divisor
    //printf("report_divisor_multiplier. clusterId: 0x%x, attrId: 0x%x\r\n", pEntry->clusterID, pEntry->attrID);

    if (pEntry->clusterID == ZCL_CLUSTER_SE_METERING) {
        switch (pEntry->attrID) {
            case ZCL_ATTRID_CURRENT_TIER_1_SUMMATION_DELIVERD:
            case ZCL_ATTRID_CURRENT_TIER_2_SUMMATION_DELIVERD:
            case ZCL_ATTRID_CURRENT_TIER_3_SUMMATION_DELIVERD:
            case ZCL_ATTRID_CURRENT_TIER_4_SUMMATION_DELIVERD:
                //printf("report tariff\r\n");
                app_forcedReport(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_MULTIPLIER);
                app_forcedReport(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_DIVISOR);
                break;
            default:
                break;
        }
    } else if (pEntry->clusterID == ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT) {
        switch (pEntry->attrID) {
            case ZCL_ATTRID_LINE_CURRENT:
                //printf("report current\r\n");
                app_forcedReport(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_AC_CURRENT_MULTIPLIER);
                app_forcedReport(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_AC_CURRENT_DIVISOR);
                break;
            case ZCL_ATTRID_RMS_VOLTAGE:
                //printf("report voltage\r\n");
                app_forcedReport(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_AC_VOLTAGE_MULTIPLIER);
                app_forcedReport(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_AC_VOLTAGE_DIVISOR);
                break;
            case ZCL_ATTRID_APPARENT_POWER:
                //printf("report power\r\n");
                app_forcedReport(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_AC_POWER_MULTIPLIER);
                app_forcedReport(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_AC_POWER_DIVISOR);
                break;
            default:
                break;
        }
    }
}

static uint8_t app_reportableChangeValueChk(uint8_t dataType, uint8_t *curValue, uint8_t *prevValue, uint8_t *reportableChange) {
    uint8_t needReport = false;

    switch(dataType) {
        case ZCL_DATA_TYPE_UINT48: {
            uint64_t P = BUILD_U48(prevValue[0], prevValue[1], prevValue[2], prevValue[3], prevValue[4], prevValue[5]);
            uint64_t C = BUILD_U48(curValue[0], curValue[1], curValue[2], curValue[3], curValue[4], curValue[5]);
            uint64_t R = BUILD_U48(reportableChange[0], reportableChange[1], reportableChange[2], reportableChange[3], reportableChange[4], reportableChange[5]);
            if(P >= C){
                needReport = ((P - C) > R) ? true : false;
            }else{
                needReport = ((C - P) > R) ? true : false;
            }
            break;
        }
        default:
            needReport = reportableChangeValueChk(dataType, curValue, prevValue, reportableChange);
            break;
    }

    return needReport;
}

static int32_t app_reportMinAttrTimerCb(void *arg) {
    app_reporting_t *app_reporting = (app_reporting_t*)arg;
    reportCfgInfo_t *pEntry = app_reporting->pEntry;

    zclAttrInfo_t *pAttrEntry = zcl_findAttribute(pEntry->endPoint, pEntry->clusterID, pEntry->attrID);
    if (!pAttrEntry) {
        //should not happen.
        ZB_EXCEPTION_POST(SYS_EXCEPTTION_ZB_ZCL_ENTRY);
        app_reporting->timerReportMinEvt = NULL;
        return -1;
    }

    if (pEntry->minInterval == pEntry->maxInterval) {
        report_divisor_multiplier(pEntry);
        reportAttr(pEntry);
        app_reporting->time_posted = clock_time();
#if UART_PRINTF_MODE && DEBUG_REPORTING
        printf("Report Min_Interval has been sent. endPoint: %d, clusterID: 0x%x, attrID: 0x%x, minInterval: %d, maxInterval: %d\r\n",
                pEntry->endPoint, pEntry->clusterID, pEntry->attrID, pEntry->minInterval, pEntry->maxInterval);
#endif
        return 0;
    }

    uint8_t len = zcl_getAttrSize(pAttrEntry->type, pAttrEntry->data);


    len = (len>8) ? (8):(len);

    if ((!zcl_analogDataType(pAttrEntry->type) && (memcmp(pEntry->prevData, pAttrEntry->data, len) != SUCCESS)) ||
            ((zcl_analogDataType(pAttrEntry->type) && app_reportableChangeValueChk(pAttrEntry->type,
            pAttrEntry->data, pEntry->prevData, pEntry->reportableChange)))) {

        report_divisor_multiplier(pEntry);
        reportAttr(pEntry);
        app_reporting->time_posted = clock_time();
#if UART_PRINTF_MODE && DEBUG_REPORTING
        printf("Report Min_Interval has been sent. endPoint: %d, clusterID: 0x%x, attrID: 0x%x, minInterval: %d, maxInterval: %d\r\n",
                pEntry->endPoint, pEntry->clusterID, pEntry->attrID, pEntry->minInterval, pEntry->maxInterval);
#endif
    }

    return 0;
}

static int32_t app_reportMaxAttrTimerCb(void *arg) {
    app_reporting_t *app_reporting = (app_reporting_t*)arg;
    reportCfgInfo_t *pEntry = app_reporting->pEntry;

    if (clock_time_exceed(app_reporting->time_posted, pEntry->minInterval*1000*1000)) {
        if (app_reporting->timerReportMinEvt) {
            TL_ZB_TIMER_CANCEL(&app_reporting->timerReportMinEvt);
        }
        report_divisor_multiplier(pEntry);
        reportAttr(pEntry);
#if UART_PRINTF_MODE && DEBUG_REPORTING
        printf("Report Max_Interval has been sent. endPoint: %d, clusterID: 0x%x, attrID: 0x%x, minInterval: %d, maxInterval: %d\r\n",
                pEntry->endPoint, pEntry->clusterID, pEntry->attrID, pEntry->minInterval, pEntry->maxInterval);
#endif
    }

    return 0;
}

static void app_reportAttrTimerStart() {
    static uint8_t first = 1;

    if (zcl_reportingEntryActiveNumGet()) {
        for(uint8_t i = 0; i < ZCL_REPORTING_TABLE_NUM; i++) {
            reportCfgInfo_t *pEntry = &reportingTab.reportCfgInfo[i];
            app_reporting[i].pEntry = pEntry;
            if (first) {
                first = 0;
            }
            if (pEntry->used && (pEntry->maxInterval != 0xFFFF) && (pEntry->minInterval || pEntry->maxInterval)) {
                if (zb_bindingTblSearched(pEntry->clusterID, pEntry->endPoint)) {
                    if (!app_reporting[i].timerReportMinEvt) {
                        if (pEntry->minInterval && pEntry->maxInterval && pEntry->minInterval <= pEntry->maxInterval) {
#if UART_PRINTF_MODE && DEBUG_REPORTING
                            printf("Start minTimer. endPoint: %d, clusterID: 0x%x, attrID: 0x%x, min: %d, max: %d\r\n", pEntry->endPoint, pEntry->clusterID, pEntry->attrID, pEntry->minInterval, pEntry->maxInterval);
#endif
                            app_reporting[i].timerReportMinEvt = TL_ZB_TIMER_SCHEDULE(app_reportMinAttrTimerCb, &app_reporting[i], pEntry->minInterval*1000);
                        }
                    }
                    if (!app_reporting[i].timerReportMaxEvt) {
                        if (pEntry->maxInterval) {
                            if (pEntry->minInterval < pEntry->maxInterval) {
                                if (pEntry->maxInterval != pEntry->minInterval && pEntry->maxInterval > pEntry->minInterval) {
#if UART_PRINTF_MODE && DEBUG_REPORTING
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

static void app_reportNoMinLimit(void) {

    if (zcl_reportingEntryActiveNumGet()) {
        zclAttrInfo_t *pAttrEntry = NULL;
        uint16_t len = 0;

        for (uint8_t i = 0; i < ZCL_REPORTING_TABLE_NUM; i++) {
            reportCfgInfo_t *pEntry = &reportingTab.reportCfgInfo[i];
            if (pEntry->used && (pEntry->maxInterval == 0 || ((pEntry->maxInterval != 0xFFFF) && (pEntry->minInterval == 0)))) {
                //there is no minimum limit
                pAttrEntry = zcl_findAttribute(pEntry->endPoint, pEntry->clusterID, pEntry->attrID);
                if (!pAttrEntry) {
                    //should not happen.
                    ZB_EXCEPTION_POST(SYS_EXCEPTTION_ZB_ZCL_ENTRY);
                    return;
                }

                len = zcl_getAttrSize(pAttrEntry->type, pAttrEntry->data);
                len = (len>8) ? (8):(len);

                if ((!zcl_analogDataType(pAttrEntry->type) && (memcmp(pEntry->prevData, pAttrEntry->data, len) != SUCCESS)) ||
                        ((zcl_analogDataType(pAttrEntry->type) && app_reportableChangeValueChk(pAttrEntry->type,
                        pAttrEntry->data, pEntry->prevData, pEntry->reportableChange)))) {

                    if (zb_bindingTblSearched(pEntry->clusterID, pEntry->endPoint)) {

                        report_divisor_multiplier(pEntry);

                        reportAttr(pEntry);

                        app_reporting[i].time_posted = clock_time();
#if UART_PRINTF_MODE && DEBUG_REPORTING
                        printf("Report No_Min_Limit has been sent. endPoint: %d, clusterID: 0x%x, attrID: 0x%x, minInterval: %d, maxInterval: %d\r\n",
                                pEntry->endPoint, pEntry->clusterID, pEntry->attrID, pEntry->minInterval, pEntry->maxInterval);
#endif
                        if (app_reporting[i].timerReportMaxEvt) {
                            TL_ZB_TIMER_CANCEL(&app_reporting[i].timerReportMaxEvt);
                        }

                        if (pEntry->maxInterval != 0) {
                            app_reporting[i].timerReportMaxEvt = TL_ZB_TIMER_SCHEDULE(app_reportMaxAttrTimerCb, &app_reporting[i], pEntry->maxInterval*1000);
#if UART_PRINTF_MODE && DEBUG_REPORTING
                            printf("Start maxTimer. endPoint: %d, clusterID: 0x%x, attrID: 0x%x, min: %d, max: %d\r\n", pEntry->endPoint, pEntry->clusterID, pEntry->attrID, pEntry->minInterval, pEntry->maxInterval);
#endif
                        }
                    }
                }
            }
        }
    }
}

/**********************************************************************
 *  Global function
 */


void app_forcedReport(uint8_t endpoint, uint16_t claster_id, uint16_t attr_id) {

    if (zb_isDeviceJoinedNwk()) {

        epInfo_t dstEpInfo;
        TL_SETSTRUCTCONTENT(dstEpInfo, 0);

        dstEpInfo.profileId = HA_PROFILE_ID;
#if FIND_AND_BIND_SUPPORT
        dstEpInfo.dstAddrMode = APS_DSTADDR_EP_NOTPRESETNT;
#else
        dstEpInfo.dstAddrMode = APS_SHORT_DSTADDR_WITHEP;
        dstEpInfo.dstEp = endpoint;
        dstEpInfo.dstAddr.shortAddr = 0xfffc;
#endif
        zclAttrInfo_t *pAttrEntry = zcl_findAttribute(endpoint, claster_id, attr_id);

        if (!pAttrEntry) {
            //should not happen.
            ZB_EXCEPTION_POST(SYS_EXCEPTTION_ZB_ZCL_ENTRY);
            return;
        }

        zcl_sendReportCmd(endpoint, &dstEpInfo,  TRUE, ZCL_FRAME_SERVER_CLIENT_DIR,
                claster_id, pAttrEntry->id, pAttrEntry->type, pAttrEntry->data);

#if UART_PRINTF_MODE && DEBUG_REPORTING
        printf("forceReportCb. endpoint: 0x%x, claster_id: 0x%x, attr_id: 0x%x\r\n", endpoint, claster_id, attr_id);
#endif
    }


}

void app_reporting_init() {

    TL_SETSTRUCTCONTENT(app_reporting, 0);

    for (uint8_t i = 0; i < ZCL_REPORTING_TABLE_NUM; i++) {
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
//    sleep_ms(0xffffffff);
}

void report_handler(void) {

    if (zb_isDeviceJoinedNwk()) {

        if (g_appCtx.timerStopReportEvt) return;

        if (zcl_reportingEntryActiveNumGet()) {


            app_reportNoMinLimit();

            //start report timers
            app_reportAttrTimerStart();
        }
    }
}

void app_all_forceReporting() {

    if (zb_isDeviceJoinedNwk()) {
        app_forcedReport(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CUSTOM_DEVICE_MODEL);
        app_forcedReport(APP_ENDPOINT_1, ZCL_CLUSTER_GEN_DEVICE_TEMP_CONFIG, ZCL_ATTRID_DEV_TEMP_CURR_TEMP);
        app_forcedReport(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_METER_SERIAL_NUMBER);
        app_forcedReport(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CUSTOM_DATE_RELEASE);
        app_forcedReport(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_MULTIPLIER);
        app_forcedReport(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_DIVISOR);
        app_forcedReport(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_1_SUMMATION_DELIVERD);
        app_forcedReport(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_2_SUMMATION_DELIVERD);
        app_forcedReport(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_3_SUMMATION_DELIVERD);
        app_forcedReport(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_TIER_4_SUMMATION_DELIVERD);
        app_forcedReport(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_AC_VOLTAGE_MULTIPLIER);
        app_forcedReport(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_AC_VOLTAGE_DIVISOR);
        app_forcedReport(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_RMS_VOLTAGE);
        app_forcedReport(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_AC_POWER_MULTIPLIER);
        app_forcedReport(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_AC_POWER_DIVISOR);
        app_forcedReport(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_APPARENT_POWER);
        app_forcedReport(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_AC_CURRENT_MULTIPLIER);
        app_forcedReport(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_AC_CURRENT_DIVISOR);
        app_forcedReport(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_LINE_CURRENT);
        app_forcedReport(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_REMAINING_BATTERY_LIFE);
    }

}
