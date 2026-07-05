/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file         stm32h5xx_hal_msp.c
  * @brief        This file provides code for the MSP Initialization
  *               and de-Initialization codes.
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

/* Includes ------------------------------------------------------------------*/
#include "main.h"
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */
extern DMA_HandleTypeDef handle_GPDMA1_Channel4;

extern DMA_HandleTypeDef handle_GPDMA1_Channel5;

extern DMA_HandleTypeDef handle_GPDMA1_Channel0;

extern DMA_HandleTypeDef handle_GPDMA1_Channel1;

extern DMA_HandleTypeDef handle_GPDMA1_Channel2;

extern DMA_HandleTypeDef handle_GPDMA1_Channel3;

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN Define */

/* USER CODE END Define */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN Macro */

/* USER CODE END Macro */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* External functions --------------------------------------------------------*/
/* USER CODE BEGIN ExternalFunctions */

/* USER CODE END ExternalFunctions */

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);
                    /**
  * Initializes the Global MSP.
  */
void HAL_MspInit(void)
{

  /* USER CODE BEGIN MspInit 0 */

  /* USER CODE END MspInit 0 */

  /* System interrupt init*/

  /* Disable the internal Pull-Up in Dead Battery pins of UCPD peripheral */
  HAL_PWREx_DisableUCPDDeadBattery();

  /* USER CODE BEGIN MspInit 1 */

  /* USER CODE END MspInit 1 */
}

/**
  * @brief ETH MSP Initialization
  * This function configures the hardware resources used in this example
  * @param heth: ETH handle pointer
  * @retval None
  */
void HAL_ETH_MspInit(ETH_HandleTypeDef* heth)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(heth->Instance==ETH)
  {
    /* USER CODE BEGIN ETH_MspInit 0 */

    /* USER CODE END ETH_MspInit 0 */
    /* Peripheral clock enable */
    __HAL_RCC_ETH_CLK_ENABLE();
    __HAL_RCC_ETHTX_CLK_ENABLE();
    __HAL_RCC_ETHRX_CLK_ENABLE();

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    /**ETH GPIO Configuration
    PC1     ------> ETH_MDC
    PA1     ------> ETH_REF_CLK
    PA2     ------> ETH_MDIO
    PA7     ------> ETH_CRS_DV
    PC4     ------> ETH_RXD0
    PC5     ------> ETH_RXD1
    PB12     ------> ETH_TXD0
    PB15     ------> ETH_TXD1
    PG11     ------> ETH_TX_EN
    */
    GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_4|GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    /* ETH interrupt Init */
    HAL_NVIC_SetPriority(ETH_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(ETH_IRQn);
    /* USER CODE BEGIN ETH_MspInit 1 */

    /* USER CODE END ETH_MspInit 1 */

  }

}

/**
  * @brief ETH MSP De-Initialization
  * This function freeze the hardware resources used in this example
  * @param heth: ETH handle pointer
  * @retval None
  */
void HAL_ETH_MspDeInit(ETH_HandleTypeDef* heth)
{
  if(heth->Instance==ETH)
  {
    /* USER CODE BEGIN ETH_MspDeInit 0 */

    /* USER CODE END ETH_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_ETH_CLK_DISABLE();
    __HAL_RCC_ETHTX_CLK_DISABLE();
    __HAL_RCC_ETHRX_CLK_DISABLE();

    /**ETH GPIO Configuration
    PC1     ------> ETH_MDC
    PA1     ------> ETH_REF_CLK
    PA2     ------> ETH_MDIO
    PA7     ------> ETH_CRS_DV
    PC4     ------> ETH_RXD0
    PC5     ------> ETH_RXD1
    PB12     ------> ETH_TXD0
    PB15     ------> ETH_TXD1
    PG11     ------> ETH_TX_EN
    */
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_1|GPIO_PIN_4|GPIO_PIN_5);

    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_7);

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_12|GPIO_PIN_15);

    HAL_GPIO_DeInit(GPIOG, GPIO_PIN_11);

    /* ETH interrupt DeInit */
    HAL_NVIC_DisableIRQ(ETH_IRQn);
    /* USER CODE BEGIN ETH_MspDeInit 1 */

    /* USER CODE END ETH_MspDeInit 1 */
  }

}

/**
  * @brief FDCAN MSP Initialization
  * This function configures the hardware resources used in this example
  * @param hfdcan: FDCAN handle pointer
  * @retval None
  */
void HAL_FDCAN_MspInit(FDCAN_HandleTypeDef* hfdcan)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(hfdcan->Instance==FDCAN2)
  {
    /* USER CODE BEGIN FDCAN2_MspInit 0 */

    /* USER CODE END FDCAN2_MspInit 0 */
    /* Peripheral clock enable */
    __HAL_RCC_FDCAN_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    /**FDCAN2 GPIO Configuration
    PB13     ------> FDCAN2_TX
    PD9     ------> FDCAN2_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF9_FDCAN2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF9_FDCAN2;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* FDCAN2 interrupt Init */
    HAL_NVIC_SetPriority(FDCAN2_IT0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(FDCAN2_IT0_IRQn);
    HAL_NVIC_SetPriority(FDCAN2_IT1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(FDCAN2_IT1_IRQn);
    /* USER CODE BEGIN FDCAN2_MspInit 1 */

    /* USER CODE END FDCAN2_MspInit 1 */

  }

}

/**
  * @brief FDCAN MSP De-Initialization
  * This function freeze the hardware resources used in this example
  * @param hfdcan: FDCAN handle pointer
  * @retval None
  */
void HAL_FDCAN_MspDeInit(FDCAN_HandleTypeDef* hfdcan)
{
  if(hfdcan->Instance==FDCAN2)
  {
    /* USER CODE BEGIN FDCAN2_MspDeInit 0 */

    /* USER CODE END FDCAN2_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_FDCAN_CLK_DISABLE();

    /**FDCAN2 GPIO Configuration
    PB13     ------> FDCAN2_TX
    PD9     ------> FDCAN2_RX
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_13);

    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_9);

    /* FDCAN2 interrupt DeInit */
    HAL_NVIC_DisableIRQ(FDCAN2_IT0_IRQn);
    HAL_NVIC_DisableIRQ(FDCAN2_IT1_IRQn);
    /* USER CODE BEGIN FDCAN2_MspDeInit 1 */

    /* USER CODE END FDCAN2_MspDeInit 1 */
  }

}

/**
  * @brief I2C MSP Initialization
  * This function configures the hardware resources used in this example
  * @param hi2c: I2C handle pointer
  * @retval None
  */
void HAL_I2C_MspInit(I2C_HandleTypeDef* hi2c)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
  if(hi2c->Instance==I2C1)
  {
    /* USER CODE BEGIN I2C1_MspInit 0 */

    /* USER CODE END I2C1_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2C1;
    PeriphClkInitStruct.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**I2C1 GPIO Configuration
    PB6     ------> I2C1_SCL
    PB7     ------> I2C1_SDA
    */
    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* Peripheral clock enable */
    __HAL_RCC_I2C1_CLK_ENABLE();
    /* USER CODE BEGIN I2C1_MspInit 1 */

    /* USER CODE END I2C1_MspInit 1 */
  }
  else if(hi2c->Instance==I2C2)
  {
    /* USER CODE BEGIN I2C2_MspInit 0 */

    /* USER CODE END I2C2_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2C2;
    PeriphClkInitStruct.I2c2ClockSelection = RCC_I2C2CLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_RCC_GPIOF_CLK_ENABLE();
    /**I2C2 GPIO Configuration
    PF0     ------> I2C2_SDA
    PF1     ------> I2C2_SCL
    */
    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C2;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    /* Peripheral clock enable */
    __HAL_RCC_I2C2_CLK_ENABLE();
    /* USER CODE BEGIN I2C2_MspInit 1 */

    /* USER CODE END I2C2_MspInit 1 */
  }

}

/**
  * @brief I2C MSP De-Initialization
  * This function freeze the hardware resources used in this example
  * @param hi2c: I2C handle pointer
  * @retval None
  */
void HAL_I2C_MspDeInit(I2C_HandleTypeDef* hi2c)
{
  if(hi2c->Instance==I2C1)
  {
    /* USER CODE BEGIN I2C1_MspDeInit 0 */

    /* USER CODE END I2C1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_I2C1_CLK_DISABLE();

    /**I2C1 GPIO Configuration
    PB6     ------> I2C1_SCL
    PB7     ------> I2C1_SDA
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6);

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_7);

    /* USER CODE BEGIN I2C1_MspDeInit 1 */

    /* USER CODE END I2C1_MspDeInit 1 */
  }
  else if(hi2c->Instance==I2C2)
  {
    /* USER CODE BEGIN I2C2_MspDeInit 0 */

    /* USER CODE END I2C2_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_I2C2_CLK_DISABLE();

    /**I2C2 GPIO Configuration
    PF0     ------> I2C2_SDA
    PF1     ------> I2C2_SCL
    */
    HAL_GPIO_DeInit(GPIOF, GPIO_PIN_0);

    HAL_GPIO_DeInit(GPIOF, GPIO_PIN_1);

    /* USER CODE BEGIN I2C2_MspDeInit 1 */

    /* USER CODE END I2C2_MspDeInit 1 */
  }

}

/**
  * @brief UART MSP Initialization
  * This function configures the hardware resources used in this example
  * @param huart: UART handle pointer
  * @retval None
  */
void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
  if(huart->Instance==LPUART1)
  {
    /* USER CODE BEGIN LPUART1_MspInit 0 */

    /* USER CODE END LPUART1_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LPUART1;
    PeriphClkInitStruct.Lpuart1ClockSelection = RCC_LPUART1CLKSOURCE_PCLK3;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
      Error_Handler();
    }

    /* Peripheral clock enable */
    __HAL_RCC_LPUART1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**LPUART1 GPIO Configuration
    PA9     ------> LPUART1_TX
    PA10     ------> LPUART1_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF3_LPUART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USER CODE BEGIN LPUART1_MspInit 1 */

    /* USER CODE END LPUART1_MspInit 1 */
  }
  else if(huart->Instance==UART4)
  {
    /* USER CODE BEGIN UART4_MspInit 0 */

    /* USER CODE END UART4_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_UART4;
    PeriphClkInitStruct.Uart4ClockSelection = RCC_UART4CLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
      Error_Handler();
    }

    /* Peripheral clock enable */
    __HAL_RCC_UART4_CLK_ENABLE();

    __HAL_RCC_GPIOD_CLK_ENABLE();
    /**UART4 GPIO Configuration
    PD11     ------> UART4_RX
    PD12     ------> UART4_TX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF8_UART4;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* UART4 DMA Init */
    /* GPDMA1_REQUEST_UART4_TX Init */
    handle_GPDMA1_Channel4.Instance = GPDMA1_Channel4;
    handle_GPDMA1_Channel4.Init.Request = GPDMA1_REQUEST_UART4_TX;
    handle_GPDMA1_Channel4.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    handle_GPDMA1_Channel4.Init.Direction = DMA_MEMORY_TO_PERIPH;
    handle_GPDMA1_Channel4.Init.SrcInc = DMA_SINC_INCREMENTED;
    handle_GPDMA1_Channel4.Init.DestInc = DMA_DINC_FIXED;
    handle_GPDMA1_Channel4.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel4.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel4.Init.Priority = DMA_LOW_PRIORITY_MID_WEIGHT;
    handle_GPDMA1_Channel4.Init.SrcBurstLength = 1;
    handle_GPDMA1_Channel4.Init.DestBurstLength = 1;
    handle_GPDMA1_Channel4.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0|DMA_DEST_ALLOCATED_PORT0;
    handle_GPDMA1_Channel4.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    handle_GPDMA1_Channel4.Init.Mode = DMA_NORMAL;
    if (HAL_DMA_Init(&handle_GPDMA1_Channel4) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(huart, hdmatx, handle_GPDMA1_Channel4);

    if (HAL_DMA_ConfigChannelAttributes(&handle_GPDMA1_Channel4, DMA_CHANNEL_NPRIV) != HAL_OK)
    {
      Error_Handler();
    }

    /* UART4 interrupt Init */
    HAL_NVIC_SetPriority(UART4_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(UART4_IRQn);
    /* USER CODE BEGIN UART4_MspInit 1 */

    /* USER CODE END UART4_MspInit 1 */
  }
  else if(huart->Instance==UART9)
  {
    /* USER CODE BEGIN UART9_MspInit 0 */

    /* USER CODE END UART9_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_UART9;
    PeriphClkInitStruct.Uart9ClockSelection = RCC_UART9CLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
      Error_Handler();
    }

    /* Peripheral clock enable */
    __HAL_RCC_UART9_CLK_ENABLE();

    __HAL_RCC_GPIOD_CLK_ENABLE();
    /**UART9 GPIO Configuration
    PD14     ------> UART9_RX
    PD15     ------> UART9_TX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_14|GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF11_UART9;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* UART9 DMA Init */
    /* GPDMA1_REQUEST_UART9_RX Init */
    handle_GPDMA1_Channel5.Instance = GPDMA1_Channel5;
    handle_GPDMA1_Channel5.Init.Request = GPDMA1_REQUEST_UART9_RX;
    handle_GPDMA1_Channel5.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    handle_GPDMA1_Channel5.Init.Direction = DMA_PERIPH_TO_MEMORY;
    handle_GPDMA1_Channel5.Init.SrcInc = DMA_SINC_FIXED;
    handle_GPDMA1_Channel5.Init.DestInc = DMA_DINC_INCREMENTED;
    handle_GPDMA1_Channel5.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel5.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel5.Init.Priority = DMA_LOW_PRIORITY_HIGH_WEIGHT;
    handle_GPDMA1_Channel5.Init.SrcBurstLength = 1;
    handle_GPDMA1_Channel5.Init.DestBurstLength = 1;
    handle_GPDMA1_Channel5.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0|DMA_DEST_ALLOCATED_PORT0;
    handle_GPDMA1_Channel5.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    handle_GPDMA1_Channel5.Init.Mode = DMA_NORMAL;
    if (HAL_DMA_Init(&handle_GPDMA1_Channel5) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(huart, hdmarx, handle_GPDMA1_Channel5);

    if (HAL_DMA_ConfigChannelAttributes(&handle_GPDMA1_Channel5, DMA_CHANNEL_NPRIV) != HAL_OK)
    {
      Error_Handler();
    }

    /* UART9 interrupt Init */
    HAL_NVIC_SetPriority(UART9_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(UART9_IRQn);
    /* USER CODE BEGIN UART9_MspInit 1 */

    /* USER CODE END UART9_MspInit 1 */
  }

}

/**
  * @brief UART MSP De-Initialization
  * This function freeze the hardware resources used in this example
  * @param huart: UART handle pointer
  * @retval None
  */
void HAL_UART_MspDeInit(UART_HandleTypeDef* huart)
{
  if(huart->Instance==LPUART1)
  {
    /* USER CODE BEGIN LPUART1_MspDeInit 0 */

    /* USER CODE END LPUART1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_LPUART1_CLK_DISABLE();

    /**LPUART1 GPIO Configuration
    PA9     ------> LPUART1_TX
    PA10     ------> LPUART1_RX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9|GPIO_PIN_10);

    /* USER CODE BEGIN LPUART1_MspDeInit 1 */

    /* USER CODE END LPUART1_MspDeInit 1 */
  }
  else if(huart->Instance==UART4)
  {
    /* USER CODE BEGIN UART4_MspDeInit 0 */

    /* USER CODE END UART4_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_UART4_CLK_DISABLE();

    /**UART4 GPIO Configuration
    PD11     ------> UART4_RX
    PD12     ------> UART4_TX
    */
    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_11|GPIO_PIN_12);

    /* UART4 DMA DeInit */
    HAL_DMA_DeInit(huart->hdmatx);

    /* UART4 interrupt DeInit */
    HAL_NVIC_DisableIRQ(UART4_IRQn);
    /* USER CODE BEGIN UART4_MspDeInit 1 */

    /* USER CODE END UART4_MspDeInit 1 */
  }
  else if(huart->Instance==UART9)
  {
    /* USER CODE BEGIN UART9_MspDeInit 0 */

    /* USER CODE END UART9_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_UART9_CLK_DISABLE();

    /**UART9 GPIO Configuration
    PD14     ------> UART9_RX
    PD15     ------> UART9_TX
    */
    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_14|GPIO_PIN_15);

    /* UART9 DMA DeInit */
    HAL_DMA_DeInit(huart->hdmarx);

    /* UART9 interrupt DeInit */
    HAL_NVIC_DisableIRQ(UART9_IRQn);
    /* USER CODE BEGIN UART9_MspDeInit 1 */

    /* USER CODE END UART9_MspDeInit 1 */
  }

}

/**
  * @brief SPI MSP Initialization
  * This function configures the hardware resources used in this example
  * @param hspi: SPI handle pointer
  * @retval None
  */
void HAL_SPI_MspInit(SPI_HandleTypeDef* hspi)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(hspi->Instance==SPI1)
  {
    /* USER CODE BEGIN SPI1_MspInit 0 */

    /* USER CODE END SPI1_MspInit 0 */
    /* Peripheral clock enable */
    __HAL_RCC_SPI1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    /**SPI1 GPIO Configuration
    PA5     ------> SPI1_SCK
    PA6     ------> SPI1_MISO
    PD7     ------> SPI1_MOSI
    */
    GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* SPI1 DMA Init */
    /* GPDMA1_REQUEST_SPI1_RX Init */
    handle_GPDMA1_Channel0.Instance = GPDMA1_Channel0;
    handle_GPDMA1_Channel0.Init.Request = GPDMA1_REQUEST_SPI1_RX;
    handle_GPDMA1_Channel0.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    handle_GPDMA1_Channel0.Init.Direction = DMA_PERIPH_TO_MEMORY;
    handle_GPDMA1_Channel0.Init.SrcInc = DMA_SINC_FIXED;
    handle_GPDMA1_Channel0.Init.DestInc = DMA_DINC_INCREMENTED;
    handle_GPDMA1_Channel0.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel0.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel0.Init.Priority = DMA_LOW_PRIORITY_HIGH_WEIGHT;
    handle_GPDMA1_Channel0.Init.SrcBurstLength = 1;
    handle_GPDMA1_Channel0.Init.DestBurstLength = 1;
    handle_GPDMA1_Channel0.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0|DMA_DEST_ALLOCATED_PORT0;
    handle_GPDMA1_Channel0.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    handle_GPDMA1_Channel0.Init.Mode = DMA_NORMAL;
    if (HAL_DMA_Init(&handle_GPDMA1_Channel0) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(hspi, hdmarx, handle_GPDMA1_Channel0);

    if (HAL_DMA_ConfigChannelAttributes(&handle_GPDMA1_Channel0, DMA_CHANNEL_NPRIV) != HAL_OK)
    {
      Error_Handler();
    }

    /* SPI1 interrupt Init */
    HAL_NVIC_SetPriority(SPI1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(SPI1_IRQn);
    /* USER CODE BEGIN SPI1_MspInit 1 */

    /* USER CODE END SPI1_MspInit 1 */
  }
  else if(hspi->Instance==SPI2)
  {
    /* USER CODE BEGIN SPI2_MspInit 0 */

    /* USER CODE END SPI2_MspInit 0 */
    /* Peripheral clock enable */
    __HAL_RCC_SPI2_CLK_ENABLE();

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    /**SPI2 GPIO Configuration
    PC2     ------> SPI2_MISO
    PG1     ------> SPI2_MOSI
    PD3     ------> SPI2_SCK
    */
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF7_SPI2;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* SPI2 DMA Init */
    /* GPDMA1_REQUEST_SPI2_RX Init */
    handle_GPDMA1_Channel1.Instance = GPDMA1_Channel1;
    handle_GPDMA1_Channel1.Init.Request = GPDMA1_REQUEST_SPI2_RX;
    handle_GPDMA1_Channel1.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    handle_GPDMA1_Channel1.Init.Direction = DMA_PERIPH_TO_MEMORY;
    handle_GPDMA1_Channel1.Init.SrcInc = DMA_SINC_FIXED;
    handle_GPDMA1_Channel1.Init.DestInc = DMA_DINC_INCREMENTED;
    handle_GPDMA1_Channel1.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel1.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel1.Init.Priority = DMA_LOW_PRIORITY_HIGH_WEIGHT;
    handle_GPDMA1_Channel1.Init.SrcBurstLength = 1;
    handle_GPDMA1_Channel1.Init.DestBurstLength = 1;
    handle_GPDMA1_Channel1.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0|DMA_DEST_ALLOCATED_PORT0;
    handle_GPDMA1_Channel1.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    handle_GPDMA1_Channel1.Init.Mode = DMA_NORMAL;
    if (HAL_DMA_Init(&handle_GPDMA1_Channel1) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(hspi, hdmarx, handle_GPDMA1_Channel1);

    if (HAL_DMA_ConfigChannelAttributes(&handle_GPDMA1_Channel1, DMA_CHANNEL_NPRIV) != HAL_OK)
    {
      Error_Handler();
    }

    /* SPI2 interrupt Init */
    HAL_NVIC_SetPriority(SPI2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(SPI2_IRQn);
    /* USER CODE BEGIN SPI2_MspInit 1 */

    /* USER CODE END SPI2_MspInit 1 */
  }
  else if(hspi->Instance==SPI3)
  {
    /* USER CODE BEGIN SPI3_MspInit 0 */

    /* USER CODE END SPI3_MspInit 0 */
    /* Peripheral clock enable */
    __HAL_RCC_SPI3_CLK_ENABLE();

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    /**SPI3 GPIO Configuration
    PC10     ------> SPI3_SCK
    PC11     ------> SPI3_MISO
    PD6     ------> SPI3_MOSI
    */
    GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI3;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* SPI3 DMA Init */
    /* GPDMA1_REQUEST_SPI3_RX Init */
    handle_GPDMA1_Channel2.Instance = GPDMA1_Channel2;
    handle_GPDMA1_Channel2.Init.Request = GPDMA1_REQUEST_SPI3_RX;
    handle_GPDMA1_Channel2.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    handle_GPDMA1_Channel2.Init.Direction = DMA_PERIPH_TO_MEMORY;
    handle_GPDMA1_Channel2.Init.SrcInc = DMA_SINC_FIXED;
    handle_GPDMA1_Channel2.Init.DestInc = DMA_DINC_INCREMENTED;
    handle_GPDMA1_Channel2.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel2.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel2.Init.Priority = DMA_LOW_PRIORITY_HIGH_WEIGHT;
    handle_GPDMA1_Channel2.Init.SrcBurstLength = 1;
    handle_GPDMA1_Channel2.Init.DestBurstLength = 1;
    handle_GPDMA1_Channel2.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0|DMA_DEST_ALLOCATED_PORT0;
    handle_GPDMA1_Channel2.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    handle_GPDMA1_Channel2.Init.Mode = DMA_NORMAL;
    if (HAL_DMA_Init(&handle_GPDMA1_Channel2) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(hspi, hdmarx, handle_GPDMA1_Channel2);

    if (HAL_DMA_ConfigChannelAttributes(&handle_GPDMA1_Channel2, DMA_CHANNEL_NPRIV) != HAL_OK)
    {
      Error_Handler();
    }

    /* SPI3 interrupt Init */
    HAL_NVIC_SetPriority(SPI3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(SPI3_IRQn);
    /* USER CODE BEGIN SPI3_MspInit 1 */

    /* USER CODE END SPI3_MspInit 1 */
  }
  else if(hspi->Instance==SPI4)
  {
    /* USER CODE BEGIN SPI4_MspInit 0 */

    /* USER CODE END SPI4_MspInit 0 */
    /* Peripheral clock enable */
    __HAL_RCC_SPI4_CLK_ENABLE();

    __HAL_RCC_GPIOE_CLK_ENABLE();
    /**SPI4 GPIO Configuration
    PE2     ------> SPI4_SCK
    PE5     ------> SPI4_MISO
    PE14     ------> SPI4_MOSI
    */
    GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_5|GPIO_PIN_14;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI4;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    /* SPI4 DMA Init */
    /* GPDMA1_REQUEST_SPI4_RX Init */
    handle_GPDMA1_Channel3.Instance = GPDMA1_Channel3;
    handle_GPDMA1_Channel3.Init.Request = GPDMA1_REQUEST_SPI4_RX;
    handle_GPDMA1_Channel3.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    handle_GPDMA1_Channel3.Init.Direction = DMA_PERIPH_TO_MEMORY;
    handle_GPDMA1_Channel3.Init.SrcInc = DMA_SINC_FIXED;
    handle_GPDMA1_Channel3.Init.DestInc = DMA_DINC_INCREMENTED;
    handle_GPDMA1_Channel3.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel3.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel3.Init.Priority = DMA_LOW_PRIORITY_HIGH_WEIGHT;
    handle_GPDMA1_Channel3.Init.SrcBurstLength = 1;
    handle_GPDMA1_Channel3.Init.DestBurstLength = 1;
    handle_GPDMA1_Channel3.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0|DMA_DEST_ALLOCATED_PORT0;
    handle_GPDMA1_Channel3.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    handle_GPDMA1_Channel3.Init.Mode = DMA_NORMAL;
    if (HAL_DMA_Init(&handle_GPDMA1_Channel3) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(hspi, hdmarx, handle_GPDMA1_Channel3);

    if (HAL_DMA_ConfigChannelAttributes(&handle_GPDMA1_Channel3, DMA_CHANNEL_NPRIV) != HAL_OK)
    {
      Error_Handler();
    }

    /* SPI4 interrupt Init */
    HAL_NVIC_SetPriority(SPI4_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(SPI4_IRQn);
    /* USER CODE BEGIN SPI4_MspInit 1 */

    /* USER CODE END SPI4_MspInit 1 */
  }
  else if(hspi->Instance==SPI6)
  {
    /* USER CODE BEGIN SPI6_MspInit 0 */

    /* USER CODE END SPI6_MspInit 0 */
    /* Peripheral clock enable */
    __HAL_RCC_SPI6_CLK_ENABLE();

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    /**SPI6 GPIO Configuration
    PC12     ------> SPI6_SCK
    PG12     ------> SPI6_MISO
    PG14     ------> SPI6_MOSI
    */
    GPIO_InitStruct.Pin = GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI6;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_14;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI6;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    /* USER CODE BEGIN SPI6_MspInit 1 */

    /* USER CODE END SPI6_MspInit 1 */
  }

}

/**
  * @brief SPI MSP De-Initialization
  * This function freeze the hardware resources used in this example
  * @param hspi: SPI handle pointer
  * @retval None
  */
void HAL_SPI_MspDeInit(SPI_HandleTypeDef* hspi)
{
  if(hspi->Instance==SPI1)
  {
    /* USER CODE BEGIN SPI1_MspDeInit 0 */

    /* USER CODE END SPI1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_SPI1_CLK_DISABLE();

    /**SPI1 GPIO Configuration
    PA5     ------> SPI1_SCK
    PA6     ------> SPI1_MISO
    PD7     ------> SPI1_MOSI
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_5|GPIO_PIN_6);

    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_7);

    /* SPI1 DMA DeInit */
    HAL_DMA_DeInit(hspi->hdmarx);

    /* SPI1 interrupt DeInit */
    HAL_NVIC_DisableIRQ(SPI1_IRQn);
    /* USER CODE BEGIN SPI1_MspDeInit 1 */

    /* USER CODE END SPI1_MspDeInit 1 */
  }
  else if(hspi->Instance==SPI2)
  {
    /* USER CODE BEGIN SPI2_MspDeInit 0 */

    /* USER CODE END SPI2_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_SPI2_CLK_DISABLE();

    /**SPI2 GPIO Configuration
    PC2     ------> SPI2_MISO
    PG1     ------> SPI2_MOSI
    PD3     ------> SPI2_SCK
    */
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_2);

    HAL_GPIO_DeInit(GPIOG, GPIO_PIN_1);

    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_3);

    /* SPI2 DMA DeInit */
    HAL_DMA_DeInit(hspi->hdmarx);

    /* SPI2 interrupt DeInit */
    HAL_NVIC_DisableIRQ(SPI2_IRQn);
    /* USER CODE BEGIN SPI2_MspDeInit 1 */

    /* USER CODE END SPI2_MspDeInit 1 */
  }
  else if(hspi->Instance==SPI3)
  {
    /* USER CODE BEGIN SPI3_MspDeInit 0 */

    /* USER CODE END SPI3_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_SPI3_CLK_DISABLE();

    /**SPI3 GPIO Configuration
    PC10     ------> SPI3_SCK
    PC11     ------> SPI3_MISO
    PD6     ------> SPI3_MOSI
    */
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_10|GPIO_PIN_11);

    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_6);

    /* SPI3 DMA DeInit */
    HAL_DMA_DeInit(hspi->hdmarx);

    /* SPI3 interrupt DeInit */
    HAL_NVIC_DisableIRQ(SPI3_IRQn);
    /* USER CODE BEGIN SPI3_MspDeInit 1 */

    /* USER CODE END SPI3_MspDeInit 1 */
  }
  else if(hspi->Instance==SPI4)
  {
    /* USER CODE BEGIN SPI4_MspDeInit 0 */

    /* USER CODE END SPI4_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_SPI4_CLK_DISABLE();

    /**SPI4 GPIO Configuration
    PE2     ------> SPI4_SCK
    PE5     ------> SPI4_MISO
    PE14     ------> SPI4_MOSI
    */
    HAL_GPIO_DeInit(GPIOE, GPIO_PIN_2|GPIO_PIN_5|GPIO_PIN_14);

    /* SPI4 DMA DeInit */
    HAL_DMA_DeInit(hspi->hdmarx);

    /* SPI4 interrupt DeInit */
    HAL_NVIC_DisableIRQ(SPI4_IRQn);
    /* USER CODE BEGIN SPI4_MspDeInit 1 */

    /* USER CODE END SPI4_MspDeInit 1 */
  }
  else if(hspi->Instance==SPI6)
  {
    /* USER CODE BEGIN SPI6_MspDeInit 0 */

    /* USER CODE END SPI6_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_SPI6_CLK_DISABLE();

    /**SPI6 GPIO Configuration
    PC12     ------> SPI6_SCK
    PG12     ------> SPI6_MISO
    PG14     ------> SPI6_MOSI
    */
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_12);

    HAL_GPIO_DeInit(GPIOG, GPIO_PIN_12|GPIO_PIN_14);

    /* USER CODE BEGIN SPI6_MspDeInit 1 */

    /* USER CODE END SPI6_MspDeInit 1 */
  }

}

/**
  * @brief TIM_Base MSP Initialization
  * This function configures the hardware resources used in this example
  * @param htim_base: TIM_Base handle pointer
  * @retval None
  */
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* htim_base)
{
  if(htim_base->Instance==TIM2)
  {
    /* USER CODE BEGIN TIM2_MspInit 0 */

    /* USER CODE END TIM2_MspInit 0 */
    /* Peripheral clock enable */
    __HAL_RCC_TIM2_CLK_ENABLE();
    /* USER CODE BEGIN TIM2_MspInit 1 */

    /* USER CODE END TIM2_MspInit 1 */
  }
  else if(htim_base->Instance==TIM4)
  {
    /* USER CODE BEGIN TIM4_MspInit 0 */

    /* USER CODE END TIM4_MspInit 0 */
    /* Peripheral clock enable */
    __HAL_RCC_TIM4_CLK_ENABLE();
    /* USER CODE BEGIN TIM4_MspInit 1 */

    /* USER CODE END TIM4_MspInit 1 */
  }
  else if(htim_base->Instance==TIM5)
  {
    /* USER CODE BEGIN TIM5_MspInit 0 */

    /* USER CODE END TIM5_MspInit 0 */
    /* Peripheral clock enable */
    __HAL_RCC_TIM5_CLK_ENABLE();
    /* USER CODE BEGIN TIM5_MspInit 1 */

    /* USER CODE END TIM5_MspInit 1 */
  }
  else if(htim_base->Instance==TIM6)
  {
    /* USER CODE BEGIN TIM6_MspInit 0 */

    /* USER CODE END TIM6_MspInit 0 */
    /* Peripheral clock enable */
    __HAL_RCC_TIM6_CLK_ENABLE();
    /* USER CODE BEGIN TIM6_MspInit 1 */

    /* USER CODE END TIM6_MspInit 1 */
  }

}

/**
  * @brief TIM_PWM MSP Initialization
  * This function configures the hardware resources used in this example
  * @param htim_pwm: TIM_PWM handle pointer
  * @retval None
  */
void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef* htim_pwm)
{
  if(htim_pwm->Instance==TIM3)
  {
    /* USER CODE BEGIN TIM3_MspInit 0 */

    /* USER CODE END TIM3_MspInit 0 */
    /* Peripheral clock enable */
    __HAL_RCC_TIM3_CLK_ENABLE();
    /* USER CODE BEGIN TIM3_MspInit 1 */

    /* USER CODE END TIM3_MspInit 1 */

  }

}

void HAL_TIM_MspPostInit(TIM_HandleTypeDef* htim)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(htim->Instance==TIM3)
  {
    /* USER CODE BEGIN TIM3_MspPostInit 0 */

    /* USER CODE END TIM3_MspPostInit 0 */

    __HAL_RCC_GPIOC_CLK_ENABLE();
    /**TIM3 GPIO Configuration
    PC6     ------> TIM3_CH1
    PC7     ------> TIM3_CH2
    */
    GPIO_InitStruct.Pin = FAN_PWM1_Pin|FAN_PWM0_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* USER CODE BEGIN TIM3_MspPostInit 1 */

    /* USER CODE END TIM3_MspPostInit 1 */
  }

}
/**
  * @brief TIM_Base MSP De-Initialization
  * This function freeze the hardware resources used in this example
  * @param htim_base: TIM_Base handle pointer
  * @retval None
  */
void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef* htim_base)
{
  if(htim_base->Instance==TIM2)
  {
    /* USER CODE BEGIN TIM2_MspDeInit 0 */

    /* USER CODE END TIM2_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_TIM2_CLK_DISABLE();
    /* USER CODE BEGIN TIM2_MspDeInit 1 */

    /* USER CODE END TIM2_MspDeInit 1 */
  }
  else if(htim_base->Instance==TIM4)
  {
    /* USER CODE BEGIN TIM4_MspDeInit 0 */

    /* USER CODE END TIM4_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_TIM4_CLK_DISABLE();
    /* USER CODE BEGIN TIM4_MspDeInit 1 */

    /* USER CODE END TIM4_MspDeInit 1 */
  }
  else if(htim_base->Instance==TIM5)
  {
    /* USER CODE BEGIN TIM5_MspDeInit 0 */

    /* USER CODE END TIM5_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_TIM5_CLK_DISABLE();
    /* USER CODE BEGIN TIM5_MspDeInit 1 */

    /* USER CODE END TIM5_MspDeInit 1 */
  }
  else if(htim_base->Instance==TIM6)
  {
    /* USER CODE BEGIN TIM6_MspDeInit 0 */

    /* USER CODE END TIM6_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_TIM6_CLK_DISABLE();
    /* USER CODE BEGIN TIM6_MspDeInit 1 */

    /* USER CODE END TIM6_MspDeInit 1 */
  }

}

/**
  * @brief TIM_PWM MSP De-Initialization
  * This function freeze the hardware resources used in this example
  * @param htim_pwm: TIM_PWM handle pointer
  * @retval None
  */
void HAL_TIM_PWM_MspDeInit(TIM_HandleTypeDef* htim_pwm)
{
  if(htim_pwm->Instance==TIM3)
  {
    /* USER CODE BEGIN TIM3_MspDeInit 0 */

    /* USER CODE END TIM3_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_TIM3_CLK_DISABLE();
    /* USER CODE BEGIN TIM3_MspDeInit 1 */

    /* USER CODE END TIM3_MspDeInit 1 */
  }

}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
