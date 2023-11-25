#pragma once

/* Enable C linkage for C++ Compilers: */
#if defined(__cplusplus)
extern "C" {
#endif

/************************* Configure KEY GPIO ***************************************/
#define BUTTON                  GPIO_PB6
#define PB6_INPUT_ENABLE        ON
#define PB6_DATA_OUT            OFF
#define PB6_OUTPUT_ENABLE       OFF
#define PB6_FUNC                AS_GPIO
#define PULL_WAKEUP_SRC_PB6     PM_PIN_PULLUP_1M

/**************************** Configure LED ******************************************/

#define LED_STATUS              GPIO_PC0
#define PC0_FUNC                AS_GPIO
#define PC0_OUTPUT_ENABLE       ON
#define PC0_INPUT_ENABLE        OFF

#define LED_POWER               GPIO_PA1
#define PA1_FUNC                AS_GPIO
#define PA1_OUTPUT_ENABLE       ON
#define PA1_INPUT_ENABLE        OFF

#define LED_PERMIT              LED_STATUS


/**************************** Configure UART ***************************************
*    UART_TX_PA2 = GPIO_PA2,
*    UART_TX_PB1 = GPIO_PB1,
*    UART_TX_PC2 = GPIO_PC2,
*    UART_TX_PD0 = GPIO_PD0,
*    UART_TX_PD3 = GPIO_PD3,
*    UART_TX_PD7 = GPIO_PD7,
*    UART_RX_PA0 = GPIO_PA0,
*    UART_RX_PB0 = GPIO_PB0,
*    UART_RX_PB7 = GPIO_PB7,
*    UART_RX_PC3 = GPIO_PC3,
*    UART_RX_PC5 = GPIO_PC5,
*    UART_RX_PD6 = GPIO_PD6,
*/

/**************************** UART for optoport ***********************************/
#define BAUDRATE_UART           9600
#define GPIO_UART_TX            UART_TX_PD7
#define GPIO_UART_RX            UART_RX_PA0

#if UART_PRINTF_MODE
#define DEBUG_INFO_TX_PIN       GPIO_PB1    //printf
#define BAUDRATE                115200
#endif

/************************* Configure Temperature ***********************************/
#define GPIO_TEMP               GPIO_PC3
#define PC3_FUNC                AS_GPIO
//#define PULL_WAKEUP_SRC_PC3     PM_PIN_PULLUP_1M

/* Disable C linkage for C++ Compilers: */
#if defined(__cplusplus)
}
#endif
