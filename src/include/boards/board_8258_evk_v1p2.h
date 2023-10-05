/********************************************************************************************************
 * @file    board_8258_evk_v1p2.h
 *
 * @brief   This is the header file for board_8258_evk_v1p2
 *
 * @author  Zigbee Group
 * @date    2021
 *
 * @par     Copyright (c) 2021, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *			All rights reserved.
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/

#pragma once

/* Enable C linkage for C++ Compilers: */
#if defined(__cplusplus)
extern "C" {
#endif

// BUTTON
//#define BUTTON1               		GPIO_PB2
//#define PB2_FUNC			  		AS_GPIO
//#define PB2_OUTPUT_ENABLE	  		0
//#define PB2_INPUT_ENABLE	  		1
//#define	PULL_WAKEUP_SRC_PB2	  		PM_PIN_PULLDOWN_100K
//
//#define BUTTON2               		GPIO_PB3
//#define PB3_FUNC			  		AS_GPIO
//#define PB3_OUTPUT_ENABLE	  		0
//#define PB3_INPUT_ENABLE	  		1
//#define	PULL_WAKEUP_SRC_PB3	  		PM_PIN_PULLDOWN_100K
//
//
//#define PB4_FUNC			  		AS_GPIO
//#define PB4_OUTPUT_ENABLE	  		0
//#define PB4_INPUT_ENABLE	  		1
//#define	PULL_WAKEUP_SRC_PB4	  		PM_PIN_PULLUP_10K
//
//#define PB5_FUNC			  		AS_GPIO
//#define PB5_OUTPUT_ENABLE	  		0
//#define PB5_INPUT_ENABLE	  		1
//#define	PULL_WAKEUP_SRC_PB5	  		PM_PIN_PULLUP_10K

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


//// LED
//#define LED1     					GPIO_PD5
//#define PD5_FUNC					AS_GPIO
//#define PD5_OUTPUT_ENABLE			1
//#define PD5_INPUT_ENABLE			0
//
//#define LED3     					GPIO_PD3
//#define PD3_FUNC					AS_GPIO
//#define PD3_OUTPUT_ENABLE			1
//#define PD3_INPUT_ENABLE			0
//
//#define	PM_WAKEUP_LEVEL		  		PM_WAKEUP_LEVEL_HIGH

#define LED_STATUS              GPIO_PD3
#define PD3_FUNC                AS_GPIO
#define PD3_OUTPUT_ENABLE       ON
#define PD3_INPUT_ENABLE        OFF

#define LED_POWER               GPIO_PD5    // PD4
#define PD5_FUNC                AS_GPIO
#define PD5_OUTPUT_ENABLE       ON
#define PD5_INPUT_ENABLE        OFF


// UART
#if ZBHCI_UART
	#error please configurate uart PIN!!!!!!
#endif

// DEBUG
//#if UART_PRINTF_MODE
//	#define	DEBUG_INFO_TX_PIN	    GPIO_PD0//print
//#endif

#if UART_PRINTF_MODE
#define DEBUG_INFO_TX_PIN       UART_TX_PB1//print
#define BAUDRATE                115200
#define PB1_DATA_OUT            ON
#define PB1_OUTPUT_ENABLE       ON
#define PB1_INPUT_ENABLE        OFF
#define PULL_WAKEUP_SRC_PB1     PM_PIN_PULLUP_1M
#define PB1_FUNC                AS_GPIO

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


//enum{
//	VK_SW1 = 0x01,
//	VK_SW2 = 0x02,
//	VK_SW3 = 0x03,
//	VK_SW4 = 0x04
//};
//
/*//#define	KB_MAP_NORMAL	{\
//		{VK_SW1, VK_SW3}, \
//		{VK_SW2, VK_SW4}, } */
//
//#define	KB_MAP_NUM		KB_MAP_NORMAL
//#define	KB_MAP_FN		KB_MAP_NORMAL
//
//#define KB_DRIVE_PINS  {GPIO_PB2,  GPIO_PB3}
//#define KB_SCAN_PINS   {GPIO_PB4,  GPIO_PB5}
//
//
//#define	KB_LINE_MODE		0
//#define	KB_LINE_HIGH_VALID	0


/* Disable C linkage for C++ Compilers: */
#if defined(__cplusplus)
}
#endif
