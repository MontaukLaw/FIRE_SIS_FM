#include "1002.h"
#include "main.h"
#include "I2C.h"

#define RM1002_DEBUG 0

#if RM1002_DEBUG
#define RM1002_LOG(...) printf(__VA_ARGS__)
#else
#define RM1002_LOG(...)
#endif

// 动态阈值
uint8_t cal_data[] = {0x1f, 0x2f, 0x4f, 0x8f};                             // 各通道的校准值
uint8_t cal_mask[] = {0x01, 0x02, 0x04, 0x08};                             // 各通道的掩码
volatile float last_c_value[SENSOR_NUM] = {0.0};                           // 上一帧传感器中间量
volatile float diff_c_value[SENSOR_NUM] = {0.0};                           // 差值
volatile float c_real_time_value[SENSOR_NUM] = {0.0};                      // 传感器中间量
volatile float pre_value[SENSOR_NUM] = {0.0};                              // 上一帧值
float parastic_cap[SENSOR_NUM] = {3.502045, 2.020049, 1.988817, 2.020808}; // 寄生电容值
// 3.502045

struct RM1X01_REG_CFG rm1002_defaultcfg[] = {
    {0x05, 0xf9},
    //  { 0x07, 0x0b },

    {0x0c, 0x40},
    {0x0d, 0x40},
    {0x0e, 0x40},
    {0x0f, 0x40},
    {0x14, 0x83},

    {0x16, 0x0f}, // 选择通道，CH0:0001  CH1:0010 CH2:0100 CH3:1000
    {0x17, 0x00}, // Scan周期配置
    {0x19, 0x06}, // ADC单次采样时间。值越小，变化越大
    {0x1a, 0x06}, // ADC单次积分时间。速度

    {0x1b, 0x00}, // CH0和CH1对应工作模式选择
    {0x1c, 0x00}, // CH2和CH3对应工作模式选择
                  //  { 0x1c, 0x38 },

    {0x1d, 0x02}, // CH0对应的物理通道配置
    {0x1e, 0x08}, // CH1对应的物理通道配置
    {0x1f, 0x20}, // CH2对应的物理通道配置
    {0x20, 0x80}, // CH3对应的物理通道配置

    {0x21, 0x03}, // 采样率，改小速度快，可能有噪声---0x01还可以
    {0x22, 0x03},
    {0x23, 0x03},
    {0x24, 0x03},

    {0x25, 0x00},
    {0x26, 0x00},
    {0x27, 0x00},
    {0x28, 0x00},
    {0x29, 0x00},

    {0x2a, 0x80},
    {0x2b, 0x81},
    {0x2c, 0x81},
    {0x2d, 0x81},
    {0x2e, 0x81},

    {0x2f, 0xff}, // 0x00：使用人工设置的offset，0xFF：使能自动补偿
    {0x30, 0x41},
    {0x31, 0xff & AVG_TARGET_COMP_CH0},
    {0x32, 0xff & (AVG_TARGET_COMP_CH0 >> 8)},
    {0x33, 0xff & AVG_TARGET_COMP_CH1},
    {0x34, 0xff & (AVG_TARGET_COMP_CH1 >> 8)},
    {0x35, 0xff & AVG_TARGET_COMP_CH2},
    {0x36, 0xff & (AVG_TARGET_COMP_CH2 >> 8)},
    {0x37, 0xff & AVG_TARGET_COMP_CH3},
    {0x38, 0xff & (AVG_TARGET_COMP_CH3 >> 8)},

    {0x39, 0x00},
    {0x3a, 0x00},
    {0x3b, 0x00},
    {0x3c, 0x00},

    {0x49, 0xff & PROXTH_CH0_CLS},
    {0x4a, 0xff & (PROXTH_CH0_CLS >> 8)},
    {0x4b, 0xff & PROXTH_CH1_CLS},
    {0x4c, 0xff & (PROXTH_CH1_CLS >> 8)},
    {0x4d, 0xff & PROXTH_CH2_CLS},
    {0x4e, 0xff & (PROXTH_CH2_CLS >> 8)},
    {0x4f, 0xff & PROXTH_CH3_CLS},
    {0x50, 0xff & (PROXTH_CH3_CLS >> 8)},

    {0x51, 0xff & PROXTH_CH0_FAR},
    {0x52, 0xff & (PROXTH_CH0_FAR >> 8)},
    {0x53, 0xff & PROXTH_CH1_FAR},
    {0x54, 0xff & (PROXTH_CH1_FAR >> 8)},
    {0x55, 0xff & PROXTH_CH2_FAR},
    {0x56, 0xff & (PROXTH_CH2_FAR >> 8)},
    {0x57, 0xff & PROXTH_CH3_FAR},
    {0x58, 0xff & (PROXTH_CH3_FAR >> 8)},

    {0x65, 0x00}, // 中断使能寄存器
    {0x66, 0x00},
    {0x67, 0x07}, // 中断配置寄存器0
    {0x68, 0x01}, // 中端配置寄存器1
};

// RM1002B系列连续写寄存器函数；建议一次性写寄存器不超过10个
static bool rm1002_reg_write(uint8_t reg_start_addr, uint8_t *reg_array, uint8_t num, uint8_t rm1002b_addr)
{
    if (HAL_I2C_Mem_Write(&I2cHandle, (uint16_t)rm1002b_addr << 1, reg_start_addr, 1, (uint8_t *)reg_array, num, 5000) != HAL_OK)
    {
        RM1002_LOG("rm1002 write fail addr:0x%02x reg:0x%02x err:0x%08lx\r\n",
                   rm1002b_addr,
                   reg_start_addr,
                   (unsigned long)HAL_I2C_GetError(&I2cHandle));
        return false;
    }
    while (HAL_I2C_GetState(&I2cHandle) != HAL_I2C_STATE_READY)
        ;

    return true;
}

void stop_rm1002(void)
{
    uint8_t tx_data[3] = {0x0f, 0x00, 0x80};

    rm1002_reg_write(0x17, tx_data, 1, DEFAULE_I2C_ADDR_3);
    rm1002_reg_write(0x16, tx_data + 1, 1, DEFAULE_I2C_ADDR_3);
    rm1002_reg_write(0x15, tx_data + 2, 1, DEFAULE_I2C_ADDR_3);
}

// RM1002B系列连续读寄存器；建议一次性读寄存器不超过3个
static bool rm1002_reg_read(uint8_t reg_start_addr, uint8_t *reg_array, uint8_t num, uint8_t rm1002b_addr)
{
    if (HAL_I2C_Mem_Read(&I2cHandle, (uint16_t)rm1002b_addr << 1, reg_start_addr, 1, (uint8_t *)reg_array, num, 5000) != HAL_OK)
    {
        RM1002_LOG("rm1002 read fail addr:0x%02x reg:0x%02x err:0x%08lx\r\n",
                   rm1002b_addr,
                   reg_start_addr,
                   (unsigned long)HAL_I2C_GetError(&I2cHandle));
        return false;
    }
    while (HAL_I2C_GetState(&I2cHandle) != HAL_I2C_STATE_READY)
        ;
    return true;
}

static int32_t sign24To32(uint32_t bit24)
{
    if ((bit24 & 0x800000) == 0x800000)
    {
        bit24 |= 0xff000000;
    }
    return bit24;
}

uint32_t rm1002_adc_data_get(int dataType, int channel, uint8_t rm1002b_addr)
{
    uint8_t rx_buf[3] = {0};
    uint8_t reg_base_addr;
    uint32_t ret_data = 0;

    switch (dataType)
    {
    case RAW_DATA:
        reg_base_addr = channel * 3 + RM1X01_RAW0_CH0;
        break;
    case USE_DATA:
        reg_base_addr = channel * 3 + RM1X01_USE0_CH0;
        break;
    case AVG_DATA:
        reg_base_addr = channel * 3 + RM1X01_AVG0_CH0;
        break;
    case DIF_DATA:
        reg_base_addr = channel * 3 + RM1X01_DIF0_CH0;
        break;
    default:
        break;
    }

    if (rm1002_reg_read(reg_base_addr, rx_buf, 3, rm1002b_addr) == true)
    {
        ret_data = rx_buf[0] | (rx_buf[1] << 8) | (rx_buf[2] << 16);
    }

    return ret_data;
}

static void rm1002_setconfig(uint8_t rm1002b_addr)
{
    int i = 0;
    uint8_t tx_buf[4] = {0};

    for (i = 0; i < sizeof(rm1002_defaultcfg) / sizeof(rm1002_defaultcfg[0]); i++)
    {
        tx_buf[0] = rm1002_defaultcfg[i].reg_value;
        rm1002_reg_write(rm1002_defaultcfg[i].reg_address, tx_buf, 1, rm1002b_addr);
    }
}

void rm1002_init(uint8_t rm1002_addr)
{
    uint8_t rxbuf[4] = {0};
    uint8_t wdata[8] = {0x80, 0x80, 0x80, 0x80, 0x7f, 0x7f, 0x7f, 0x7f};
    uint8_t data = 0x00;
    bool ok;

#if RM1002_DEBUG
    RM1002_LOG("rm1002 init target:0x%02x state:%d err:0x%08lx\r\n",
               rm1002_addr,
               HAL_I2C_GetState(&I2cHandle),
               (unsigned long)HAL_I2C_GetError(&I2cHandle));
    RM1002_LOG("rm1002 ready default 0x2b:%d target 0x%02x:%d\r\n",
               HAL_I2C_IsDeviceReady(&I2cHandle, (uint16_t)0x2b << 1, 3, 50),
               rm1002_addr,
               HAL_I2C_IsDeviceReady(&I2cHandle, (uint16_t)rm1002_addr << 1, 3, 50));
#endif

    data = 0xcc;
    ok = rm1002_reg_write(0x6f, &data, 1, rm1002_addr);
    RM1002_LOG("rm1002 soft reset target 0x%02x ok:%d\r\n", rm1002_addr, ok);
    HAL_Delay(10);

    // RM1002B复位后地址都是0x2b，根据目标地址写入相应配置值
    switch (rm1002_addr)
    {
    case DEFAULE_I2C_ADDR_0:
        data = 0x03; // 11   2b
        break;
    case DEFAULE_I2C_ADDR_1:
        data = 0x02;
        break;
    case DEFAULE_I2C_ADDR_2:
        data = 0x01;
        break;
    case DEFAULE_I2C_ADDR_3:
        data = 0x00;
        break;
    default:
        return; // 无效地址，直接返回
    }

    ok = rm1002_reg_write(0x59, &data, 1, 0x2b);
    RM1002_LOG("rm1002 set addr via 0x2b data:0x%02x ok:%d\r\n", data, ok);
    HAL_Delay(10);

#if RM1002_DEBUG
    RM1002_LOG("rm1002 ready after set default 0x2b:%d target 0x%02x:%d\r\n",
               HAL_I2C_IsDeviceReady(&I2cHandle, (uint16_t)0x2b << 1, 3, 50),
               rm1002_addr,
               HAL_I2C_IsDeviceReady(&I2cHandle, (uint16_t)rm1002_addr << 1, 3, 50));
#endif

    ok = rm1002_reg_read(0x70, rxbuf, 1, rm1002_addr);
    if (ok == false)
    {
        // 读取who am I 地址
        RM1002_LOG("read id error target:0x%02x err:0x%08lx\r\n",
                   rm1002_addr,
                   (unsigned long)HAL_I2C_GetError(&I2cHandle));
        return;
    }
    else
    {
        RM1002_LOG("rm1002 id: 0x%02x\r\n", rxbuf[0]);
    }

    data = 0x00;
    rm1002_reg_write(0x17, &data, 1, rm1002_addr);
    HAL_Delay(10);
    rm1002_setconfig(rm1002_addr);
    HAL_Delay(10);

    data = 0xff;
    rm1002_reg_write(0x2f, &data, 1, rm1002_addr);
    HAL_Delay(10);

    rm1002_reg_write(0x39, wdata, 8, rm1002_addr);
    HAL_Delay(10);
}

/**
 * @brief 超量程校准
 *
 * @param rawdata
 * @param rm1002_addr
 * @param start_idx
 * @return void
 */
void cali(int32_t *rawdata, uint8_t rm1002_addr, uint8_t start_idx)
{
    uint32_t cali_start_time_ts = 0; // 校准开始时间
    uint16_t i;                      // 通道索引
    for (i = 0; i < 4; i++)
    {
        if (rawdata[i] > 8300000 || rawdata[i] < 20000) // 数据超出范围
        {
            rm1002_reg_write(0x2f, &cal_data[i], 1, rm1002_addr); // 写入校准值
            uint8_t status = 0;
            cali_start_time_ts = HAL_GetTick();
            while (!(status & cal_mask[i]))
            {
                rm1002_reg_read(0x73, &status, 1, rm1002_addr); // 校准完成标志位
                // if ((HAL_GetTick() - cali_start_time_ts) > TX_MSG_ITERVAL_MS) // 超时处理
                // {
                //     // 预判一下趋势
                //     if (diff_c_value[i + start_idx] > 0)
                //     {
                //         c_real_time_value[i + start_idx] = c_real_time_value[i + start_idx] * INCREASE_PER;
                //     }
                //     else if (diff_c_value[i + start_idx] < 0)
                //     {
                //         c_real_time_value[i + start_idx] = c_real_time_value[i + start_idx] * DECREASE_PER;
                //     }
                //     feed_dog();                            // 喂狗
                //     // send_data_with_lfo(c_real_time_value); // 发送数据并做低通滤波
                //     cali_start_time_ts = HAL_GetTick();
                // }
            }
        }
    }
}

/**
 * @brief 收集1002的电容值 v1.0
 *
 * @param rm1002_addr
 * @param cxxx0
 * @param cxxx1
 * @param cxxx2
 * @param cxxx3
 * @return float*
 */
void collect_1002_data(uint8_t rm1002_addr, uint8_t start_idx)
{
    int32_t rawdata[4] = {0};
    uint16_t i = 0;
    uint8_t cxoff[4] = {0};
    float temp_f = 0.0f;

    for (i = 0; i < 4; i++)
    {
        rawdata[i] = sign24To32(rm1002_adc_data_get(RAW_DATA, i, rm1002_addr));
    }
    cali(rawdata, rm1002_addr, start_idx);
    rm1002_reg_read(0x41, cxoff, 4, rm1002_addr);
    for (i = 0; i < 4; i++)
    {
        temp_f = rawdata[i] / 1677721.0;
        temp_f = temp_f + (float)(256 - cxoff[i]) * 0.2f;

        // 测量值减去寄生电容, 就是(串联电容+传感器电容)串联的电容值, 因为跟寄生是并联关系, 所以直接减去
        temp_f = temp_f - parastic_cap[i + start_idx];

        // 据串联电容公式, 计算传感器值
        if (SERIALS_CAP <= temp_f)
        {
            temp_f = 0.0f;
        }
        else
        {
            temp_f = (SERIALS_CAP * temp_f) / (SERIALS_CAP - temp_f);
        }

        // 如果差异太大, 就使用之前的结果, 加上旧变化量
        // if (temp_f > 0 && diff < MAX_C_CHANGE_PER_FRAME)
        if (temp_f > 0)
        {
            c_real_time_value[i + start_idx] = temp_f;
            // 计算差值
            diff_c_value[i + start_idx] = c_real_time_value[i + start_idx] - last_c_value[i + start_idx];
            // 保存旧值
            last_c_value[i + start_idx] = temp_f;
        }
    }
}

/**
 * @brief 读取所有1002的电容值
 *
 */
void read_all_rm1002_sensors(void)
{
    collect_1002_data(DEFAULE_I2C_ADDR_0, 0);
    collect_1002_data(DEFAULE_I2C_ADDR_1, 4);
    collect_1002_data(DEFAULE_I2C_ADDR_2, 8);
    collect_1002_data(DEFAULE_I2C_ADDR_3, 12);
}

void read_rmof_addres3(void)
{
    collect_1002_data(DEFAULE_I2C_ADDR_3, 0);
}

void print_1002_data(void)
{
    uint16_t i = 0;
    for (i = 0; i < SENSOR_NUM; i++)
    {
        printf("c%d: %f ", i, c_real_time_value[i]);
    }
    printf("\r\n");
}

void get_parastic_cap(uint8_t rm1002_addr, uint8_t start_idx)
{
    int32_t rawdata[4] = {0};
    uint16_t i = 0;
    uint8_t cxoff[4] = {0};
    float temp_f = 0.0f;

    float c_stay_init[4] = {0.0, 0.0, 0.0, 0.0}; // 1002测出总电容值仅供观察使用

    for (i = 0; i < 4; i++)
    {
        rawdata[i] = sign24To32(rm1002_adc_data_get(RAW_DATA, i, rm1002_addr));
    }
    cali(rawdata, rm1002_addr, start_idx);
    rm1002_reg_read(0x41, cxoff, 4, rm1002_addr);

    for (i = 0; i < 4; i++)
    {
        temp_f = rawdata[i] / 1677721.0;
        temp_f = temp_f + (float)(256 - cxoff[i]) * 0.2f;

        // 这里就是测量值
        c_stay_init[i] = temp_f;

        // 此时测量值
        // 计算寄生电容值
        parastic_cap[i + start_idx] = temp_f - SERIALS_CAP;
    }
}

void print_parastic_cap(void)
{
    uint16_t i = 0;
    for (i = 0; i < SENSOR_NUM; i++)
    {
        printf("parastic cap %d: %f ", i, parastic_cap[i]);
    }
    printf("\r\n");
}

void get_all_parastic_cap(void)
{
    get_parastic_cap(DEFAULE_I2C_ADDR_3, 0);
    print_parastic_cap();
}

