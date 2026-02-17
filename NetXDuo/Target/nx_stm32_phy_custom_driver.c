/**************************************************************************/
/*                                                                        */
/*       Copyright (c) Microsoft Corporation. All rights reserved.        */
/*                                                                        */
/*       This software is licensed under the Microsoft Software License   */
/*       Terms for Microsoft Azure RTOS. Full text of the license can be  */
/*       found in the LICENSE file at https://aka.ms/AzureRTOS_EULA       */
/*       and in the root directory of this software.                      */
/*                                                                        */
/**************************************************************************/

/* Private includes ----------------------------------------------------------*/
#include "nx_stm32_phy_driver.h"
#include "nx_stm32_eth_config.h"

/* USER CODE BEGIN Includes */
#include "lan8670.h"
#include "u_nx_ethernet.h"
#include "u_tx_debug.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
lan8670_t lan8670;
static lan8670_IOCtx_t lan8670_io_ctx;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
static int32_t ETH_WritePHYRegister(uint32_t DevAddr, uint32_t RegAddr, uint32_t RegVal)
{
    return HAL_ETH_WritePHYRegister(&eth_handle, DevAddr, RegAddr, RegVal);
}

static int32_t ETH_ReadPHYRegister(uint32_t DevAddr, uint32_t RegAddr, uint32_t *pRegVal)
{
    return HAL_ETH_ReadPHYRegister(&eth_handle, DevAddr, RegAddr, pRegVal);
}
/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

int32_t nx_eth_phy_init(void)
{

/* USER CODE BEGIN PHY_INIT_0 */
PRINTLN_INFO("got to this part of nx_eth_phy_init()");
/* USER CODE END PHY_INIT_0 */

    int32_t ret = ETH_PHY_STATUS_OK;

/* USER CODE BEGIN PHY_INIT_1 */
    /* Set up LAN8670 IO */
    lan8670_io_ctx.Init = NULL;
    lan8670_io_ctx.DeInit = NULL;
    lan8670_io_ctx.WriteReg = ETH_WritePHYRegister;  // STM32 ETH write function
    lan8670_io_ctx.ReadReg = ETH_ReadPHYRegister;    // STM32 ETH read function
    lan8670_io_ctx.GetTick = HAL_GetTick;            // HAL tick function

    /* Register the IO functions with the LAN8670 driver */
    ret = LAN8670_RegisterBusIO(&lan8670, &lan8670_io_ctx);
    if (ret != LAN8670_STATUS_OK)
    {
        return ETH_PHY_STATUS_ERROR;
    }

    /* Initialize the LAN8670 */
    ret = LAN8670_Init(&lan8670, 0b00001);
    if (ret != LAN8670_STATUS_OK)
    {
        return ETH_PHY_STATUS_ERROR;
    }

    /* Reset the PHY */
    ret = LAN8670_Reset(&lan8670);
    if (ret != LAN8670_STATUS_OK)
    {
        return ETH_PHY_STATUS_ERROR;
    }
    HAL_Delay(2);

    /* Enable PLCA (Physical Layer Collision Avoidance) */
    ret = LAN8670_PLCA_On(&lan8670, true);
    if (ret != LAN8670_STATUS_OK)
    {
        return ETH_PHY_STATUS_ERROR;
    }

    /* Set PLCA node count and ID */
    ret = LAN8670_PLCA_Set_Node_Count(&lan8670, PLCA_NUM_NODES);
    if (ret != LAN8670_STATUS_OK)
    {
        return ETH_PHY_STATUS_ERROR;
    }

    ret = LAN8670_PLCA_Set_Node_Id(&lan8670, PLCA_COMPUTE);
    if (ret != LAN8670_STATUS_OK)
    {
        return ETH_PHY_STATUS_ERROR;
    }

    /* Enable collision detection */
    ret = LAN8670_Collision_Detection(&lan8670, true);
/* USER CODE END PHY_INIT_1 */
    return ret;
}

int32_t nx_eth_phy_get_link_state(void)
{

  /* USER CODE BEGIN LINK_STATE_0 */
    return ETH_PHY_STATUS_10MBITS_FULLDUPLEX;
  /* USER CODE END LINK_STATE_0 */

  int32_t  linkstate = ETH_PHY_STATUS_LINK_ERROR;

  /* USER CODE BEGIN LINK_STATE_1 */
    uint8_t link_state;
    int32_t ret = LAN8670_Get_Link_State(&lan8670, &link_state);
    // NOTE: This should always return 1, since the LAN8670 doesn't support link status indication.
    
    if (ret == LAN8670_STATUS_OK && link_state == 1)
    {
      linkstate = ETH_PHY_STATUS_10MBITS_FULLDUPLEX;  // since we're using 10BASE-T1S
    }
    else
    {
      linkstate = ETH_PHY_STATUS_LINK_DOWN;
    }
  /* USER CODE END LINK_STATE_1 */
  return linkstate;
}

nx_eth_phy_handle_t nx_eth_phy_get_handle(void)
{

 nx_eth_phy_handle_t handle = NULL;
  /* USER CODE BEGIN GET_HANDLE */
    handle = (nx_eth_phy_handle_t)&lan8670;
  /* USER CODE END GET_HANDLE */
    return handle;
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
