#include "tl_common.h"
#include "zb_api.h"
#include "zcl_include.h"

#include "app_dev_config.h"

#define ID_CONFIG   0x0FED141A
#define TOP_MASK    0xFFFFFFFF

dev_config_t dev_config;

static u16 checksum(const u8 *src_buffer, u8 len) {

    const u16 generator = 0xa010;

    u16 crc = 0xffff;

    len -= 2;

    for (const u8 *ptr = src_buffer; ptr < src_buffer + len; ptr++) {
        crc ^= *ptr;

        for (u8 bit = 8; bit > 0; bit--) {
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
    write_config();
}

void init_config(u8 print) {

    nv_sts_t st = NV_SUCC;

#if UART_PRINTF_MODE && DEBUG_CONFIG
    printf("\r\nOTA mode disabled. MCU boot from address: 0x%x\r\n", FIRMWARE_ADDRESS);
#endif /* UART_PRINTF_MODE */

#if !NV_ENABLE
#error "NV_ENABLE must be enable in "stack_cfg.h" file!"
#endif

    st = nv_flashReadNew(1, NV_MODULE_APP,  NV_ITEM_APP_USER_CFG, sizeof(dev_config_t), (u8*)&dev_config);

    u16 crc = checksum((u8*)&dev_config, sizeof(dev_config_t));

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
    dev_config.crc = checksum((u8*)&(dev_config), sizeof(dev_config_t));
    nv_flashWriteNew(1, NV_MODULE_APP,  NV_ITEM_APP_USER_CFG, sizeof(dev_config_t), (u8*)&dev_config);

#if UART_PRINTF_MODE && DEBUG_CONFIG
    printf("Save config to nv_ram in module NV_MODULE_APP (%d) item NV_ITEM_APP_USER_CFG (%d)\r\n",
            NV_MODULE_APP,  NV_ITEM_APP_USER_CFG);
#endif /* UART_PRINTF_MODE */
}

