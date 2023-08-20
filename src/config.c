#include "tl_common.h"
#include "zb_api.h"
#include "zcl_include.h"
//#include "gp.h"
//#include "app_uart.h"

#include "config.h"
#include "app_ui.h"
#include "electricityMeter.h"

#define ID_CONFIG   0x0FED141A
#define TOP_MASK    0xFFFFFFFF

static u8  default_config = false;
static u32 config_addr_start = 0;
static u32 config_addr_end = 0;

em_config_t em_config;

u8 mcuBootAddrGet(void);

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

static void get_user_data_addr(u8 print) {
#ifdef ZCL_OTA
    if (mcuBootAddrGet()) {
        config_addr_start = BEGIN_USER_DATA1;
        config_addr_end = END_USER_DATA1;
#if UART_PRINTF_MODE
        if (print) printf("OTA mode enabled. MCU boot from address: 0x%x\r\n", BEGIN_USER_DATA2);
#endif /* UART_PRINTF_MODE */
    } else {
        config_addr_start = BEGIN_USER_DATA2;
        config_addr_end = END_USER_DATA2;
#if UART_PRINTF_MODE
        if (print) printf("OTA mode enabled. MCU boot from address: 0x%x\r\n", BEGIN_USER_DATA1);
#endif /* UART_PRINTF_MODE */
    }
#else
    config_addr_start = BEGIN_USER_DATA2;
    config_addr_end = END_USER_DATA2;

#if UART_PRINTF_MODE
    if (print) printf("OTA mode desabled. MCU boot from address: 0x%x\r\n", BEGIN_USER_DATA1);
#endif /* UART_PRINTF_MODE */

#endif
}

static void clear_user_data(u32 flash_addr) {

    u32 flash_data_size = flash_addr + USER_DATA_SIZE;

    while(flash_addr < flash_data_size) {
        flash_erase_sector(flash_addr);
        flash_addr += FLASH_SECTOR_SIZE;
    }
}

static void init_default_config() {
    memset(&em_config, 0, sizeof(em_config_t));
    em_config.id = ID_CONFIG;
    em_config.top = 0;
    em_config.new_ota = 0;
    em_config.flash_addr_start = config_addr_start;
    em_config.flash_addr_end = config_addr_end;
    em_config.measurement_period = DEFAULT_MEASUREMENT_PERIOD;
    default_config = true;
    write_config();
}

static void write_restore_config() {
    em_config.crc = checksum((u8*)&(em_config), sizeof(em_config_t));
    nv_flashWriteNew(1, NV_MODULE_APP,  NV_ITEM_APP_USER_CFG, sizeof(em_config_t), (u8*)&em_config);

#if UART_PRINTF_MODE && DEBUG_LEVEL
    printf("Save restored config to nv_ram in module NV_MODULE_APP (%d) item NV_ITEM_APP_USER_CFG (%d)\r\n",
            NV_MODULE_APP,  NV_ITEM_APP_USER_CFG);
#endif /* UART_PRINTF_MODE */

}

void init_config(u8 print) {
    em_config_t config_curr, config_next, config_restore;
    u8 find_config = false;
    nv_sts_t st = NV_SUCC;

    get_user_data_addr(print);

#if !NV_ENABLE
#error "NV_ENABLE must be enable in "stack_cfg.h" file!"
#endif

    st = nv_flashReadNew(1, NV_MODULE_APP,  NV_ITEM_APP_USER_CFG, sizeof(em_config_t), (u8*)&config_restore);

    u16 crc = checksum((u8*)&config_restore, sizeof(em_config_t));

    if (st != NV_SUCC || config_restore.id != ID_CONFIG || crc != config_restore.crc) {
#if UART_PRINTF_MODE
        printf("No saved config! Init.\r\n");
#endif /* UART_PRINTF_MODE */
        clear_user_data(config_addr_start);
        init_default_config();
        return;

    }

    if (config_restore.new_ota) {
        config_restore.new_ota = false;
        config_restore.flash_addr_start = config_addr_start;
        config_restore.flash_addr_end = config_addr_end;
        memcpy(&em_config, &config_restore, sizeof(em_config_t));
        default_config = true;
        write_config();
        return;
    }

    u32 flash_addr = config_addr_start;

    flash_read_page(flash_addr, sizeof(em_config_t), (u8*)&config_curr);

    if (config_curr.id != ID_CONFIG) {
#if UART_PRINTF_MODE
        printf("No saved config! Init.\r\n");
#endif /* UART_PRINTF_MODE */
        clear_user_data(config_addr_start);
        init_default_config();
        return;
    }

    flash_addr += FLASH_PAGE_SIZE;

    while(flash_addr < config_addr_end) {
        flash_read_page(flash_addr, sizeof(em_config_t), (u8*)&config_next);
        crc = checksum((u8*)&config_next, sizeof(em_config_t));
        if (config_next.id == ID_CONFIG && crc == config_next.crc) {
            if ((config_curr.top + 1) == config_next.top || (config_curr.top == TOP_MASK && config_next.top == 0)) {
                memcpy(&config_curr, &config_next, sizeof(em_config_t));
                flash_addr += FLASH_PAGE_SIZE;
                continue;
            }
            find_config = true;
            break;
        }
        find_config = true;
        break;
    }

    if (find_config) {
        memcpy(&em_config, &config_curr, sizeof(em_config_t));
        em_config.flash_addr_start = flash_addr-FLASH_PAGE_SIZE;
#if UART_PRINTF_MODE
        printf("Read config from flash address - 0x%x\r\n", em_config.flash_addr_start);
#endif /* UART_PRINTF_MODE */
    } else {
#if UART_PRINTF_MODE
        printf("No active saved config! Reinit.\r\n");
#endif /* UART_PRINTF_MODE */
        clear_user_data(config_addr_start);
        init_default_config();
    }
}

void write_config() {
    if (default_config) {
        write_restore_config();
        flash_erase(em_config.flash_addr_start);
        flash_write(em_config.flash_addr_start, sizeof(em_config_t), (u8*)&(em_config));
        default_config = false;
#if UART_PRINTF_MODE && DEBUG_LEVEL
        printf("Save config to flash address - 0x%x\r\n", em_config.flash_addr_start);
#endif /* UART_PRINTF_MODE */
    } else {
        if (!em_config.new_ota) {
            em_config.flash_addr_start += FLASH_PAGE_SIZE;
            if (em_config.flash_addr_start == config_addr_end) {
                em_config.flash_addr_start = config_addr_start;
            }
            if (em_config.flash_addr_start % FLASH_SECTOR_SIZE == 0) {
                flash_erase(em_config.flash_addr_start);
            }
            em_config.top++;
            em_config.top &= TOP_MASK;
            em_config.crc = checksum((u8*)&(em_config), sizeof(em_config_t));
            flash_write(em_config.flash_addr_start, sizeof(em_config_t), (u8*)&(em_config));
#if UART_PRINTF_MODE && DEBUG_LEVEL
            printf("Save config to flash address - 0x%x\r\n", em_config.flash_addr_start);
#endif /* UART_PRINTF_MODE */
        } else {
            write_restore_config();
        }
    }

}

