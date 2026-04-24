#include "user_define.h"
#include "sc7a20.h"
#include "I2C.h"

#define Sensor_Wr_Addr 0x32
#define Sensor_Rd_Addr 0x33

static uint8_t if_data_init = SC7A20_INIT_MAX;
float base_g = 0;
float base_g_buffer[SC7A20_INIT_MAX] = {0};

uint8_t SC7A20_REG[10] = {0x67, 0x04, 0x98, 0x05, 0x08, 0x02, 0x05, 0x01, 0x15, 0x80};

int16_t sc7a20_acc_data[3] = {0};
int16_t sc7a20_delta_axes[3] = {0};
float sc7a20_motion_delta = 0.0f;
uint32_t sc7a20_i2c_error_count = 0;
static int16_t prev_acc_data[3] = {0};
static bool prev_acc_valid = false;

static bool sc7a20_reg_write(uint8_t reg_start_addr, uint8_t *reg_array, uint8_t num, uint8_t sc7a20_waddr)
{
    if (HAL_I2C_Mem_Write(&I2cHandle, (uint16_t)sc7a20_waddr, reg_start_addr, 1, (uint8_t *)reg_array, num, 5000) != HAL_OK)
    {
        printf("I2C write error: reg 0x%02X\n", reg_start_addr);
        return false;
    }
    while (HAL_I2C_GetState(&I2cHandle) != HAL_I2C_STATE_READY)
        ;

    return true;
}

static bool sc7a20_reg_read(uint8_t reg_start_addr, uint8_t *reg_array, uint8_t num, uint8_t sc7a20_raddr)
{
    if (HAL_I2C_Mem_Read(&I2cHandle, (uint16_t)sc7a20_raddr, reg_start_addr, 1, (uint8_t *)reg_array, num, 5000) != HAL_OK)
    {
        return false;
    }
    while (HAL_I2C_GetState(&I2cHandle) != HAL_I2C_STATE_READY)
        ;
    return true;
}

static bool sc7a20_reg_read_bytes(uint8_t reg_start_addr, uint8_t *reg_array, uint8_t num, uint8_t sc7a20_raddr)
{
    uint8_t start_addr = reg_start_addr;

    if (num > 1U)
    {
        /* 连续读时在子地址上带自增位，按 0x28->0x2D 一次性读完 XYZ。 */
        start_addr = (uint8_t)(reg_start_addr | 0x80U);
    }

    if (HAL_I2C_Mem_Read(&I2cHandle, (uint16_t)sc7a20_raddr, start_addr, 1, reg_array, num, 5000) != HAL_OK)
    {
        return false;
    }
    while (HAL_I2C_GetState(&I2cHandle) != HAL_I2C_STATE_READY)
        ;
    return true;
}

void sc7a20_reg_read_bytes_public(int16_t *xyz_val)
{

    uint8_t reg_start_addr = 0x28;
    uint8_t reg_array[6] = {0};
    if (!sc7a20_reg_read_bytes(reg_start_addr, reg_array, 6, Sensor_Rd_Addr))
    {
        sc7a20_i2c_error_count++;
    }
    else
    {

        xyz_val[0] = (int16_t)((uint16_t)reg_array[1] << 8 | (uint16_t)reg_array[0]) >> 6;
        xyz_val[1] = (int16_t)((uint16_t)reg_array[3] << 8 | (uint16_t)reg_array[2]) >> 6;
        xyz_val[2] = (int16_t)((uint16_t)reg_array[5] << 8 | (uint16_t)reg_array[4]) >> 6;
    }
}

uint8_t init_data[9] = {
    GSENSOR_INT_CTRL_REG1,
    GSENSOR_INT_CTRL_REG4,
    GSENSOR_INT_CTRL_REG2,
    GSENSOR_INT_CTRL_REG3,
    GSENSOR_INT_CTRL_REG6,
    GSENSOR_INT_CTRL_REG5,
    GSENSOR_INT_AOI1_CFG,
    GSENSOR_INT_AOI1_THS,
    GSENSOR_INT_AOI1_DUR};

void sc7a20_init(void)
{
    /* 普通初始化：200Hz、三轴使能。 */
    // 注意这里是0x67 不是0x57，ODR=200Hz 和 XYZ 使能。
    sc7a20_reg_write(0x20, &SC7A20_REG[0], 1, Sensor_Wr_Addr);

    /* 使能高通滤波。 */
    sc7a20_reg_write(0x20, &init_data[0], 1, Sensor_Wr_Addr);
    sc7a20_reg_write(0x21, &init_data[2], 1, Sensor_Wr_Addr);

    /* 量程设为 +/-2g。 */
    sc7a20_reg_write(0x23, &init_data[1], 1, Sensor_Wr_Addr);

    /* 其他控制寄存器初始化。 */
    sc7a20_reg_write(0x1e, &init_data[3], 1, Sensor_Wr_Addr);
    sc7a20_reg_write(0x57, &init_data[4], 1, Sensor_Wr_Addr);

    /* INT1 电平极性相关配置。 */
    sc7a20_reg_write(0x25, &init_data[7], 1, Sensor_Wr_Addr);
}

void init_gsensor_for_interrupt(void)
{

    /* CTRL_REG1 (0x20): ODR、工作模式、XYZ 使能。 */
    sc7a20_reg_write(0x20, &init_data[0], 1, Sensor_Wr_Addr);

    /* CTRL_REG4 (0x23): BDU=1, FS=+/-2g。 */
    sc7a20_reg_write(0x23, &init_data[1], 1, Sensor_Wr_Addr);

    /* CTRL_REG2 (0x21): 高通滤波参与 AOI1 中断。 */
    sc7a20_reg_write(0x21, &init_data[2], 1, Sensor_Wr_Addr);

    /* CTRL_REG3 (0x22): AOI1 路由到 INT1。 */
    sc7a20_reg_write(0x22, &init_data[3], 1, Sensor_Wr_Addr);

    /* CTRL_REG6 (0x25): INT2 默认配置。 */
    sc7a20_reg_write(0x25, &init_data[4], 1, Sensor_Wr_Addr);

    /* CTRL_REG5 (0x24): INT1 非锁存。 */
    sc7a20_reg_write(0x24, &init_data[5], 1, Sensor_Wr_Addr);

    /* AOI1_CFG (0x30): 当前按 user_define.h 的配置决定哪些轴参与中断。 */
    sc7a20_reg_write(0x30, &init_data[6], 1, Sensor_Wr_Addr);

    /* AOI1_THS (0x32): 中断门限。 */
    sc7a20_reg_write(0x32, &init_data[7], 1, Sensor_Wr_Addr);

    /* AOI1_DURATION (0x33): 门限需要持续的采样周期数。 */
    sc7a20_reg_write(0x33, &init_data[8], 1, Sensor_Wr_Addr);
}

void count_base_g(float combin_acc)
{
    if (if_data_init > 0)
    {
        base_g_buffer[SC7A20_INIT_MAX - if_data_init] = combin_acc;
        if_data_init--;
    }
    else
    {
        float sum = 0;
        for (int i = 0; i < SC7A20_INIT_MAX; i++)
        {
            sum += base_g_buffer[i];
        }
        base_g = sum / SC7A20_INIT_MAX;
    }
}

void sc7a20_read_acc_raw(int16_t *acc_data)
{
    uint8_t raw_data[6] = {0};
    int16_t delta_x;
    int16_t delta_y;
    int16_t delta_z;
    int16_t max_delta;

    if (!sc7a20_reg_read_bytes(0x28, raw_data, 6, Sensor_Rd_Addr))
    {
        sc7a20_i2c_error_count++;
        return;
    }

    acc_data[0] = (int16_t)(((int16_t)((uint16_t)raw_data[1] << 8 | raw_data[0])) >> 4);
    acc_data[1] = (int16_t)(((int16_t)((uint16_t)raw_data[3] << 8 | raw_data[2])) >> 4);
    acc_data[2] = (int16_t)(((int16_t)((uint16_t)raw_data[5] << 8 | raw_data[4])) >> 4);

    if (prev_acc_valid)
    {
        delta_x = acc_data[0] - prev_acc_data[0];
        delta_y = acc_data[1] - prev_acc_data[1];
        delta_z = acc_data[2] - prev_acc_data[2];

        sc7a20_delta_axes[0] = delta_x;
        sc7a20_delta_axes[1] = delta_y;
        sc7a20_delta_axes[2] = delta_z;

        max_delta = (int16_t)abs(delta_x);
        if ((int16_t)abs(delta_y) > max_delta)
        {
            max_delta = (int16_t)abs(delta_y);
        }
        if ((int16_t)abs(delta_z) > max_delta)
        {
            max_delta = (int16_t)abs(delta_z);
        }
        sc7a20_motion_delta = (float)max_delta;
    }
    else
    {
        sc7a20_delta_axes[0] = 0;
        sc7a20_delta_axes[1] = 0;
        sc7a20_delta_axes[2] = 0;
        sc7a20_motion_delta = 0.0f;
        prev_acc_valid = true;
    }

    prev_acc_data[0] = acc_data[0];
    prev_acc_data[1] = acc_data[1];
    prev_acc_data[2] = acc_data[2];
}

void sc7a20_read_acc(int16_t *acc_data, float *combin_acc)
{
    float temp = 0.0f;
    sc7a20_read_acc_raw(acc_data);

    temp = sqrtf(acc_data[0] * acc_data[0] + acc_data[1] * acc_data[1] + acc_data[2] * acc_data[2]);

    count_base_g(temp);

    if (if_data_init == 0)
    {
        *combin_acc = fabsf(temp - base_g);
    }
    else
    {
        *combin_acc = 0.0f;
    }
}
