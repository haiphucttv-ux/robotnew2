
/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * PS2 Controller Task added for STM32F767ZI
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdint.h>
#ifdef ROBOT_MICROROS
#include "robot_microros.h"
#endif
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define PS2_TASK_PERIOD_MS       10U
#define PS2_AXIS_CENTER          128U
#define PS2_VALID_FRAMES_TO_CONNECT       3U
#define PS2_RECONFIGURE_AFTER_INVALID   100U
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
osThreadId defaultTaskHandle;

/* USER CODE BEGIN Variables */

osThreadId ps2TaskHandle;

/* Trang thai PS2 duoc cap nhat boi PS2 Task */
volatile uint8_t g_ps2_connected = 0U;

volatile uint8_t g_ps2_lx = PS2_AXIS_CENTER;
volatile uint8_t g_ps2_ly = PS2_AXIS_CENTER;

volatile uint8_t g_ps2_l1_pressed = 0U;

/* USER CODE END Variables */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
extern void PS2_ReconfigureAnalog(void);
extern void Robot_MotorControl_Update(
    uint8_t ps2_connected,
    uint8_t lx,
    uint8_t ly,
    uint8_t l1_pressed
);
extern uint8_t PS2_Init(void);

extern uint8_t PS2_ReadState(
    uint8_t *lx,
    uint8_t *ly,
    uint8_t *l1_pressed
);

void StartPS2Task(void const *argument);

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const *argument);

extern void MX_USB_DEVICE_Init(void);
extern void MX_LWIP_Init(void);

void MX_FREERTOS_Init(void);

/* GetIdleTaskMemory prototype */
void vApplicationGetIdleTaskMemory(
    StaticTask_t **ppxIdleTaskTCBBuffer,
    StackType_t **ppxIdleTaskStackBuffer,
    uint32_t *pulIdleTaskStackSize
);

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */

static StaticTask_t xIdleTaskTCBBuffer;

static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory(
    StaticTask_t **ppxIdleTaskTCBBuffer,
    StackType_t **ppxIdleTaskStackBuffer,
    uint32_t *pulIdleTaskStackSize)
{
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
    *ppxIdleTaskStackBuffer = &xIdleStack[0];
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief FreeRTOS initialization
  * @param None
  * @retval None
  */
void MX_FREERTOS_Init(void)
{
    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    /* USER CODE BEGIN RTOS_MUTEX */

    /* USER CODE END RTOS_MUTEX */

    /* USER CODE BEGIN RTOS_SEMAPHORES */

    /* USER CODE END RTOS_SEMAPHORES */

    /* USER CODE BEGIN RTOS_TIMERS */

    /* USER CODE END RTOS_TIMERS */

    /* USER CODE BEGIN RTOS_QUEUES */

    /* USER CODE END RTOS_QUEUES */

    /* Create the thread(s) */

    /* Default Task */
    osThreadDef(
        defaultTask,
        StartDefaultTask,
        osPriorityNormal,
        0,
        256
    );

    defaultTaskHandle = osThreadCreate(
        osThread(defaultTask),
        NULL
    );

    /* USER CODE BEGIN RTOS_THREADS */

    /* PS2 Controller Task */
    osThreadDef(
        ps2Task,
        StartPS2Task,
        osPriorityNormal,
        0,
        256
    );

    ps2TaskHandle = osThreadCreate(
        osThread(ps2Task),
        NULL
    );

    /* USER CODE END RTOS_THREADS */
}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief Function implementing the defaultTask thread.
  * @param argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */

void StartDefaultTask(void const *argument)
{
    (void)argument;

    /* Initialize USB Device */
    MX_USB_DEVICE_Init();

    /* Initialize LWIP */
    MX_LWIP_Init();

    /* USER CODE BEGIN StartDefaultTask */
#ifdef ROBOT_MICROROS
    osThreadDef(microRosTask, Robot_MicroRosTask, osPriorityBelowNormal, 0, 4096);
    (void)osThreadCreate(osThread(microRosTask), NULL);
#endif

    for (;;)
    {
        osDelay(1U);
    }

    /* USER CODE END StartDefaultTask */
}

/* Private application code -----------------------------------------------*/
/* USER CODE BEGIN Application */

/**
  * @brief PS2 Controller Task
  *
  * Nhiem vu:
  * - Khoi tao PS2
  * - Doc joystick trai X/Y
  * - Doc nut L1
  * - Cap nhat trang thai ket noi
  *
  * Chu ky doc: 10 ms
  */

void StartPS2Task(void const *argument)
{
    uint8_t lx = PS2_AXIS_CENTER;
    uint8_t ly = PS2_AXIS_CENTER;
    uint8_t l1 = 0U;

    uint8_t valid_frames = 0U;
    uint16_t invalid_frames = 0U;

    (void)argument;

    /* ==========================================
     * 1. INITIALIZE PS2
     * ========================================== */

    for (;;)
    {
        if (PS2_Init() != 0U)
        {
            break;
        }

        g_ps2_connected = 0U;

        g_ps2_lx = PS2_AXIS_CENTER;
        g_ps2_ly = PS2_AXIS_CENTER;
        g_ps2_l1_pressed = 0U;

        Robot_MotorControl_Update(
            0U,
            PS2_AXIS_CENTER,
            PS2_AXIS_CENTER,
            0U
        );

        osDelay(500U);
    }

    /* ==========================================
     * 2. READ PS2 CONTINUOUSLY
     * ========================================== */

    for (;;)
    {
        if (PS2_ReadState(&lx, &ly, &l1) != 0U)
        {
            /* Valid frame */
            invalid_frames = 0U;

            if (valid_frames < PS2_VALID_FRAMES_TO_CONNECT)
            {
                valid_frames++;
            }

            if (valid_frames >= PS2_VALID_FRAMES_TO_CONNECT)
            {
                g_ps2_connected = 1U;

                g_ps2_lx = lx;
                g_ps2_ly = ly;
                g_ps2_l1_pressed = l1;
            }
            else
            {
                /* Wait for 3 consecutive valid frames */
                g_ps2_connected = 0U;

                g_ps2_lx = PS2_AXIS_CENTER;
                g_ps2_ly = PS2_AXIS_CENTER;
                g_ps2_l1_pressed = 0U;
            }
        }
        else
        {
            /* Invalid frame: stop motor immediately */
            valid_frames = 0U;

            g_ps2_connected = 0U;

            g_ps2_lx = PS2_AXIS_CENTER;
            g_ps2_ly = PS2_AXIS_CENTER;
            g_ps2_l1_pressed = 0U;

            if (invalid_frames < PS2_RECONFIGURE_AFTER_INVALID)
            {
                invalid_frames++;
            }

            /* Reconfigure after 1 second of continuous errors */
            if (invalid_frames >= PS2_RECONFIGURE_AFTER_INVALID)
            {
                PS2_ReconfigureAnalog();
                invalid_frames = 0U;
            }
        }

        /* Update motor control */
        Robot_MotorControl_Update(
            g_ps2_connected,
            g_ps2_lx,
            g_ps2_ly,
            g_ps2_l1_pressed
        );

        osDelay(PS2_TASK_PERIOD_MS);
    }
}


/* USER CODE END Application */
