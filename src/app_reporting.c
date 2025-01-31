#include "app_main.h"

#define BUILD_U48(b0, b1, b2, b3, b4, b5)   ( (uint64_t)((((uint64_t)(b5) & 0x0000000000ff) << 40) + (((uint64_t)(b4) & 0x0000000000ff) << 32) + (((uint64_t)(b3) & 0x0000000000ff) << 24) + (((uint64_t)(b2) & 0x0000000000ff) << 16) + (((uint64_t)(b1) & 0x0000000000ff) << 8) + ((uint64_t)(b0) & 0x00000000FF)) )

uint8_t counter_delivered_multdiv = 0;
uint8_t counter_current_multdiv = 0;
uint8_t counter_power_multdiv = 0;
uint8_t counter_voltage_multdiv = 0;

app_reporting_t app_reporting[ZCL_REPORTING_TABLE_NUM];

//void report_divisor_multiplier(reportCfgInfo_t *pEntry);

#if DEBUG_REPORTING

static uint8_t attr_name[ATTR_MAX][48] = {{"ZCL_ATTRID_CUSTOM_DEVICE_MODEL"},
                                          {"ZCL_ATTRID_DEV_TEMP_CURR_TEMP"},
                                          {"ZCL_ATTRID_METER_SERIAL_NUMBER"},
                                          {"ZCL_ATTRID_CUSTOM_DATE_RELEASE"},
                                          {"ZCL_ATTRID_MULTIPLIER"},
                                          {"ZCL_ATTRID_DIVISOR"},
                                          {"ZCL_ATTRID_CURRENT_SUMMATION_DELIVERD"},
                                          {"ZCL_ATTRID_CURRENT_TIER_1_SUMMATION_DELIVERD"},
                                          {"ZCL_ATTRID_CURRENT_TIER_2_SUMMATION_DELIVERD"},
                                          {"ZCL_ATTRID_CURRENT_TIER_3_SUMMATION_DELIVERD"},
                                          {"ZCL_ATTRID_CURRENT_TIER_4_SUMMATION_DELIVERD"},
                                          {"ZCL_ATTRID_AC_VOLTAGE_MULTIPLIER"},
                                          {"ZCL_ATTRID_AC_VOLTAGE_DIVISOR"},
                                          {"ZCL_ATTRID_RMS_VOLTAGE"},
                                          {"ZCL_ATTRID_AC_POWER_MULTIPLIER"},
                                          {"ZCL_ATTRID_AC_POWER_DIVISOR"},
                                          {"ZCL_ATTRID_APPARENT_POWER"},
                                          {"ZCL_ATTRID_AC_CURRENT_MULTIPLIER"},
                                          {"ZCL_ATTRID_AC_CURRENT_DIVISOR"},
                                          {"ZCL_ATTRID_LINE_CURRENT"},
                                          {"ZCL_ATTRID_REMAINING_BATTERY_LIFE"}};

static uint8_t get_attr_name(uint16_t cluster_id, uint16_t attr_id) {
    uint8_t attr_num;

    if (cluster_id == ZCL_CLUSTER_SE_METERING) {
        switch(attr_id) {
            case ZCL_ATTRID_CUSTOM_DEVICE_MODEL:
                attr_num = DEVICE_MODEL;
                break;
            case ZCL_ATTRID_METER_SERIAL_NUMBER:
                attr_num = SERIAL_NUMBER;
                break;
            case ZCL_ATTRID_CUSTOM_DATE_RELEASE:
                attr_num = DATE_RELEASE;
                break;
            case ZCL_ATTRID_MULTIPLIER:
                attr_num = MULTIPLIER;
                break;
            case ZCL_ATTRID_DIVISOR:
                attr_num = DIVISOR;
                break;
            case ZCL_ATTRID_CURRENT_SUMMATION_DELIVERD:
                attr_num = CUR_SUMM_DELIVERD;
                break;
            case ZCL_ATTRID_CURRENT_TIER_1_SUMMATION_DELIVERD:
                attr_num = CUR_T1_SUMM_DELIVERD;
                break;
            case ZCL_ATTRID_CURRENT_TIER_2_SUMMATION_DELIVERD:
                attr_num = CUR_T2_SUMM_DELIVERD;
                break;
            case ZCL_ATTRID_CURRENT_TIER_3_SUMMATION_DELIVERD:
                attr_num = CUR_T3_SUMM_DELIVERD;
                break;
            case ZCL_ATTRID_CURRENT_TIER_4_SUMMATION_DELIVERD:
                attr_num = CUR_T4_SUMM_DELIVERD;
                break;
            case ZCL_ATTRID_REMAINING_BATTERY_LIFE:
                attr_num = BATTERY_LIFE;
                break;
            default:
                attr_num = 0xff;
                break;
        }
    } else if (cluster_id == ZCL_CLUSTER_GEN_DEVICE_TEMP_CONFIG) {
        switch(attr_id) {
            case ZCL_ATTRID_DEV_TEMP_CURR_TEMP:
                attr_num = CURR_TEMP;
                break;
            default:
                attr_num = 0xff;
                break;
        }
    } else if (cluster_id == ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT) {
        switch(attr_id) {
            case ZCL_ATTRID_AC_VOLTAGE_MULTIPLIER:
                attr_num = VOLTAGE_MULTIPLIER;
                break;
            case ZCL_ATTRID_AC_VOLTAGE_DIVISOR:
                attr_num = VOLTAGE_DIVISOR;
                break;
            case ZCL_ATTRID_RMS_VOLTAGE:
                attr_num = VOLTAGE;
                break;
            case ZCL_ATTRID_AC_POWER_MULTIPLIER:
                attr_num = POWER_MULTIPLIER;
                break;
            case ZCL_ATTRID_AC_POWER_DIVISOR:
                attr_num = POWER_DIVISOR;
                break;
            case ZCL_ATTRID_APPARENT_POWER:
                attr_num = POWER;
                break;
            case ZCL_ATTRID_AC_CURRENT_MULTIPLIER:
                attr_num = CURRENT_MULTIPLIER;
                break;
            case ZCL_ATTRID_AC_CURRENT_DIVISOR:
                attr_num = CURRENT_DIVISOR;
                break;
            case ZCL_ATTRID_LINE_CURRENT:
                attr_num = CURRENT;
                break;
            default:
                attr_num = 0xff;
                break;
        }
    } else {
        attr_num = 0xff;
    }

    return attr_num;
}
#endif

/**********************************************************************
 * Custom reporting application
 *
 * Local function
 */

static void reportAttr(reportCfgInfo_t *pEntry) {
    if(!zb_bindingTblSearched(pEntry->clusterID, pEntry->endPoint)){
        return;
    }

    epInfo_t dstEpInfo;
    TL_SETSTRUCTCONTENT(dstEpInfo, 0);

    dstEpInfo.dstAddrMode = APS_DSTADDR_EP_NOTPRESETNT;
    dstEpInfo.profileId = pEntry->profileID;

    zclAttrInfo_t *pAttrEntry = zcl_findAttribute(pEntry->endPoint, pEntry->clusterID, pEntry->attrID);
    if(!pAttrEntry){
        //should not happen.
        ZB_EXCEPTION_POST(SYS_EXCEPTTION_ZB_ZCL_ENTRY);
        return;
    }

    u16 len = zcl_getAttrSize(pAttrEntry->type, pAttrEntry->data);

    len = (len>8) ? (8):(len);

    //store for next compare
    memcpy(pEntry->prevData, pAttrEntry->data, len);

    zcl_sendReportCmd(pEntry->endPoint, &dstEpInfo,  TRUE, ZCL_FRAME_SERVER_CLIENT_DIR,
                      pEntry->clusterID, pAttrEntry->id, pAttrEntry->type, pAttrEntry->data);
}


void report_divisor_multiplier(reportCfgInfo_t *pEntry) {

    //force report for multiplier and divisor
    //printf("report_divisor_multiplier. clusterId: 0x%x, attrId: 0x%x\r\n", pEntry->clusterID, pEntry->attrID);

    if (pEntry->clusterID == ZCL_CLUSTER_SE_METERING) {
        switch (pEntry->attrID) {
            case ZCL_ATTRID_CURRENT_TIER_1_SUMMATION_DELIVERD:
            case ZCL_ATTRID_CURRENT_TIER_2_SUMMATION_DELIVERD:
            case ZCL_ATTRID_CURRENT_TIER_3_SUMMATION_DELIVERD:
            case ZCL_ATTRID_CURRENT_TIER_4_SUMMATION_DELIVERD:
                //printf("report tariff, counter_delivered_multdiv: %d\r\n", counter_delivered_multdiv);
                if(counter_delivered_multdiv++ == 0) {
                    app_forcedReport(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_MULTIPLIER);
                    app_forcedReport(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_DIVISOR);
                } else {
                    if (counter_delivered_multdiv == 10) {
                        counter_delivered_multdiv = 0;
                    }
                }
                break;
            default:
                break;
        }
    } else if (pEntry->clusterID == ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT) {
        switch (pEntry->attrID) {
            case ZCL_ATTRID_LINE_CURRENT:
                //printf("report current, counter_current_multdiv: %d\r\n", counter_current_multdiv);
                if(counter_current_multdiv++ == 0) {
                    app_forcedReport(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_AC_CURRENT_MULTIPLIER);
                    app_forcedReport(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_AC_CURRENT_DIVISOR);
                } else {
                    if (counter_current_multdiv == 10) {
                        counter_current_multdiv = 0;
                    }
                }
                break;
            case ZCL_ATTRID_RMS_VOLTAGE:
                //printf("report voltage, counter_voltage_multdiv: %d\r\n", counter_voltage_multdiv);
                if(counter_voltage_multdiv++ == 0) {
                    app_forcedReport(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_AC_VOLTAGE_MULTIPLIER);
                    app_forcedReport(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_AC_VOLTAGE_DIVISOR);
                } else {
                    if (counter_voltage_multdiv == 10) {
                        counter_voltage_multdiv = 0;
                    }
                }
                break;
            case ZCL_ATTRID_APPARENT_POWER:
                //printf("report power, counter_power_multdiv: %d\r\n", counter_power_multdiv);
                if(counter_power_multdiv++ == 0) {
                    app_forcedReport(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_AC_POWER_MULTIPLIER);
                    app_forcedReport(APP_ENDPOINT_1, ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT, ZCL_ATTRID_AC_POWER_DIVISOR);
                } else {
                    if (counter_power_multdiv == 10) {
                        counter_power_multdiv = 0;
                    }
                }
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
        uint8_t attr_num = get_attr_name(pEntry->clusterID, pEntry->attrID);
        if (attr_num == 0xff) {
            printf("Report Min_Interval has been sent. endPoint: %d, clusterID: 0x%x, attrID: 0x%x, minInterval: %d, maxInterval: %d\r\n",
                    pEntry->endPoint, pEntry->clusterID, pEntry->attrID, pEntry->minInterval, pEntry->maxInterval);
        } else {
            printf("Report Min_Interval has been sent. endPoint: %d, clusterID: 0x%x, attrID: %s, minInterval: %d, maxInterval: %d\r\n",
                    pEntry->endPoint, pEntry->clusterID, attr_name[attr_num], pEntry->minInterval, pEntry->maxInterval);
        }
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
        uint8_t attr_num = get_attr_name(pEntry->clusterID, pEntry->attrID);
        if (attr_num == 0xff) {
            printf("Report Min_Interval has been sent. endPoint: %d, clusterID: 0x%x, attrID: 0x%x, minInterval: %d, maxInterval: %d\r\n",
                    pEntry->endPoint, pEntry->clusterID, pEntry->attrID, pEntry->minInterval, pEntry->maxInterval);
        } else {
            printf("Report Min_Interval has been sent. endPoint: %d, clusterID: 0x%x, attrID: %s, minInterval: %d, maxInterval: %d\r\n",
                    pEntry->endPoint, pEntry->clusterID, attr_name[attr_num], pEntry->minInterval, pEntry->maxInterval);
        }
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
        uint8_t attr_num = get_attr_name(pEntry->clusterID, pEntry->attrID);
        if (attr_num == 0xff) {
            printf("Report Max_Interval has been sent. endPoint: %d, clusterID: 0x%x, attrID: 0x%x, minInterval: %d, maxInterval: %d\r\n",
                    pEntry->endPoint, pEntry->clusterID, pEntry->attrID, pEntry->minInterval, pEntry->maxInterval);
        } else {
            printf("Report Max_Interval has been sent. endPoint: %d, clusterID: 0x%x, attrID: %s, minInterval: %d, maxInterval: %d\r\n",
                    pEntry->endPoint, pEntry->clusterID, attr_name[attr_num], pEntry->minInterval, pEntry->maxInterval);
        }
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
                            uint8_t attr_num = get_attr_name(pEntry->clusterID, pEntry->attrID);
                            if (attr_num == 0xff) {
                                printf("Start minTimer. endPoint: %d, clusterID: 0x%x, attrID: 0x%x, min: %d, max: %d\r\n",
                                        pEntry->endPoint, pEntry->clusterID, pEntry->attrID, pEntry->minInterval, pEntry->maxInterval);
                            } else {
                                printf("Start minTimer. endPoint: %d, clusterID: 0x%x, attrID: %s, min: %d, max: %d\r\n",
                                        pEntry->endPoint, pEntry->clusterID, attr_name[attr_num], pEntry->minInterval, pEntry->maxInterval);
                            }
#endif
                            app_reporting[i].timerReportMinEvt = TL_ZB_TIMER_SCHEDULE(app_reportMinAttrTimerCb, &app_reporting[i], pEntry->minInterval*1000);
                        }
                    }
                    if (!app_reporting[i].timerReportMaxEvt) {
                        if (pEntry->maxInterval) {
                            if (pEntry->minInterval < pEntry->maxInterval) {
                                if (pEntry->maxInterval != pEntry->minInterval && pEntry->maxInterval > pEntry->minInterval) {
#if UART_PRINTF_MODE && DEBUG_REPORTING
                                    uint8_t attr_num = get_attr_name(pEntry->clusterID, pEntry->attrID);
                                    if (attr_num == 0xff) {
                                        printf("Start maxTimer. endPoint: %d, clusterID: 0x%x, attrID: 0x%x, min: %d, max: %d\r\n",
                                                pEntry->endPoint, pEntry->clusterID, pEntry->attrID, pEntry->minInterval, pEntry->maxInterval);
                                    } else {
                                        printf("Start maxTimer. endPoint: %d, clusterID: 0x%x, attrID: %s, min: %d, max: %d\r\n",
                                                pEntry->endPoint, pEntry->clusterID, attr_name[attr_num], pEntry->minInterval, pEntry->maxInterval);
                                    }
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
                        uint8_t attr_num = get_attr_name(pEntry->clusterID, pEntry->attrID);
                        if (attr_num == 0xff) {
                            printf("Report No_Min_Limit has been sent. endPoint: %d, clusterID: 0x%x, attrID: 0x%x, minInterval: %d, maxInterval: %d\r\n",
                                    pEntry->endPoint, pEntry->clusterID, pEntry->attrID, pEntry->minInterval, pEntry->maxInterval);
                        } else {
                            printf("Report No_Min_Limit has been sent. endPoint: %d, clusterID: 0x%x, attrID: %s, minInterval: %d, maxInterval: %d\r\n",
                                    pEntry->endPoint, pEntry->clusterID, attr_name[attr_num], pEntry->minInterval, pEntry->maxInterval);
                        }
#endif
                        if (app_reporting[i].timerReportMaxEvt) {
                            TL_ZB_TIMER_CANCEL(&app_reporting[i].timerReportMaxEvt);
                        }

                        if (pEntry->maxInterval != 0) {
                            app_reporting[i].timerReportMaxEvt = TL_ZB_TIMER_SCHEDULE(app_reportMaxAttrTimerCb, &app_reporting[i], pEntry->maxInterval*1000);
#if UART_PRINTF_MODE && DEBUG_REPORTING
                            uint8_t attr_num = get_attr_name(pEntry->clusterID, pEntry->attrID);
                            if (attr_num == 0xff) {
                                printf("Start maxTimer. endPoint: %d, clusterID: 0x%x, attrID: 0x%x, min: %d, max: %d\r\n",
                                        pEntry->endPoint, pEntry->clusterID, pEntry->attrID, pEntry->minInterval, pEntry->maxInterval);
                            } else {
                                printf("Start maxTimer. endPoint: %d, clusterID: 0x%x, attrID: %s, min: %d, max: %d\r\n",
                                        pEntry->endPoint, pEntry->clusterID, attr_name[attr_num], pEntry->minInterval, pEntry->maxInterval);
                            }
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

void app_report_handler(void) {

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
        app_forcedReport(APP_ENDPOINT_1, ZCL_CLUSTER_SE_METERING, ZCL_ATTRID_CURRENT_SUMMATION_DELIVERD);
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
