#include "tl_common.h"
#include "zcl_include.h"

#include "app_endpoint_cfg.h"
#include "app_temperature.h"
#include "app_cfg.h"
#define DIRECT_MODE_INPUT   one_wire_mode_input
#define DIRECT_MODE_OUTPUT  one_wire_mode_output
#define DIRECT_WRITE_HIGH() one_wire_gpio_write(1)
#define DIRECT_WRITE_LOW()  one_wire_gpio_write(0)
#define DIRECT_READ         one_wire_gpio_read

typedef u8 scratch_pad_t[9];

static void one_wire_mode_output() {
    drv_gpio_output_en(GPIO_TEMP, true);
}

static void one_wire_mode_input() {
    drv_gpio_output_en(GPIO_TEMP, false);
}

static u8 one_wire_gpio_read() {
    return drv_gpio_read(GPIO_TEMP);
}

static void one_wire_gpio_write(bool v) {
    drv_gpio_write(GPIO_TEMP, v);
}

static u8 ds18b20_reset(void) {
    u8 r;
    u8 retries = 125;

    DIRECT_MODE_INPUT();
    // wait until the wire is high... just in case
    do {
        if (--retries == 0) return 0;
        sleep_us(2);
    } while (!DIRECT_READ());

    DIRECT_MODE_OUTPUT();  // drive output low
    DIRECT_WRITE_LOW();
    sleep_us(480);
    DIRECT_MODE_INPUT();   // allow it to float
    sleep_us(70);
    r = !DIRECT_READ();
    sleep_us(410);
    return r;
}

u8 ds18b20_read_bit(void)
{
    u8 r;

    DIRECT_MODE_OUTPUT();
    DIRECT_WRITE_LOW();
    sleep_us(3);
    DIRECT_MODE_INPUT();   // let pin float, pull up will raise
    sleep_us(10);
    r = DIRECT_READ();
    sleep_us(53);
    return r;
}

u8 ds18b20_read_byte() {
    u8 bitMask;
    u8 r = 0;

    for (bitMask = 0x01; bitMask; bitMask <<= 1) {
    if ( ds18b20_read_bit()) r |= bitMask;
    }
    return r;
}

static void ds18b20_write_bit(u8 v)
{

    if (v & 1) {
        DIRECT_MODE_OUTPUT();  // drive output low
        DIRECT_WRITE_LOW();
        sleep_us(10);
        DIRECT_WRITE_HIGH();   // drive output high
        sleep_us(55);
    } else {
        DIRECT_MODE_OUTPUT();  // drive output low
        DIRECT_WRITE_LOW();
        sleep_us(65);
        DIRECT_WRITE_HIGH();   // drive output high
        sleep_us(5);
    }
}

static void ds18b20_write_byte(u8 v) {
    u8 bitMask;

    for (bitMask = 0x01; bitMask; bitMask <<= 1) {
        ds18b20_write_bit( (bitMask & v)?1:0);
    }
}

static void ds18b20_request_temp(void) {
    ds18b20_reset();
    ds18b20_write_byte(SKIPROM);
    ds18b20_write_byte(STARTCONVO);
}

static void ds18b20_read_scratch_pad(u8 *scratch_pad, u8 fields) {
    ds18b20_reset();
    ds18b20_write_byte(SKIPROM);
    ds18b20_write_byte(READSCRATCH);

    for(u8 i=0; i < fields; i++) {
        scratch_pad[i] = ds18b20_read_byte();
    }
}

static float ds18b20_get_temp() {

    if (!ds18b20_reset()) {
        return 0;
    }

    ds18b20_request_temp();

    scratch_pad_t scratch_pad;
    ds18b20_read_scratch_pad(scratch_pad, 2);
    s16 rawTemperature = (((s16)scratch_pad[TEMP_MSB]) << 8) | scratch_pad[TEMP_LSB];
    float temp = 0.0625 * rawTemperature;
    return temp;
}

void ds18b20_init() {
    drv_gpio_input_en(GPIO_TEMP, true);
    drv_gpio_output_en(GPIO_TEMP, true);
    drv_gpio_up_down_resistor(GPIO_TEMP, PM_PIN_PULLUP_10K);
    drv_gpio_irq_dis(GPIO_TEMP);

    DIRECT_WRITE_HIGH();
}

s32 getTemperatureCb(void *arg) {

    s16 temperature = ds18b20_get_temp();

    zcl_setAttrVal(APP_ENDPOINT_1, ZCL_CLUSTER_GEN_DEVICE_TEMP_CONFIG, ZCL_ATTRID_DEV_TEMP_CURR_TEMP, (u8*)&temperature);

    return 0;
}

///**********************************************************************
// *  For internal temperature sensor. Only TLSR8258. Not correct worked!!!
// */
//
//#if defined(MCU_CORE_8258)
//
//void adc_temp_init() {
//
//    drv_adc_init();
//    adc_set_resolution(ADC_MISC_CHN, RES12/*RES14*/);
//    adc_set_vref_chn_misc(ADC_VREF_1P2V);
//    adc_set_ain_channel_differential_mode(ADC_MISC_CHN, TEMSENSORP, TEMSENSORN);
//    adc_set_vref_vbat_divider(ADC_VBAT_DIVIDER_OFF);//set Vbat divider select,
//    adc_set_ain_pre_scaler(ADC_PRESCALER_1F8);
//
//    drv_adc_enable(ON);
//
//    //enable temperature sensor
////    analog_write(0x00, (analog_read(0x00)&0xef));
//    analog_write(0x07, analog_read(0x07)&0xEF);
//}
//
//
//s16 adc_temp_result() {
//    s16 adc_temp_value;
//    u32 adc_data;
//
//    adc_data = drv_get_adc_data();
//
//    if (adc_data > 255) adc_data = 255;
//
//    //printf("adc_data: %d\r\n", adc_data);
//
//    adc_temp_value = ((adc_data*100 - 130*100)/54 - 40); // * 100 ;
////    adc_temp_value |= (adc_data*100 - 130*100)%54;
//
//    return adc_temp_value;
//}
//
//#endif
//
