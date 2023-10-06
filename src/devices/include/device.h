#ifndef SRC_INCLUDE_DEVICE_H_
#define SRC_INCLUDE_DEVICE_H_

#include "app_utility.h"

#define PKT_BUFF_MAX_LEN    128         /* max len read from uart   */
#define DATA_MAX_LEN        30          /* do not change!           */
#define DEVICE_NAME_LEN     48

#define RESOURCE_BATTERY    120         /* in month                 */
#define DEFAULT_MEASUREMENT_PERIOD  60  /* in sec                   */
#define FAULT_MEASUREMENT_PERIOD    10  /* in sec                   */

#define SE_ATTR_SN_SIZE     25          /* 0 - len, 1..24 - str     */

typedef u8 (*measure_meter_f)(void);

typedef enum {
    DEVICE_UNDEFINED = 0,
    DEVICE_KASKAD_1_MT,
    DEVICE_KASKAD_11,
    DEVICE_MERCURY_206,
    DEVICE_ENERGOMERA_CE102M,
    DEVICE_NEVA_MT124,
    DEVICE_MAX,
} device_model_t;

typedef enum _pkt_error_t {
    PKT_OK  = 0,
    PKT_ERR_NO_PKT,
    PKT_ERR_TIMEOUT,
    PKT_ERR_UNKNOWN_FORMAT,
    PKT_ERR_DIFFERENT_COMMAND,
    PKT_ERR_INCOMPLETE,
    PKT_ERR_UNSTUFFING,
    PKT_ERR_ADDRESS,
    PKT_ERR_RESPONSE,
    PKT_ERR_CRC,
    PKT_ERR_UART
} pkt_error_t;

u16 attr_len;
u8 attr_data[8];
extern u8 new_start;
extern pkt_error_t pkt_error_no;
extern measure_meter_f measure_meter;
extern u8 device_model[DEVICE_MAX][32];
extern u8 fault_measure_flag;
extern ev_timer_event_t *timerFaultMeasurementEvt;

//u16 get_divisor(const u8 division_factor);
void print_error(pkt_error_t err_no);
void print_package(u8 *head, u8 *buff, size_t len);
u8 set_device_model(device_model_t model);

s32 measure_meterCb(void *arg);
s32 fault_measure_meterCb(void *arg);
u8 measure_meter_kaskad_1_mt();
u8 measure_meter_kaskad_11();
u8 measure_meter_mercury_206();
u8 measure_meter_energomera_ce102m();
u8 measure_meter_neva_mt124();


#endif /* SRC_INCLUDE_DEVICE_H_ */
