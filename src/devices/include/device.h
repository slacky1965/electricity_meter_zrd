#ifndef SRC_INCLUDE_DEVICE_H_
#define SRC_INCLUDE_DEVICE_H_

#include "app_utility.h"

#define PKT_BUFF_MAX_LEN    128         /* max len read from uart   */
#define DATA_MAX_LEN        30          /* do not change!           */
#define MULTIPLIER          1
#define DIVISOR             0

#define RESOURCE_BATTERY    120         /* in month                 */
#define DEFAULT_MEASUREMENT_PERIOD  60  /* in sec                   */

#define SE_ATTR_SN_SIZE     25          /* 0 - len, 1..24 - str     */

typedef u8 (*measure_meter_f)(void);

typedef enum {
    DEVICE_UNDEFINED = 0,
    DEVICE_KASKAD_1_MT,
    DEVICE_KASKAD_11,
    DEVICE_MERCURY_206,
    DEVICE_ENERGOMERA_CE102M,
    DEVICE_MAX,
} device_model_t;

//typedef struct __attribute__((packed)) _divisor_t {
//    u16 power_divisor      :2;     /* 00-0, 01-10, 10-100, 11-1000     */
//    u16 power_sign         :1;     /* 0 - divisor, 1 - multiplier      */
//    u16 voltage_divisor    :2;
//    u16 voltage_sign       :1;
//    u16 current_divisor    :2;
//    u16 current_sign       :1;
//    u16 tariffs_divisor    :2;
//    u16 tariffs_sign       :1;
//    u16 reserve            :4;
//} divisor_t;

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
extern u8 fault_measure_counter;

//u16 get_divisor(const u8 division_factor);
void print_error(pkt_error_t err_no);
u8 set_device_model(device_model_t model);

s32 measure_meterCb(void *arg);
u8 measure_meter_kaskad1mt();
u8 measure_meter_kaskad11();
u8 measure_meter_mercury206();
u8 measure_meter_energomera_ce102m();
//void measure_meter_energomera_ce102();

#endif /* SRC_INCLUDE_DEVICE_H_ */
