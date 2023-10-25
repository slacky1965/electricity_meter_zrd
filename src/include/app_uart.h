#ifndef SRC_INCLUDE_APP_UART_H_
#define SRC_INCLUDE_APP_UART_H_

#define UART_DATA_LEN  188
#define UART_BUFF_SIZE 2048             /* size ring buffer  */
#define UART_BUFF_MASK UART_BUFF_SIZE-1 /* mask ring buffer  */

#define app_uart_rx_on  app_uart_init


typedef struct {
    uint32_t dma_len;        // dma len must be 4 byte
    uint8_t  data[UART_DATA_LEN];
} uart_data_t;

void app_uart_init(uint32_t baudrate);
size_t write_bytes_to_uart(uint8_t *data, size_t len);
uint8_t read_byte_from_buff_uart();
uint8_t available_buff_uart();
size_t get_queue_len_buff_uart();
void flush_buff_uart();
void app_uart_rx_off();

#endif /* SRC_INCLUDE_APP_UART_H_ */
