#include "main.h"

tick system_tick;
volatile uint8_t system_state = 0;
uint32_t exchange_data_num = 0;
uint8_t rx_buff_length = 0;
uint8_t rx_buff[64] = {0};

hal_nfc_regval_t COS_typeA_configs[36] =
    {
        {0, 0x01, 0x00}, // 打开接收，芯片处于Ready状态
        {0, 0x02, 0x00},
        {0, 0x03, 0x80}, // 最高位bit7置1，配置IRQ为推挽输出，根据实际场景可不配
        {0, 0x08, 0x00},
        {1, 0x11, 0x3D}, // 定义CRC_Processor执行CalcCRC指令的预置，A/M卡低两位写01；B卡低两位写11
        {1, 0x12, 0x00}, // 定义发送数据的帧结构
        {1, 0x13, 0x00}, // 定义接收数据的帧结构
        {1, 0x14, 0xC3},
        {1, 0x15, 0x40},  // 输出调制信号类型，bit6置1是OOK(A/V卡)，置0是ASK(B/F卡)
        {1, 0x17, 0x92},  // 配置接收等待时间
        {1, 0x1D, 0x64},  // 高通滤波器带宽影响刷卡
        {2, 0x26, 0x68},  // 手动增益调整值，配合第四页35寄存器
        {2, 0x28, 0x80},  // 发射场强调整
        {4, 0x35, 0xe8},  // 增益配置
        {4, 0x3F, 0x52},  // bit1：Rxwait扩展位使能
        {4, 0x3A, 0x23},  // 低三位配置相位补偿，IQ翻转影响刷卡
        {4, 0x3B, 0x65},  // bit6置1，EMV认证用；bit6置0，正常模式用；bit5:3 Bypass噪声EMV认证测灵敏度时建议置111
        {5, 0x34, 0x50},  // bit6置1：AWC关闭
        {6, 0x33, 0x10},  // 使能LPCD通道(ARC使能时配置)，可以实时读第6页3A寄存器反应接收信号大小
        {6, 0x3F, 0x70},  // bit6:4 A波下降沿调整
        {7, 0x31, 0x22},  // LPF增益控制位，严重影响刷卡，下版配置为02
        {7, 0x32, 0x72},  // 接收内部Rtune调整
        {7, 0x38, 0x0E},  // bit0=0关闭调制深度自适应功能
        {7, 0x3C, 0x36},  // bit1:单端差分选择
        {7, 0x3D, 0x2B},  // bit6:使能模拟相位校准与调整低通滤波器带宽，影响刷卡
        {7, 0x3F, 0x80},  // bit7:低通滤波器带宽使能；bit4选择V1/V2 RFCLK
        {8, 0x32, 0xD0},  // ARC上限阈值
        {8, 0x33, 0xB0},  // ARC下限阈值
        {8, 0x34, 0x69},  // ARC使能
        {8, 0x38, 0x93},  // 初始化是E3，调整到93过协议TA304
        {8, 0x39, 0xC6},  // 初始化是C8，调整到C6过协议TA304
        {9, 0x3A, 0x44},  // bit6：V1版本使能；bit5：V2版本使能。配合3E/3F选择V2版本，但是RFCLK拉低。否则容易死机
        {10, 0x31, 0x20}, // TA上升沿步长
        {10, 0x32, 0x00}, // TA上升沿步数
        {10, 0x33, 0xe0}, // TA下降沿步长
        {10, 0x34, 0x22}, // TA下降沿步数
};

hal_nfc_regval_t COS_typeB_configs[] =
    {
        {0, 0x01, 0x00}, // 打开接收，芯片处于Ready状态
        {0, 0x02, 0x00},
        {0, 0x03, 0x80}, // 最高位bit7置1，配置IRQ为推挽输出，根据实际场景可不配
        {0, 0x08, 0x00},
        {0, 0x0D, 0x00}, // 低三位只影响B卡且只能配置000；A卡无法修政
        {1, 0x11, 0x3F}, // 定义CRC_Processor执行CalcCRC指令的预置，A/M卡低两位写01；B卡低两位写11
        {1, 0x12, 0x83}, // 定义发送数据的帧结构
        {1, 0x13, 0x83}, // 定义接收数据的帧结构
        {1, 0x14, 0xC3},
        {1, 0x15, 0x00}, // 输出调制信号类型，置1是OOK(A/V；置0是ASK(B/F)
        {1, 0x17, 0x92}, // 接收等待时间
        {1, 0x1D, 0x64}, // 高通滤波器带宽影响刷卡
        {1, 0x1E, 0x03}, // B卡专用，SOF与EGT定义，新方案这一位有改动要配置成03
        {2, 0x26, 0x48}, // 手动增益调整值，配合第四页35寄存器
        {2, 0x28, 0xF0}, // P管发射场强调整
        {2, 0x29, 0x18}, // P管MOD信号调整，改变调制深度
        {4, 0x31, 0x0d}, // AD信号输出使能，Debug时用
        {4, 0x35, 0xE4}, // 增益配置
        {4, 0x3A, 0x24}, // 低三位配置相位补偿，IQ翻转影响刷卡
        {4, 0x3F, 0x52},
        {6, 0x33, 0x10},  // 使能LPCD通道(调制深度自适应使能时配置)，FPGA使用，SOC可以不用
        {7, 0x31, 0x22},  // LPF增益控制位，严重影响刷卡，下版配置为02
        {7, 0x32, 0x72},  // 接收内部Rtune调整
        {7, 0x38, 0x0E},  // bit0调制深度自适应使能,调不好影响EMD协议测试，最好关闭
        {7, 0x39, 0xD1},  // 目标调制深度设置
        {7, 0x3C, 0x36},  // bit1单端差分选择
        {7, 0x3D, 0x2B},  // bit6:使能模拟相位校准与调整低通滤波器带宽，影响刷卡
        {7, 0x3F, 0x80},  // bit7:低通滤波器带宽使能；bit4选择V1/V2 RFCLK
        {8, 0x32, 0xD0},  // ARC上限阈值
        {8, 0x33, 0xB0},  // ARC下限阈值
        {8, 0x34, 0x69},  // ARC使能
        {9, 0x3A, 0x44},  // bit6：V1版本使能；bit5：V2版本使能。配合3E/3F选择V2版本，但是RFCLK拉低。否则容易死机
        {10, 0x35, 0x20}, // TB上升沿步长
        {10, 0x36, 0xC0}, // TB上升沿步数
        {10, 0x38, 0x30}, // TB下降沿步长
        {10, 0x39, 0x03}, // TB下降沿步数
};

uint8_t tx_buff[64] = {0xaa, 0xaa, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                       3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
                       5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
                       0xcc, 0xcc};

void nfc_app(void)
{
    system_tick = get_tick(); // set init timer

    switch (system_state)
    {

    case STATE_READ_CARD:

        // NVIC_DisableIRQ(EXIT_IRQ_NUM);
        hal_nfc_pcd_reset();
        // printf("hal_nfc_pcd_reset\n");
        nfc_config(COS_typeA_configs, sizeof(COS_typeA_configs) / sizeof(hal_nfc_regval_t));

        if (is_timeout(system_tick, 500))
        {
            // printf("is_timeout exchange STATE_PICC_INIT()\n");
            hal_nfc_pcd_reset();
            HAL_Delay(200);
            // system_state = STATE_PICC_INIT;
            system_state = STATE_READ_CARD;
        }

        if (reader_mode() == 0) // read succeed
        {
            printf("reader_mode()==0\n");
            system_tick = get_tick();
            system_state++;
            // system_state=STATE_PICC_INIT;
        }
        break;

    case STATE_READER_EXCHANGTE_DATA:

        printf(" STATE_READER_EXCHANGTE_DATA1\n ");

        if (is_timeout(system_tick, 2000) || exchange_data_num >= SEND_DATA_NUM_MAX)
        {
            exchange_data_num = 0;
            printf("STATE_READER_EXCHANGE_DATA  END  !!! \n");
            // system_state++;
            system_state = STATE_READ_CARD;
        }
        if (apdu_exchange(tx_buff, sizeof(tx_buff), rx_buff, &rx_buff_length) == 0)
        {

            system_tick = get_tick();
            printf("rx_buff => %d,  %d\n", exchange_data_num, rx_buff_length);
            for (int i = 0; i < rx_buff_length; i++)
                printf(" %02x", rx_buff[i]);
            printf("\n");
            for (int i = 0; i < sizeof(rx_buff); i++)
            {
                rx_buff[i] = 0;
            }
        }
        else
        {
            system_state = STATE_READ_CARD;
            break;
        }
        exchange_data_num++; // send SEND_DATA_NUM_MAX packet number.
        break;

    case STATE_PICC_INIT:

        printf("STATE_PICC_INIT\n");
        // NVIC_DisableIRQ(EXIT_IRQ_NUM);
        hal_nfc_pcd_reset();

        set_picc_init();

        system_tick = get_tick();
        system_state++;
        break;

    case STATE_PICC:
        //  printf(" STATE_PICC ");
        if (is_timeout(system_tick, 100000))
        {
            printf("STATE_PICC\n");
            system_state++;
        }
        else
        {
            if (if_picc_state_active())
            {
                system_tick = get_tick();
            }
            picc_task();
        }

        break;

    case STATE_CLEAR:

        printf("STATE_CLEAR\n");
        if (nfc_check_bit(8, 0x3b, BIT1, 1))
        {
            system_tick = get_tick();
            system_state = STATE_PICC;
        }
        else
        {
            system_tick = get_tick();
            system_state = STATE_PICC; // STATE_READ_CARD;
        }
        break;

    default:
        break;
    }
}
