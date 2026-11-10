#include "main.h"
#include "tim.h"

#include <stddef.h>
#include <stdint.h>

#define PS2_FRAME_STANDARD      9U
#define PS2_FRAME_PRESSURE     21U

#define PS2_ANALOG_ID         0x73U
#define PS2_PRESSURE_ID       0x79U
#define PS2_ANALOG_MARKER     0x5AU
#define PS2_BUTTON_L1_MASK    0x04U

#define PS2_CLOCK_LOW_US        4U
#define PS2_CLOCK_HIGH_US       4U
#define PS2_BYTE_DELAY_US      15U

#define PS2_INIT_PROBE_COUNT    5U

static uint8_t ps2_initialized = 0U;
static uint8_t ps2_tim5_started = 0U;

/* Bien theo doi khi debug */
volatile uint8_t g_ps2_last_mode = 0U;
volatile uint8_t g_ps2_last_marker = 0U;
volatile uint8_t g_ps2_last_l1_byte = 0U;
volatile uint8_t g_ps2_last_lx = 128U;
volatile uint8_t g_ps2_last_ly = 128U;
volatile uint8_t g_ps2_last_frame_ok = 0U;

volatile uint32_t g_ps2_valid_count = 0U;
volatile uint32_t g_ps2_invalid_count = 0U;
volatile uint32_t g_ps2_reconfigure_count = 0U;

/* TIM5 dem o 1 MHz, moi dem tuong ung 1 us. */
static void PS2_DelayUs(uint32_t us)
{
    uint32_t start = __HAL_TIM_GET_COUNTER(&htim5);

    while ((uint32_t)(__HAL_TIM_GET_COUNTER(&htim5) - start) < us)
    {
        /* Wait */
    }
}

static void PS2_BeginTransaction(void)
{
    HAL_GPIO_WritePin(PS2_CMD_GPIO_Port, PS2_CMD_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(PS2_CLK_GPIO_Port, PS2_CLK_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(PS2_ATT_GPIO_Port, PS2_ATT_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(PS2_ATT_GPIO_Port, PS2_ATT_Pin, GPIO_PIN_RESET);

    PS2_DelayUs(PS2_BYTE_DELAY_US);
}

static void PS2_EndTransaction(void)
{
    HAL_GPIO_WritePin(PS2_ATT_GPIO_Port, PS2_ATT_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(PS2_CMD_GPIO_Port, PS2_CMD_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(PS2_CLK_GPIO_Port, PS2_CLK_Pin, GPIO_PIN_SET);

    PS2_DelayUs(20U);
}

/* Gui va nhan mot byte, bit thap truoc. */
static uint8_t PS2_TransferByte(uint8_t tx)
{
    uint8_t rx = 0U;

    for (uint8_t bit = 0U; bit < 8U; bit++)
    {
        uint8_t mask = (uint8_t)(1U << bit);
        GPIO_PinState cmd_level =
            ((tx & mask) != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET;

        HAL_GPIO_WritePin(PS2_CMD_GPIO_Port, PS2_CMD_Pin, cmd_level);

        HAL_GPIO_WritePin(PS2_CLK_GPIO_Port, PS2_CLK_Pin, GPIO_PIN_RESET);
        PS2_DelayUs(PS2_CLOCK_LOW_US);

        if (HAL_GPIO_ReadPin(PS2_DAT_GPIO_Port, PS2_DAT_Pin) == GPIO_PIN_SET)
        {
            rx |= mask;
        }

        HAL_GPIO_WritePin(PS2_CLK_GPIO_Port, PS2_CLK_Pin, GPIO_PIN_SET);
        PS2_DelayUs(PS2_CLOCK_HIGH_US);
    }

    HAL_GPIO_WritePin(PS2_CMD_GPIO_Port, PS2_CMD_Pin, GPIO_PIN_SET);
    PS2_DelayUs(PS2_BYTE_DELAY_US);

    return rx;
}

static void PS2_TransferFrame(const uint8_t *tx, uint8_t *rx, uint8_t length)
{
    PS2_BeginTransaction();

    for (uint8_t i = 0U; i < length; i++)
    {
        uint8_t received = PS2_TransferByte(tx[i]);

        if (rx != NULL)
        {
            rx[i] = received;
        }
    }

    PS2_EndTransaction();
}
static void PS2_ConfigureAnalog(void)
{
    static const uint8_t enter_config[] = {
        0x01U, 0x43U, 0x00U, 0x01U, 0x00U
    };

    static const uint8_t set_analog[] = {
        0x01U, 0x44U, 0x00U, 0x01U, 0x03U,
        0x00U, 0x00U, 0x00U, 0x00U
    };

    static const uint8_t exit_config[] = {
        0x01U, 0x43U, 0x00U, 0x00U, 0x5AU,
        0x5AU, 0x5AU, 0x5AU, 0x5AU
    };

    PS2_TransferFrame(enter_config, NULL, sizeof(enter_config));
    HAL_Delay(5U);

    PS2_TransferFrame(set_analog, NULL, sizeof(set_analog));
    HAL_Delay(5U);

    PS2_TransferFrame(exit_config, NULL, sizeof(exit_config));
    HAL_Delay(5U);
}

/* Tra ve 1 khi doc duoc goi analog hop le. */
uint8_t PS2_ReadState(uint8_t *lx, uint8_t *ly, uint8_t *l1_pressed)
{
    static const uint8_t tx[PS2_FRAME_STANDARD] = {
        0x01U, 0x42U, 0x00U,
        0x00U, 0x00U, 0x00U,
        0x00U, 0x00U, 0x00U
    };

    uint8_t response[PS2_FRAME_PRESSURE] = {0U};

    if ((ps2_initialized == 0U) ||
        (lx == NULL) || (ly == NULL) || (l1_pressed == NULL))
    {
        g_ps2_last_frame_ok = 0U;
        return 0U;
    }

    PS2_BeginTransaction();

    for (uint8_t i = 0U; i < PS2_FRAME_STANDARD; i++)
    {
        response[i] = PS2_TransferByte(tx[i]);
    }

    if (response[1] == PS2_PRESSURE_ID)
    {
        for (uint8_t i = PS2_FRAME_STANDARD; i < PS2_FRAME_PRESSURE; i++)
        {
            response[i] = PS2_TransferByte(0x00U);
        }
    }

    PS2_EndTransaction();

    g_ps2_last_mode = response[1];
    g_ps2_last_marker = response[2];
    g_ps2_last_l1_byte = response[4];
    g_ps2_last_lx = response[7];
    g_ps2_last_ly = response[8];
    g_ps2_last_frame_ok = 0U;

    if (response[2] != PS2_ANALOG_MARKER)
    {
        g_ps2_invalid_count++;
        return 0U;
    }

    if ((response[1] != PS2_ANALOG_ID) && (response[1] != PS2_PRESSURE_ID))
    {
        g_ps2_invalid_count++;
        return 0U;
    }

    *lx = response[7];
    *ly = response[8];
    *l1_pressed = ((response[4] & PS2_BUTTON_L1_MASK) == 0U) ? 1U : 0U;

    g_ps2_last_frame_ok = 1U;
    g_ps2_valid_count++;
    return 1U;
}

uint8_t PS2_Init(void)
{
    uint8_t lx = 128U;
    uint8_t ly = 128U;
    uint8_t l1 = 0U;

    if (ps2_tim5_started == 0U)
    {
        if (HAL_TIM_Base_Start(&htim5) != HAL_OK)
        {
            return 0U;
        }

        ps2_tim5_started = 1U;
        __HAL_TIM_SET_COUNTER(&htim5, 0U);
    }

    HAL_GPIO_WritePin(PS2_CMD_GPIO_Port, PS2_CMD_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(PS2_CLK_GPIO_Port, PS2_CLK_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(PS2_ATT_GPIO_Port, PS2_ATT_Pin, GPIO_PIN_SET);

    HAL_Delay(100U);

    ps2_initialized = 1U;
    PS2_ConfigureAnalog();

    for (uint8_t attempt = 0U; attempt < PS2_INIT_PROBE_COUNT; attempt++)
    {
        if (PS2_ReadState(&lx, &ly, &l1) != 0U)
        {
            return 1U;
        }

        HAL_Delay(20U);
    }

    return 0U;
}

void PS2_ReconfigureAnalog(void)
{
if ((ps2_initialized == 0U) || (ps2_tim5_started == 0U))
    {
        return;
    }

    PS2_ConfigureAnalog();
    g_ps2_reconfigure_count++;
}
