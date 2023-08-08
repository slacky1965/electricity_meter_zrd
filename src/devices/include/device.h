#ifndef SRC_INCLUDE_DEVICE_H_
#define SRC_INCLUDE_DEVICE_H_

#define PKT_BUFF_MAX_LEN    128         /* max len read from uart          */
#define DATA_MAX_LEN        30          /* do not change!                  */
#define MULTIPLIER          1
#define DIVISOR             0

#define RESOURCE_BATTERY    120         /* in month */
#define DEFAULT_MEASUREMENT_PERIOD  60  /* in sec   */

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

//typedef struct __attribute__((packed)) _meter_t {
//    u32 tariff_1;                      /* last value of tariff #1      */
//    u32 tariff_2;                      /* last value of tariff #2      */
//    u32 tariff_3;                      /* last value of tariff #3      */
//    u32 power;                         /* last value of power          */
//    u16 voltage;                       /* last value of voltage        */
//    u16 amps;                          /* last value of ampere         */
//    u8  serial_number[DATA_MAX_LEN+1]; /* serial number                */
//    u8  serial_number_len;             /* lenght of serial number      */
//    u8  date_release[DATA_MAX_LEN+1];  /* date of release              */
//    u8  date_release_len;              /* lenght of release date       */
//    u8  battery_level;
//    void   (*measure_meter) (void);
//    void   (*get_serial_number_data) (void);
//    void   (*get_date_release_data) (void);
//} meter_t;

extern u8 release_month;
extern u8 release_year;
extern u8 new_start;
extern pkt_error_t pkt_error_no;
extern measure_meter_f measure_meter;
extern u8 device_model[DEVICE_MAX][32];

u16 get_divisor(const u8 division_factor);
u32 from24to32(const u8 *str);
//u8 set_device_type(device_type_t type);
void print_error(pkt_error_t err_no);
u8 set_device_model(device_model_t model);

u8 measure_meter_kaskad1mt();
//void get_serial_number_data_kaskad1mt();
//void get_date_release_data_kaskad1mt();
//void measure_meter_kaskad11();
//void get_serial_number_data_kaskad11();
//void get_date_release_data_kaskad11();
//void measure_meter_mercury206();
//void get_serial_number_data_mercury206();
//void get_date_release_data_mercury206();
//void measure_meter_energomera_ce102m();
//void get_serial_number_data_energomera_ce102m();
//void get_date_release_data_energomera_ce102m();
//void measure_meter_energomera_ce102();

#endif /* SRC_INCLUDE_DEVICE_H_ */
