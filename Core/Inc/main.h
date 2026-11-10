/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "stm32f7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define I2C2_SDA_AUX_SENSOR_BUS_Pin GPIO_PIN_0
#define I2C2_SDA_AUX_SENSOR_BUS_GPIO_Port GPIOF
#define I2C2_SCL_AUX_SENSOR_BUS_Pin GPIO_PIN_1
#define I2C2_SCL_AUX_SENSOR_BUS_GPIO_Port GPIOF
#define UART7_RX_EXPANSION_PORT_Pin GPIO_PIN_6
#define UART7_RX_EXPANSION_PORT_GPIO_Port GPIOF
#define UART7_TX_EXPANSION_PORT_Pin GPIO_PIN_7
#define UART7_TX_EXPANSION_PORT_GPIO_Port GPIOF
#define SPI2_MISO_EXPANSION_BUS_Pin GPIO_PIN_2
#define SPI2_MISO_EXPANSION_BUS_GPIO_Port GPIOC
#define SPI2_MOSI_EXPANSION_BUS_Pin GPIO_PIN_3
#define SPI2_MOSI_EXPANSION_BUS_GPIO_Port GPIOC
#define ADC1_IN0_BATTERY_VOLTAGE_Pin GPIO_PIN_0
#define ADC1_IN0_BATTERY_VOLTAGE_GPIO_Port GPIOA
#define ADC1_IN3_BATTERY_TOTAL_CURRENT_Pin GPIO_PIN_3
#define ADC1_IN3_BATTERY_TOTAL_CURRENT_GPIO_Port GPIOA
#define ADC1_IN4_LEFT_MOTOR_CURRENT_Pin GPIO_PIN_4
#define ADC1_IN4_LEFT_MOTOR_CURRENT_GPIO_Port GPIOA
#define ADC1_IN5_RIGHT_MOTOR_CURRENT_Pin GPIO_PIN_5
#define ADC1_IN5_RIGHT_MOTOR_CURRENT_GPIO_Port GPIOA
#define TIM3_CH1_HALL_LEFT_A_Pin GPIO_PIN_6
#define TIM3_CH1_HALL_LEFT_A_GPIO_Port GPIOA
#define TIM3_CH3_HALL_LEFT_C_Pin GPIO_PIN_0
#define TIM3_CH3_HALL_LEFT_C_GPIO_Port GPIOB
#define ADC1_IN9_BATTERY_TEMPERATURE_Pin GPIO_PIN_1
#define ADC1_IN9_BATTERY_TEMPERATURE_GPIO_Port GPIOB
#define ESTOP_FB_Pin GPIO_PIN_12
#define ESTOP_FB_GPIO_Port GPIOF
#define PS2_DAT_Pin GPIO_PIN_13
#define PS2_DAT_GPIO_Port GPIOF
#define GPIO_RIGHT_WHEEL_DIR_Pin GPIO_PIN_14
#define GPIO_RIGHT_WHEEL_DIR_GPIO_Port GPIOF
#define GPIO_LEFT_WHEEL_DIR_Pin GPIO_PIN_15
#define GPIO_LEFT_WHEEL_DIR_GPIO_Port GPIOF
#define BUMPER_SENSOR_Pin GPIO_PIN_0
#define BUMPER_SENSOR_GPIO_Port GPIOG
#define TIM1_CH1_LEFT_WHEEL_PWM_Pin GPIO_PIN_9
#define TIM1_CH1_LEFT_WHEEL_PWM_GPIO_Port GPIOE
#define PS2_ACK_Pin GPIO_PIN_10
#define PS2_ACK_GPIO_Port GPIOE
#define PS2_CMD_Pin GPIO_PIN_11
#define PS2_CMD_GPIO_Port GPIOE
#define PS2_ATT_Pin GPIO_PIN_12
#define PS2_ATT_GPIO_Port GPIOE
#define PS2_CLK_Pin GPIO_PIN_13
#define PS2_CLK_GPIO_Port GPIOE
#define TIM1_CH4_RIGHT_WHEEL_PWM_Pin GPIO_PIN_14
#define TIM1_CH4_RIGHT_WHEEL_PWM_GPIO_Port GPIOE
#define TIM2_CH3_MED_DOOR3_SERVO_Pin GPIO_PIN_10
#define TIM2_CH3_MED_DOOR3_SERVO_GPIO_Port GPIOB
#define USART1_TX_EXPANSION_PORT_Pin GPIO_PIN_14
#define USART1_TX_EXPANSION_PORT_GPIO_Port GPIOB
#define USART1_RX_EXPANSION_PORT_Pin GPIO_PIN_15
#define USART1_RX_EXPANSION_PORT_GPIO_Port GPIOB
#define USART3_TX_MAINTENANCE_CONSOLE_Pin GPIO_PIN_8
#define USART3_TX_MAINTENANCE_CONSOLE_GPIO_Port GPIOD
#define USART3_RX_MAINTENANCE_CONSOLE_Pin GPIO_PIN_9
#define USART3_RX_MAINTENANCE_CONSOLE_GPIO_Port GPIOD
#define TIM4_CH1_HALL_RIGHT_A_Pin GPIO_PIN_12
#define TIM4_CH1_HALL_RIGHT_A_GPIO_Port GPIOD
#define TIM4_CH2_HALL_RIGHT_B_Pin GPIO_PIN_13
#define TIM4_CH2_HALL_RIGHT_B_GPIO_Port GPIOD
#define TIM4_CH3_HALL_RIGHT_C_Pin GPIO_PIN_14
#define TIM4_CH3_HALL_RIGHT_C_GPIO_Port GPIOD
#define USART6_TX_EXPANSION_PORT_Pin GPIO_PIN_6
#define USART6_TX_EXPANSION_PORT_GPIO_Port GPIOC
#define TIM3_CH2_HALL_LEFT_B_Pin GPIO_PIN_7
#define TIM3_CH2_HALL_LEFT_B_GPIO_Port GPIOC
#define I2C3_SDA_Pin GPIO_PIN_9
#define I2C3_SDA_GPIO_Port GPIOC
#define I2C3_SCL_Pin GPIO_PIN_8
#define I2C3_SCL_GPIO_Port GPIOA
#define USB_FS_VBUS_SENSE_Pin GPIO_PIN_9
#define USB_FS_VBUS_SENSE_GPIO_Port GPIOA
#define USB_FS_DM_Pin GPIO_PIN_11
#define USB_FS_DM_GPIO_Port GPIOA
#define USB_FS_DP_Pin GPIO_PIN_12
#define USB_FS_DP_GPIO_Port GPIOA
#define TIM2_CH1_MED_DOOR1_SERVO_Pin GPIO_PIN_15
#define TIM2_CH1_MED_DOOR1_SERVO_GPIO_Port GPIOA
#define SPI3_SCK_EXPANSION_BUS_Pin GPIO_PIN_10
#define SPI3_SCK_EXPANSION_BUS_GPIO_Port GPIOC
#define SPI3_MISO_EXPANSION_BUS_Pin GPIO_PIN_11
#define SPI3_MISO_EXPANSION_BUS_GPIO_Port GPIOC
#define SPI3_MOSI_EXPANSION_BUS_Pin GPIO_PIN_12
#define SPI3_MOSI_EXPANSION_BUS_GPIO_Port GPIOC
#define CAN1_RX_EXPANSION_BUS_Pin GPIO_PIN_0
#define CAN1_RX_EXPANSION_BUS_GPIO_Port GPIOD
#define CAN1_TX_EXPANSION_BUS_Pin GPIO_PIN_1
#define CAN1_TX_EXPANSION_BUS_GPIO_Port GPIOD
#define SPI2_SCK_EXPANSION_BUS_Pin GPIO_PIN_3
#define SPI2_SCK_EXPANSION_BUS_GPIO_Port GPIOD
#define USART2_TX_AUX_DEVICE_Pin GPIO_PIN_5
#define USART2_TX_AUX_DEVICE_GPIO_Port GPIOD
#define USART2_RX_AUX_DEVICE_Pin GPIO_PIN_6
#define USART2_RX_AUX_DEVICE_GPIO_Port GPIOD
#define USART6_RX_EXPANSION_PORT_Pin GPIO_PIN_9
#define USART6_RX_EXPANSION_PORT_GPIO_Port GPIOG
#define TIM2_CH2_MED_DOOR2_SERVO_Pin GPIO_PIN_3
#define TIM2_CH2_MED_DOOR2_SERVO_GPIO_Port GPIOB
#define CAN2_RX_EXPANSION_BUS_Pin GPIO_PIN_5
#define CAN2_RX_EXPANSION_BUS_GPIO_Port GPIOB
#define CAN2_TX_EXPANSION_BUS_Pin GPIO_PIN_6
#define CAN2_TX_EXPANSION_BUS_GPIO_Port GPIOB
#define I2C1_SCL_PRIMARY_SENSOR_BUS_Pin GPIO_PIN_8
#define I2C1_SCL_PRIMARY_SENSOR_BUS_GPIO_Port GPIOB
#define I2C1_SDA_PRIMARY_SENSOR_BUS_Pin GPIO_PIN_9
#define I2C1_SDA_PRIMARY_SENSOR_BUS_GPIO_Port GPIOB
#define UART8_RX_EXPANSION_PORT_Pin GPIO_PIN_0
#define UART8_RX_EXPANSION_PORT_GPIO_Port GPIOE
#define UART8_TX_EXPANSION_PORT_Pin GPIO_PIN_1
#define UART8_TX_EXPANSION_PORT_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
