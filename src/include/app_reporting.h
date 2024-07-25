#ifndef SRC_INCLUDE_APP_REPORTING_H_
#define SRC_INCLUDE_APP_REPORTING_H_

typedef struct {
    ev_timer_event_t *timerReportMinEvt;
    ev_timer_event_t *timerReportMaxEvt;
    reportCfgInfo_t  *pEntry;
    uint32_t          time_posted;
} app_reporting_t;

extern app_reporting_t app_reporting[ZCL_REPORTING_TABLE_NUM];

void app_forcedReport(uint8_t endpoint, uint16_t claster_id, uint16_t attr_id);
void app_all_forceReporting();
void app_reporting_init();
void report_handler(void);

#endif /* SRC_INCLUDE_APP_REPORTING_H_ */
