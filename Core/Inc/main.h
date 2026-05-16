/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
extern ADC_HandleTypeDef hadc1;

extern CAN_HandleTypeDef hcan1;
extern CAN_HandleTypeDef hcan2;

extern RTC_HandleTypeDef hrtc;

extern SPI_HandleTypeDef hspi3;

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim8;

extern UART_HandleTypeDef huart1;
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
uint32_t tim2_value(void);
uint32_t tim1_value(void);

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LV_BATT_IN_Pin GPIO_PIN_0
#define LV_BATT_IN_GPIO_Port GPIOA
#define PEDAL_IN_1_Pin GPIO_PIN_1
#define PEDAL_IN_1_GPIO_Port GPIOA
#define BRAKE_IN_Pin GPIO_PIN_2
#define BRAKE_IN_GPIO_Port GPIOA
#define PEDAL_IN_2_Pin GPIO_PIN_3
#define PEDAL_IN_2_GPIO_Port GPIOA
#define PRESSURE_IN_2_Pin GPIO_PIN_4
#define PRESSURE_IN_2_GPIO_Port GPIOA
#define PRESSURE_IN_1_Pin GPIO_PIN_5
#define PRESSURE_IN_1_GPIO_Port GPIOA
#define PRESSURE_IN_4_Pin GPIO_PIN_6
#define PRESSURE_IN_4_GPIO_Port GPIOA
#define PRESSURE_IN_3_Pin GPIO_PIN_7
#define PRESSURE_IN_3_GPIO_Port GPIOA
#define GA_IN_1_Pin GPIO_PIN_4
#define GA_IN_1_GPIO_Port GPIOC
#define PWR_METER_IN_Pin GPIO_PIN_5
#define PWR_METER_IN_GPIO_Port GPIOC
#define GA_IN_3_Pin GPIO_PIN_0
#define GA_IN_3_GPIO_Port GPIOB
#define GA_IN_2_Pin GPIO_PIN_1
#define GA_IN_2_GPIO_Port GPIOB
#define ASS_RELAY_Pin GPIO_PIN_2
#define ASS_RELAY_GPIO_Port GPIOB
#define GDI_IN_1_Pin GPIO_PIN_7
#define GDI_IN_1_GPIO_Port GPIOE
#define GDI_IN_2_Pin GPIO_PIN_8
#define GDI_IN_2_GPIO_Port GPIOE
#define GDI_IN_3_Pin GPIO_PIN_9
#define GDI_IN_3_GPIO_Port GPIOE
#define GDI_IN_4_Pin GPIO_PIN_10
#define GDI_IN_4_GPIO_Port GPIOE
#define AMS_IN_Pin GPIO_PIN_11
#define AMS_IN_GPIO_Port GPIOE
#define IMS_IN_Pin GPIO_PIN_12
#define IMS_IN_GPIO_Port GPIOE
#define RTD_SWITCH_IN_Pin GPIO_PIN_13
#define RTD_SWITCH_IN_GPIO_Port GPIOE
#define EMSDC_IN_Pin GPIO_PIN_14
#define EMSDC_IN_GPIO_Port GPIOE
#define IGNITION_IN_Pin GPIO_PIN_15
#define IGNITION_IN_GPIO_Port GPIOE
#define GDO_LOW_5_Pin GPIO_PIN_13
#define GDO_LOW_5_GPIO_Port GPIOB
#define GDO_LOW_3_Pin GPIO_PIN_14
#define GDO_LOW_3_GPIO_Port GPIOB
#define GDO_LOW_6_Pin GPIO_PIN_15
#define GDO_LOW_6_GPIO_Port GPIOB
#define GDO_LOW_4_Pin GPIO_PIN_12
#define GDO_LOW_4_GPIO_Port GPIOD
#define SPEAKER_CTRL_Pin GPIO_PIN_13
#define SPEAKER_CTRL_GPIO_Port GPIOD
#define BRAKE_CTRL_Pin GPIO_PIN_14
#define BRAKE_CTRL_GPIO_Port GPIOD
#define GDO_LOW_1_Pin GPIO_PIN_15
#define GDO_LOW_1_GPIO_Port GPIOD
#define GDO_LOW_2_Pin GPIO_PIN_6
#define GDO_LOW_2_GPIO_Port GPIOC
#define PUSH_PULL_1_Pin GPIO_PIN_7
#define PUSH_PULL_1_GPIO_Port GPIOC
#define PUSH_PULL_2_Pin GPIO_PIN_8
#define PUSH_PULL_2_GPIO_Port GPIOC
#define PUSH_PULL_3_Pin GPIO_PIN_9
#define PUSH_PULL_3_GPIO_Port GPIOC
#define PUSH_PULL_4_Pin GPIO_PIN_8
#define PUSH_PULL_4_GPIO_Port GPIOA
#define RS232_TX_MICRO_Pin GPIO_PIN_9
#define RS232_TX_MICRO_GPIO_Port GPIOA
#define RS232_RX_MICRO_Pin GPIO_PIN_10
#define RS232_RX_MICRO_GPIO_Port GPIOA
#define RS232_EN_Pin GPIO_PIN_11
#define RS232_EN_GPIO_Port GPIOA
#define SD_CS_Pin GPIO_PIN_15
#define SD_CS_GPIO_Port GPIOA
#define SD_CLK_Pin GPIO_PIN_10
#define SD_CLK_GPIO_Port GPIOC
#define SD_MISO_Pin GPIO_PIN_11
#define SD_MISO_GPIO_Port GPIOC
#define SD_MOSI_Pin GPIO_PIN_12
#define SD_MOSI_GPIO_Port GPIOC
#define LED_1_Pin GPIO_PIN_0
#define LED_1_GPIO_Port GPIOE
#define LED_2_Pin GPIO_PIN_1
#define LED_2_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
