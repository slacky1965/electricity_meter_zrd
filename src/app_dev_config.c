#include "app_main.h"

#define ID_CONFIG   0x0FED141A
#define TOP_MASK    0xFFFFFFFF

dev_config_t dev_config;

static uint16_t checksum(const uint8_t *src_buffer, uint8_t len) {

    const uint16_t generator = 0xa010;

    uint16_t crc = 0xffff;

    len -= 2;

    for (const uint8_t *ptr = src_buffer; ptr < src_buffer + len; ptr++) {
        crc ^= *ptr;

        for (uint8_t bit = 8; bit > 0; bit--) {
            if (crc & 1)
                crc = (crc >> 1) ^ generator;
            else
                crc >>= 1;
        }
    }

    return crc;
}

static void init_default_config() {
    memset(&dev_config, 0, sizeof(dev_config_t));
    dev_config.id = ID_CONFIG;
    dev_config.measurement_period = DEFAULT_MEASUREMENT_PERIOD;
    write_config();
}

void init_config(uint8_t print) {

    nv_sts_t st = NV_SUCC;

    start_message();

#if !NV_ENABLE
#error "NV_ENABLE must be enable in "stack_cfg.h" file!"
#endif

    st = nv_flashReadNew(1, NV_MODULE_APP,  NV_ITEM_APP_USER_CFG, sizeof(dev_config_t), (uint8_t*)&dev_config);

    uint16_t crc = checksum((uint8_t*)&dev_config, sizeof(dev_config_t));

    if (st != NV_SUCC || dev_config.id != ID_CONFIG || crc != dev_config.crc) {
#if UART_PRINTF_MODE && DEBUG_CONFIG
        printf("No saved config! Init.\r\n");
#endif /* UART_PRINTF_MODE */
        init_default_config();
    } else {
#if UART_PRINTF_MODE && DEBUG_CONFIG
        printf("Read config from nv_ram in module NV_MODULE_APP (%d) item NV_ITEM_APP_USER_CFG (%d)\r\n",
                NV_MODULE_APP,  NV_ITEM_APP_USER_CFG);
#endif /* UART_PRINTF_MODE */
    }

}

void write_config() {
    dev_config.crc = checksum((uint8_t*)&(dev_config), sizeof(dev_config_t));
    nv_flashWriteNew(1, NV_MODULE_APP,  NV_ITEM_APP_USER_CFG, sizeof(dev_config_t), (uint8_t*)&dev_config);

#if UART_PRINTF_MODE && DEBUG_CONFIG
    printf("Save config to nv_ram in module NV_MODULE_APP (%d) item NV_ITEM_APP_USER_CFG (%d)\r\n",
            NV_MODULE_APP,  NV_ITEM_APP_USER_CFG);
#endif /* UART_PRINTF_MODE */
}

