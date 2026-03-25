#include "user_define.h"
#include "sc7a20.h"
#include "I2C.h"
#define Sensor_Wr_Addr 0x32
#define Sensor_Rd_Addr 0x33

static uint8_t if_data_init = SC7A20_INIT_MAX;
float base_g = 0;
float base_g_buffer[SC7A20_INIT_MAX] = {0};

uint8_t SC7A20_REG[10] = {0x57, 0x04, 0x98, 0x05, 0x08, 0x02, 0x05, 0x01, 0x15, 0x80};

int16_t sc7a20_acc_data[3] = {0};

static bool sc7a20_reg_write(uint8_t reg_start_addr, uint8_t *reg_array, uint8_t num, uint8_t sc7a20_waddr)
{
    if (HAL_I2C_Mem_Write(&I2cHandle, (uint16_t)sc7a20_waddr, reg_start_addr, 1, (uint8_t *)reg_array, num, 5000) != HAL_OK)
    {
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

void sc7a20_init(void)
{
    sc7a20_reg_write(0x20, &SC7A20_REG[0], 1, Sensor_Wr_Addr); // odr 10Hz 开启低功耗
    sc7a20_reg_write(0x21, &SC7A20_REG[1], 1, Sensor_Wr_Addr); // fds -->开启高通滤波器(滤掉地球G)(一定要开启，否则阈值要超过1G，而且动作也要超过1G)
    sc7a20_reg_write(0x23, &SC7A20_REG[2], 1, Sensor_Wr_Addr); // 2g量程

    sc7a20_reg_write(0x1e, &SC7A20_REG[3], 1, Sensor_Wr_Addr); // 开启控制开关
    sc7a20_reg_write(0x57, &SC7A20_REG[4], 1, Sensor_Wr_Addr); // 关闭sdo管脚上的上拉电阻

    // selects active level low for pin INT 正常是高电平，有效的时候是低电平
    sc7a20_reg_write(0x25, &SC7A20_REG[7], 1, Sensor_Wr_Addr); // int1 active low
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

void sc7a20_read_acc(int16_t *acc_data, float *combin_acc)
{
    uint8_t raw_data[6] = {0};
    uint8_t i;
    float temp = 0.0f;

    for (i = 0; i < 6; i++)
    {
        sc7a20_reg_read(0x28 + i, &raw_data[i], 1, Sensor_Rd_Addr); // 0x80表示自动地址递增
    }
    // sc7a20_reg_read(0x28 | 0x80, raw_data, 6, Sensor_Rd_Addr); // 0x80表示自动地址递增

    acc_data[0] = (int16_t)(raw_data[1] << 8 | raw_data[0]) >> 6;
    acc_data[1] = (int16_t)(raw_data[3] << 8 | raw_data[2]) >> 6;
    acc_data[2] = (int16_t)(raw_data[5] << 8 | raw_data[4]) >> 6;

    // 三轴合成加速度
    temp = sqrtf(acc_data[0] * acc_data[0] + acc_data[1] * acc_data[1] + acc_data[2] * acc_data[2]);

    count_base_g(temp);

    if (if_data_init == 0)
    {
        // 仅保留绝对值
        *combin_acc = fabsf(temp - base_g);
    }
    else
    {
        *combin_acc = 0.0f;
    }
}
