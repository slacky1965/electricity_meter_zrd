#include "tl_common.h"

#include "app_uart.h"
#include "app_dev_config.h"
#include "device.h"

uart_data_t rec_buff = {0,  {0, } };
u8  uart_buff[UART_BUFF_SIZE];
u16 uart_head, uart_tail;

u8 available_buff_uart() {
    if (uart_head != uart_tail) {
        return true;
    }
    return false;
}

size_t get_queue_len_buff_uart() {
   return (uart_head - uart_tail) & (UART_BUFF_MASK);
}

static size_t get_freespace_buff_uart() {
    return (sizeof(uart_buff)/sizeof(uart_buff[0]) - get_queue_len_buff_uart());
}

void flush_buff_uart() {
    uart_head = uart_tail = 0;
    memset(uart_buff, 0, UART_BUFF_SIZE);
}

u8 read_byte_from_buff_uart() {
    u8 ch = uart_buff[uart_tail++];
    uart_tail &= UART_BUFF_MASK;
    return ch;

}

static size_t write_bytes_to_buff_uart(u8 *data, size_t len) {

    size_t free_space = get_freespace_buff_uart();
    size_t put_len;

    if (free_space >= len) put_len = len;
    else put_len = free_space;

    for (int i = 0; i < put_len; i++) {
        uart_buff[uart_head++] = data[i];
        uart_head &= UART_BUFF_MASK;
    }

    return put_len;
}


static void app_uartRecvCb() {

    u8 write_len;

    write_len = write_bytes_to_buff_uart(rec_buff.data, rec_buff.dma_len);

    if (write_len != 0) {
        if (write_len == rec_buff.dma_len) {
            rec_buff.dma_len = 0;
        } else if (write_len < rec_buff.dma_len) {
            memcpy(rec_buff.data, rec_buff.data+write_len, rec_buff.dma_len-write_len);
            rec_buff.dma_len -= write_len;
        }
    }
}

void app_uart_init() {

    u32 baudrate = 9600;

    flush_buff_uart();
    drv_uart_pin_set(UART_TX_PD7, UART_RX_PA0);

    switch (dev_config.device_model) {
        case DEVICE_KASKAD_11:
            baudrate = 2400;
            break;
        default:
            baudrate = 9600;
            break;
    }

    drv_uart_init(baudrate, (u8*)&rec_buff, sizeof(uart_data_t), app_uartRecvCb);
}

size_t write_bytes_to_uart(u8 *data, size_t len) {

    if (len > UART_DATA_LEN) len = UART_DATA_LEN;

    if (drv_uart_tx_start(data, len)) {
        return len;
    }
    return 0;
}
