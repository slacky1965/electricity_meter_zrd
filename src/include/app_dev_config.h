#ifndef SRC_INCLUDE_APP_DEV_CONFIG_H_
#define SRC_INCLUDE_APP_DEV_CONFIG_H_

/* must be no more than FLASH_PAGE_SIZE (256) bytes */
typedef struct __attribute__((packed)) {
    uint32_t id;                    /* ID - ID_CONFIG                       */
    uint16_t measurement_period;    /* measurement period in sec.           */
    uint8_t  device_model;          /* manufacturer of electric meters      */
    uint32_t device_address;        /* see address on dislpay ID-20109      */
    uint8_t  device_password[8];    /* password for read electricity meter  */
    uint32_t tariff_multiplier;
    uint32_t tariff_devisor;
    uint8_t  summation_formatting;
    uint16_t voltage_multiplier;
    uint16_t voltage_divisor;
    uint16_t current_multiplier;
    uint16_t current_divisor;
    uint16_t crc;
} dev_config_t;

extern dev_config_t dev_config;

void init_config(uint8_t print);
void write_config();


#endif /* SRC_INCLUDE_APP_DEV_CONFIG_H_ */
