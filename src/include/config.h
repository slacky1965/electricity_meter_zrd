#ifndef SRC_INCLUDE_CONFIG_H_
#define SRC_INCLUDE_CONFIG_H_

/* must be no more than FLASH_PAGE_SIZE (256) bytes */
typedef struct __attribute__((packed)) {
    u32 id;                     /* ID - ID_CONFIG                   */
    u8  new_ota;                /* new ota flag                     */
    u32 top;                    /* 0x0 .. 0xFFFFFFFF                */
    u32 flash_addr_start;       /* flash page address start         */
    u32 flash_addr_end;         /* flash page address end           */
    u16 measurement_period;     /* measurement period in sec.       */
    u8  device_model;           /* manufacturer of electric meters  */
    u32 device_address;         /* see address on dislpay ID-20109  */
    u32 tariff_multiplier;
    u32 tariff_devisor;
    u8  summation_formatting;
    u16 voltage_multiplier;
    u16 voltage_divisor;
    u16 current_multiplier;
    u16 current_divisor;
    u16 crc;
} em_config_t;

extern em_config_t em_config;

void init_config(u8 print);
void write_config();



#endif /* SRC_INCLUDE_CONFIG_H_ */
