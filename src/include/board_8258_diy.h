#ifndef SRC_INCLUDE_BOARD_8258_DIY_H_
#define SRC_INCLUDE_BOARD_8258_DIY_H_

/************************* Configure KEY GPIO ***************************************/
#define BUTTON                  GPIO_PB5
#define PB5_INPUT_ENABLE        ON
#define PB5_DATA_OUT            OFF
#define PB5_OUTPUT_ENABLE       OFF
#define PB5_FUNC                AS_GPIO
#define PULL_WAKEUP_SRC_PB6     PM_PIN_PULLUP_1M

#define BUTTON_2W               GPIO_PB2
#define PB2_FUNC                AS_GPIO
#define PB2_OUTPUT_ENABLE       OFF
#define PB2_INPUT_ENABLE        ON
#define PULL_WAKEUP_SRC_PB2     PM_PIN_PULLDOWN_100K


/**************************** Configure LED ******************************************/

#define LED_STATUS              GPIO_PD3
#define PD3_FUNC                AS_GPIO
#define PD3_OUTPUT_ENABLE       ON
#define PD3_INPUT_ENABLE        OFF

#define LED_POWER               GPIO_PD5    // PD4
#define PD5_FUNC                AS_GPIO
#define PD5_OUTPUT_ENABLE       ON
#define PD5_INPUT_ENABLE        OFF

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
#define UART_BAUD_RATE          9600
#define UART_TX_GPIO            UART_TX_PD7
#define UART_RX_GPIO            UART_RX_PA0

#if UART_PRINTF_MODE
//#define DEBUG_INFO_TX_PIN       UART_TX_PB1//print
//#define BAUDRATE                115200
//#define PB1_DATA_OUT            ON
//#define PB1_OUTPUT_ENABLE       ON
//#define PB1_INPUT_ENABLE        OFF
//#define PULL_WAKEUP_SRC_PB1     PM_PIN_PULLUP_1M
//#define PB1_FUNC                AS_GPIO

#define DEBUG_INFO_TX_PIN       GPIO_PA4//print
#define BAUDRATE                115200
#define PA4_DATA_OUT            ON
#define PA4_OUTPUT_ENABLE       ON
#define PA4_INPUT_ENABLE        OFF
#define PULL_WAKEUP_SRC_PA4     PM_PIN_PULLUP_1M
#define PA4_FUNC                AS_GPIO

#endif /* UART_PRINTF_MODE */


/************************* For 512K Flash only ***************************************/
/* Flash map:
  0x00000 Old Firmware bin
  0x34000 NV_1
  0x40000 OTA New bin storage Area
  0x76000 MAC address
  0x77000 C_Cfg_Info
  0x78000 U_Cfg_Info
  0x7A000 NV_2
  0x80000 End Flash
 */
#define USER_DATA_SIZE          0x34000
#define BEGIN_USER_DATA1        0x00000
#define END_USER_DATA1          (BEGIN_USER_DATA1 + USER_DATA_SIZE)
#define BEGIN_USER_DATA2        0x40000
#define END_USER_DATA2          (BEGIN_USER_DATA2 + USER_DATA_SIZE)
#define GEN_USER_CFG_DATA       END_USER_DATA2



#endif /* SRC_INCLUDE_BOARD_8258_DIY_H_ */
