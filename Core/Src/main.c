/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "adc.h"
#include "can.h"
#include "dma.h"
#include "i2c.h"
#include "lwip.h"
#include "rtc.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* ===== MOTOR CONFIGURATION ===== */

#define MOTOR_PWM_MAX_PERCENT              70
#define MOTOR_STEER_REDUCTION_MAX_PERCENT  70U
#define MOTOR_RAMP_STEP_PERCENT             2

#define JOYSTICK_DEADZONE_COUNTS           13

/* Dao chieu joystick neu can */
#define JOYSTICK_X_INVERT                  0U
#define JOYSTICK_Y_INVERT                  1U

/* Dao chieu DIR rieng tung banh */
#define MOTOR_LEFT_DIR_INVERT              1U
#define MOTOR_RIGHT_DIR_INVERT             0U

/* Muc DIR mac dinh de chay tien */
#define MOTOR_FORWARD_LEVEL                GPIO_PIN_SET

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static int16_t s_motor_left_applied = 0;
static int16_t s_motor_right_applied = 0;

/* Bat buoc tha L1 roi moi duoc chay */
static uint8_t s_motor_armed = 0U;

/* Theo doi khi Debug */
volatile int16_t g_motor_left_target = 0;
volatile int16_t g_motor_right_target = 0;
volatile int16_t g_motor_left_applied = 0;
volatile int16_t g_motor_right_applied = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */
void Robot_MotorControl_Update(
    uint8_t ps2_connected,
    uint8_t lx,
    uint8_t ly,
    uint8_t l1_pressed
);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* =====================================================
 * MOTOR CONTROL - STM32F767ZI
 * PWM: TIM1_CH1 - Left wheel
 * PWM: TIM1_CH4 - Right wheel
 * DIR: PF15 - Left wheel
 * DIR: PF14 - Right wheel
 * ===================================================== */

static int16_t Motor_Abs16(int16_t value)
{
    return (value < 0) ? -value : value;
}


/* Convert joystick raw value to -100 ... +100 */
static int16_t Joystick_ToPercent(
    uint8_t raw,
    uint8_t invert
)
{
    int16_t delta = (int16_t)raw - 128;
    int16_t deadzone = JOYSTICK_DEADZONE_COUNTS;

    if (invert != 0U)
    {
        delta = -delta;
    }

    if (delta > deadzone)
    {
        return (int16_t)(
            ((delta - deadzone) * 100) / (127 - deadzone)
        );
    }

    if (delta < -deadzone)
    {
        return (int16_t)(
            -(((-delta - deadzone) * 100) / (128 - deadzone))
        );
    }

    return 0;
}


/* Smooth acceleration and deceleration */
static int16_t Motor_RampToward(
    int16_t current,
    int16_t target
)
{
    /* Khong doi chieu truc tiep khi banh dang quay */
    if (((current > 0) && (target < 0)) ||
        ((current < 0) && (target > 0)))
    {
        target = 0;
    }

    if (current < target)
    {
        current += MOTOR_RAMP_STEP_PERCENT;

        if (current > target)
            current = target;
    }
    else if (current > target)
    {
        current -= MOTOR_RAMP_STEP_PERCENT;

        if (current < target)
            current = target;
    }

    return current;
}


/* Apply DIR and PWM to one wheel */
static void Motor_WriteOne(
    uint16_t dir_pin,
    uint8_t dir_invert,
    uint32_t pwm_channel,
    int16_t command
)
{
    uint8_t forward_requested;
    GPIO_PinState dir_level;
    GPIO_PinState reverse_level;

    uint32_t duty;
    uint32_t period_counts;
    uint32_t compare;

    forward_requested = (command >= 0) ? 1U : 0U;

    if (dir_invert != 0U)
    {
        forward_requested ^= 1U;
    }

    reverse_level = (MOTOR_FORWARD_LEVEL == GPIO_PIN_SET)
                  ? GPIO_PIN_RESET
                  : GPIO_PIN_SET;

    dir_level = (forward_requested != 0U)
              ? MOTOR_FORWARD_LEVEL
              : reverse_level;

    HAL_GPIO_WritePin(GPIOF, dir_pin, dir_level);

    duty = (uint32_t)Motor_Abs16(command);

    if (duty > MOTOR_PWM_MAX_PERCENT)
    {
        duty = MOTOR_PWM_MAX_PERCENT;
    }

    period_counts = __HAL_TIM_GET_AUTORELOAD(&htim1) + 1U;

    compare = (period_counts * duty) / 100U;

    __HAL_TIM_SET_COMPARE(
        &htim1,
        pwm_channel,
        compare
    );
}

/* Apply both wheels */
/* Apply both wheels */
static void Motor_WriteOutputs(
    int16_t left,
    int16_t right
)
{
    Motor_WriteOne(
        GPIO_LEFT_WHEEL_DIR_Pin,
        MOTOR_LEFT_DIR_INVERT,
        TIM_CHANNEL_1,
        left
    );

    Motor_WriteOne(
        GPIO_RIGHT_WHEEL_DIR_Pin,
        MOTOR_RIGHT_DIR_INVERT,
        TIM_CHANNEL_4,
        right
    );
}

/* Immediate stop - bypass acceleration ramp */
static void Motor_StopImmediate(void)
{
    s_motor_left_applied = 0;
    s_motor_right_applied = 0;

    g_motor_left_target = 0;
    g_motor_right_target = 0;

    g_motor_left_applied = 0;
    g_motor_right_applied = 0;

    Motor_WriteOutputs(0, 0);
}


/* =====================================================
 * MAIN ROBOT MOTOR CONTROL FUNCTION
 *
 * Y: Forward / Reverse
 * X: Steering
 * L1: Dead-man enable
 * ===================================================== */

void Robot_MotorControl_Update(
    uint8_t ps2_connected,
    uint8_t lx,
    uint8_t ly,
    uint8_t l1_pressed
)
{
    int16_t linear;
    int16_t steering;

    int16_t base_speed;
    int16_t reduction;
    int16_t inner_speed;

    int16_t left_target = 0;
    int16_t right_target = 0;

    /* 1. Mat ket noi: dung ngay */
    if (ps2_connected == 0U)
    {
        s_motor_armed = 0U;
        Motor_StopImmediate();
        return;
    }

    /* 2. Tha L1: dung ngay va cho phep nhan lai */
    if (l1_pressed == 0U)
    {
        s_motor_armed = 1U;
        Motor_StopImmediate();
        return;
    }

    /* 3. Chua tha L1 sau khi khoi dong/mat ket noi */
    if (s_motor_armed == 0U)
    {
        Motor_StopImmediate();
        return;
    }

    /* 4. Doc joystick */
    linear = Joystick_ToPercent(
        ly,
        JOYSTICK_Y_INVERT
    );

    steering = Joystick_ToPercent(
        lx,
        JOYSTICK_X_INVERT
    );

    base_speed = Motor_Abs16(linear);

    /* 5. Chi re khi co lenh tien/lui */
    if (linear != 0)
    {
        reduction = (int16_t)(
            (Motor_Abs16(steering) *
             MOTOR_STEER_REDUCTION_MAX_PERCENT) / 100U
        );

        inner_speed = (int16_t)(
            (base_speed * (100 - reduction)) / 100
        );

        if (linear > 0)
        {
            /* Dang tien */
            if (steering > 0)
            {
                /* Re phai: banh phai cham hon */
                left_target = base_speed;
                right_target = inner_speed;
            }
            else if (steering < 0)
            {
                /* Re trai: banh trai cham hon */
                left_target = inner_speed;
                right_target = base_speed;
            }
            else
            {
                left_target = base_speed;
                right_target = base_speed;
            }
        }
        else
        {
            /* Dang lui: doi banh trong de giu huong mui robot */
            if (steering > 0)
            {
                left_target = -inner_speed;
                right_target = -base_speed;
            }
            else if (steering < 0)
            {
                left_target = -base_speed;
                right_target = -inner_speed;
            }
            else
            {
                left_target = -base_speed;
                right_target = -base_speed;
            }
        }
    }

    /* 6. Gioi han PWM toi da 70% */
    left_target = (int16_t)(
        (left_target * MOTOR_PWM_MAX_PERCENT) / 100
    );

    right_target = (int16_t)(
        (right_target * MOTOR_PWM_MAX_PERCENT) / 100
    );

    g_motor_left_target = left_target;
    g_motor_right_target = right_target;

    /* 7. Tang giam toc binh thuong */
    s_motor_left_applied = Motor_RampToward(
        s_motor_left_applied,
        left_target
    );

    s_motor_right_applied = Motor_RampToward(
        s_motor_right_applied,
        right_target
    );

    g_motor_left_applied = s_motor_left_applied;
    g_motor_right_applied = s_motor_right_applied;

    /* 8. Xuat DIR va PWM */
    Motor_WriteOutputs(
        s_motor_left_applied,
        s_motor_right_applied
    );
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* Enable the CPU Cache */

  /* Enable I-Cache---------------------------------------------------------*/
  SCB_EnableICache();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_RTC_Init();
  MX_ADC1_Init();
  MX_CAN1_Init();
  MX_I2C1_Init();
  MX_SPI3_Init();
  MX_USART3_UART_Init();
  MX_USART6_UART_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_CAN2_Init();
  MX_I2C2_Init();
  MX_I2C3_Init();
  MX_SPI2_Init();
  MX_UART7_Init();
  MX_UART8_Init();
  MX_USART2_UART_Init();
  MX_USART1_UART_Init();
  MX_TIM5_Init();
  /* USER CODE BEGIN 2 */
  /* Start PWM for two BLDC controllers */

  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0U);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, 0U);

  if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1) != HAL_OK)
  {
      Error_Handler();
  }

  if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4) != HAL_OK)
  {
      Error_Handler();
  }
  /* USER CODE END 2 */

  /* Call init function for freertos objects (in cmsis_os2.c) */
  MX_FREERTOS_Init();

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 432;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 9;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
