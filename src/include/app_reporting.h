#ifndef SRC_INCLUDE_APP_REPORTING_H_
#define SRC_INCLUDE_APP_REPORTING_H_

typedef struct {
    ev_timer_event_t *timerReportMinEvt;
    ev_timer_event_t *timerReportMaxEvt;
    reportCfgInfo_t  *pEntry;
    u32               time_posted;
} app_reporting_t;

extern app_reporting_t app_reporting[ZCL_REPORTING_TABLE_NUM];

void app_reportAttrTimerStart();
void app_reportNoMinLimit(void);
void app_reporting_init();
void report_handler(void);

#endif /* SRC_INCLUDE_APP_REPORTING_H_ */
