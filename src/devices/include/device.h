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

typedef uint8_t (*measure_meter_f)(void);

typedef enum {
    DEVICE_UNDEFINED = 0,
    DEVICE_KASKAD_1_MT,
    DEVICE_KASKAD_11,
    DEVICE_MERCURY_206,
    DEVICE_ENERGOMERA_CE102M,
    DEVICE_NEVA_MT124,
    DEVICE_NARTIS_100,
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
    PKT_ERR_DEST_ADDRESS,
    PKT_ERR_SRC_ADDRESS,
    PKT_ERR_RESPONSE,
    PKT_ERR_CRC,
    PKT_ERR_UART,
    PKT_ERR_TYPE,
    PKT_ERR_SEGMENTATION,
} pkt_error_t;

extern uint16_t attr_len;
extern uint8_t attr_data[8];
extern uint8_t new_start;
extern pkt_error_t pkt_error_no;
extern measure_meter_f measure_meter;
extern uint8_t device_model[DEVICE_MAX][32];
extern uint8_t fault_measure_flag;
extern ev_timer_event_t *timerFaultMeasurementEvt;

//uint16_t get_divisor(const uint8_t division_factor);
void print_error(pkt_error_t err_no);
void print_package(uint8_t *head, uint8_t *buff, size_t len);
uint8_t set_device_model(device_model_t model);

int32_t measure_meterCb(void *arg);
int32_t fault_measure_meterCb(void *arg);
uint8_t measure_meter_kaskad_1_mt();
uint8_t measure_meter_kaskad_11();
uint8_t measure_meter_mercury_206();
uint8_t measure_meter_energomera_ce102m();
uint8_t measure_meter_neva_mt124();
void nartis100_init();
uint8_t measure_meter_nartis_100();

#endif /* SRC_INCLUDE_DEVICE_H_ */
