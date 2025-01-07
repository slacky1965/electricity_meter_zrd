#ifndef SRC_INCLUDE_APP_DEV_CONFIG_H_
#define SRC_INCLUDE_APP_DEV_CONFIG_H_

typedef struct __attribute__((packed)) {
    uint8_t size;
    uint8_t data[16];
} m_password_t;

/* must be no more than FLASH_PAGE_SIZE (256) bytes */
typedef struct __attribute__((packed)) {
    uint32_t        id;                     /* ID - ID_CONFIG                                       */
    uint16_t        measurement_period;     /* measurement period in sec.                           */
    uint8_t         device_model;           /* manufacturer of electric meters                      */
    uint32_t        device_address;         /* see address on dislpay ID-20109                      */
    m_password_t    device_password;        /* password for electricity meter - "[size]12345678"    */
    uint16_t        crc;
} dev_config_t;

extern dev_config_t dev_config;

void init_config(uint8_t print);
void write_config();


#endif /* SRC_INCLUDE_APP_DEV_CONFIG_H_ */
