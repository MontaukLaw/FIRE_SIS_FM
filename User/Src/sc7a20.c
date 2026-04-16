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

// ===============================
// SC7A20H 震动/运动中断配置（AOI1 -> INT1）
// 用途：震动唤醒 MCU / 动作触发中断
// 模式：非锁存（电平型），MCU 用上升沿 EXTI 捕获
// ===============================
void init_gsensor_for_interrupt(void)
{

    uint8_t init_data[16] = {0x3f, 0x88, 0x31, 0x40, 0x00, 0x00, 0x2a, 0x08, 0x02};

    // CTRL_REG1 (0x20)
    // ODR=25Hz, 低功耗模式, XYZ 三轴使能
    // → 25Hz = 40ms/采样，决定 duration 的时间基准
    sc7a20_reg_write(0x20, &init_data[0], 1, Sensor_Wr_Addr);

    // CTRL_REG4 (0x23)
    // BDU=1（高低字节锁存，防止读数撕裂）
    // FS=±2g（灵敏度最高，适合震动检测）
    // DLPF 部分开启（简单滤波）
    sc7a20_reg_write(0x23, &init_data[1], 1, Sensor_Wr_Addr); // 2g量程

    // CTRL_REG2 (0x21)
    // HPIS1=1 → AOI1 使用高通滤波后的数据
    // → 过滤重力（静态），只关注“变化/震动”
    sc7a20_reg_write(0x21, &init_data[2], 1, Sensor_Wr_Addr);

    // CTRL_REG3 (0x22)
    // I1_AOI1=1 → AOI1 中断输出到 INT1 引脚
    // → INT1 用于连接 MCU EXTI 唤醒/中断
    sc7a20_reg_write(0x22, &init_data[3], 1, Sensor_Wr_Addr);

    // CTRL_REG6 (0x25)
    // INT2 不使用
    // INT 引脚：推挽输出、默认极性（触发=高电平）
    sc7a20_reg_write(0x25, &init_data[4], 1, Sensor_Wr_Addr);

    // CTRL_REG5 (0x24)
    // LIR_INT1=0 → 非锁存中断（推荐）
    // → 条件成立时 INT=1，条件消失 INT 自动恢复
    // → MCU 用“上升沿”捕获即可
    sc7a20_reg_write(0x24, &init_data[5], 1, Sensor_Wr_Addr);

    // AOI1_CFG (0x30)
    // 0x2A = 0010 1010b
    // XHIE=1, YHIE=1, ZHIE=1 → 三轴“高事件”检测
    // AOI=0 → OR 逻辑（任意一轴超过门限就触发）
    // → 典型“震动检测”配置
    sc7a20_reg_write(0x30, &init_data[6], 1, Sensor_Wr_Addr);

    // AOI1_THS (0x32)
    // 门限值（1LSB ≈ 16mg @ ±2g）
    // 0x08 ≈ 128mg
    // 1 LSB ≈ 16 mg
    // → 比 0x05 稍稳，减少误触发（推荐起点）
    sc7a20_reg_write(0x32, &init_data[7], 1, Sensor_Wr_Addr);

    // AOI1_DURATION (0x33)
    // 触发需持续的采样周期数
    // 当前 ODR=25Hz → 1周期≈40ms
    // 0x01 → 需要持续约 40ms 才触发
    // 0x02 → 需要持续约 80ms 才触发
    // → 过滤瞬间毛刺，比 0 更稳定
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
