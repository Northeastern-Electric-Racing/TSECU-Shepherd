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
#include "stm32h5xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
//extern IWDG_HandleTypeDef hiwdg;
extern SPI_HandleTypeDef hspi2;
extern SPI_HandleTypeDef hspi6;
extern TIM_HandleTypeDef htim3;
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define SPI4_CS_Pin GPIO_PIN_4
#define SPI4_CS_GPIO_Port GPIOE
#define TS_MINUS_SENSE_Pin GPIO_PIN_0
#define TS_MINUS_SENSE_GPIO_Port GPIOC
#define TS_PLUS_SENSE_Pin GPIO_PIN_3
#define TS_PLUS_SENSE_GPIO_Port GPIOC
#define SPI6_CS_Pin GPIO_PIN_0
#define SPI6_CS_GPIO_Port GPIOA
#define SPI2_CS_Pin GPIO_PIN_3
#define SPI2_CS_GPIO_Port GPIOA
#define SPI3_CS_Pin GPIO_PIN_4
#define SPI3_CS_GPIO_Port GPIOA
#define ACC_SENSE_Pin GPIO_PIN_0
#define ACC_SENSE_GPIO_Port GPIOB
#define TSIP_SENSE_Pin GPIO_PIN_1
#define TSIP_SENSE_GPIO_Port GPIOB
#define PHY_IRQ_Pin GPIO_PIN_7
#define PHY_IRQ_GPIO_Port GPIOE
#define PHY_RESET_Pin GPIO_PIN_10
#define PHY_RESET_GPIO_Port GPIOE
#define PHY_GPIO_Pin GPIO_PIN_11
#define PHY_GPIO_GPIO_Port GPIOE
#define TRACEX_TRIG_Pin GPIO_PIN_8
#define TRACEX_TRIG_GPIO_Port GPIOD
#define TRACEX_TRIG_EXTI_IRQn EXTI8_IRQn
#define FAN_PWM0_Pin GPIO_PIN_7
#define FAN_PWM0_GPIO_Port GPIOC
#define SP1_CS_Pin GPIO_PIN_10
#define SP1_CS_GPIO_Port GPIOG
#define FAULT_MCU_Pin GPIO_PIN_15
#define FAULT_MCU_GPIO_Port GPIOG

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
