#include "main.h"

static uint8_t gsensor_wr_addr = 0x32;
static uint8_t gsensor_rd_addr = 0x33;

void init_sim_i2c_gpio(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin = I2C_SCL_PIN | I2C_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(I2C_SCL_PORT, &GPIO_InitStruct);

    HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, GPIO_PIN_SET);
}

void deinit_gsensor_gpio(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin = I2C_SCL_PIN | I2C_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(I2C_SCL_PORT, &GPIO_InitStruct);
}

void init_gsenor_sim(void)
{
    i2c_write_reg(0x20, 0x3f, gsensor_wr_addr);
    i2c_write_reg(0x23, 0x88, gsensor_wr_addr); // +-2g
    i2c_write_reg(0x21, 0x31, gsensor_wr_addr);
    i2c_write_reg(0x22, 0x40, gsensor_wr_addr); // AOI中断on int1
    i2c_write_reg(0x25, 0x00, gsensor_wr_addr); // H_LACTIVE=0, 中断高电平
    i2c_write_reg(0x24, 0x00, gsensor_wr_addr); // FIFO关闭, INT1非锁存, 4D检测关闭
    i2c_write_reg(0x30, 0x2a, gsensor_wr_addr); // XYZ高事件或检测
    i2c_write_reg(0x32, 0x05, gsensor_wr_addr); // 检测门限: 1-127, 值越小, 灵敏度越高
    i2c_write_reg(0x33, 0x00, gsensor_wr_addr); // 检测持续时间, 单位为ODR周期数, 0表示只要超过门限就触发

}

void init_gsensor_for_lp_int_sim(void)
{
    /*
     * SC7A20 低功耗中断配置。
     * 使用 AOI1 做运动中断，灵敏度主要由 AOI1_THS(0x32)
     * 和 AOI1_DURATION(0x33) 决定。
     */
    uint8_t lp_init_data[] = {
        0x1F, // 0x20 CTRL_REG1 = 0001 1111: ODR=0001(约1Hz), LPen=1(低功耗), Z/Y/X=1(三轴使能)
        0x00, // 0x23 CTRL_REG4 = 0000 0000: BDU=0, BLE=0(小端), FS=00(+/-2g), HR=0(非高分辨率)
        0x31, // 0x21 CTRL_REG2 = 0011 0001: HPM=00, HPCF=11, FDS=0, HPCLICK=0, HP_IA2=0, HP_IA1=1(AOI1用高通)
        0x40, // 0x22 CTRL_REG3 = 0100 0000: I1_AOI1=1, AOI1中断输出到INT1
        0x02, // 0x25 CTRL_REG6 = 0000 0010: H_LACTIVE=1, 中断低电平有效
        0x00, // 0x24 CTRL_REG5 = 0000 0000: FIFO关闭, INT1非锁存, 4D检测关闭
        0x2A, // 0x30 AOI1_CFG  = 0010 1010: AOI=0(OR逻辑), ZH/YH/XH=1, 三轴高方向触发
        0x08, // 0x32 AOI1_THS  = 0000 1000: 阈值=8LSB, +/-2g下约8*16mg=128mg
        0x01  // 0x33 AOI1_DURATION = 0000 0001: 超过阈值持续1个采样周期后触发
    };

    /* CTRL_REG1 (0x20): 输出速率、低功耗模式、XYZ轴使能。 */
    i2c_write_reg(0x20, lp_init_data[0], gsensor_wr_addr);

    /* CTRL_REG4 (0x23): 量程、字节序、数据更新方式、高分辨率开关。 */
    i2c_write_reg(0x23, lp_init_data[1], gsensor_wr_addr);

    /* CTRL_REG2 (0x21): 高通滤波配置，当前作用于AOI1中断。 */
    i2c_write_reg(0x21, lp_init_data[2], gsensor_wr_addr);

    /* CTRL_REG3 (0x22): 将AOI1中断路由到INT1引脚。 */
    i2c_write_reg(0x22, lp_init_data[3], gsensor_wr_addr);

    /* CTRL_REG6 (0x25): 中断电平极性等配置。 */
    i2c_write_reg(0x25, lp_init_data[4], gsensor_wr_addr);

    /* CTRL_REG5 (0x24): FIFO、INT1锁存、4D检测等配置。 */
    i2c_write_reg(0x24, lp_init_data[5], gsensor_wr_addr);

    /* AOI1_CFG (0x30): AOI1参与轴、方向和AND/OR逻辑配置。 */
    i2c_write_reg(0x30, lp_init_data[6], gsensor_wr_addr);

    /* AOI1_THS (0x32): 中断阈值，值越小越容易触发。 */
    // 0x70 = 112 * 16mg = 1.792g
    // 0x60 =  96 * 16mg = 1.536g
    // 0x40 =  64 * 16mg = 1.024g
    // 0x30 =  48 * 16mg = 0.768g
    // 0x20 =  32 * 16mg = 0.512g
    // 0x10 =  16 * 16mg = 0.256g
    // 0x08 =   8 * 16mg = 0.128g，当前配置
    i2c_write_reg(0x32, lp_init_data[7], gsensor_wr_addr);

    /* AOI1_DURATION (0x33): 超过阈值后需要持续的采样周期数。 */
    i2c_write_reg(0x33, lp_init_data[8], gsensor_wr_addr);
}

void sc7a20_get_data_sim(int16_t *xyz_val)
{
    uint8_t reg_start_addr = 0x28 | 0x80; // 0x80 for auto-increment
    // uint8_t reg_start_addr = 0x28 ; // 0x80 for auto-increment
    uint8_t reg_array[6] = {0};
    // void i2c_read_bytes(uint8_t RAddr, uint8_t *RData, uint8_t RLen, uint8_t slave_addr)
    // i2c_read_bytes(reg_start_addr, reg_array, 6, gsensor_wr_addr);

    i2c_read_bytes(reg_start_addr, reg_array, 6, gsensor_wr_addr);

    xyz_val[0] = (int16_t)((uint16_t)reg_array[1] << 8 | (uint16_t)reg_array[0]) >> 6;
    xyz_val[1] = (int16_t)((uint16_t)reg_array[3] << 8 | (uint16_t)reg_array[2]) >> 6;
    xyz_val[2] = (int16_t)((uint16_t)reg_array[5] << 8 | (uint16_t)reg_array[4]) >> 6;
}
