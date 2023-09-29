#ifndef SRC_INCLUDE_APP_UART_H_
#define SRC_INCLUDE_APP_UART_H_

#define UART_DATA_LEN  188
#define UART_BUFF_SIZE 512              /* size ring buffer  */
#define UART_BUFF_MASK UART_BUFF_SIZE-1 /* mask ring buffer  */

#define app_uart_rx_on  app_uart_init


typedef struct {
    u32 dma_len;        // dma len must be 4 byte
    u8  data[UART_DATA_LEN];
} uart_data_t;

void app_uart_init();
size_t write_bytes_to_uart(u8 *data, size_t len);
u8 read_byte_from_buff_uart();
u8 available_buff_uart();
size_t get_queue_len_buff_uart();
void flush_buff_uart();
void app_uart_rx_off();

#endif /* SRC_INCLUDE_APP_UART_H_ */
