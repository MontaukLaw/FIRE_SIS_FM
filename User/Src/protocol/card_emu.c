#include "main.h"
// PICC = Proximity Integrated Circuit Card（近距离集成电路卡）

hal_nfc_regval_t picc_configs[] = {
    {0, 0x01, 0x00},
    {0, 0x02, 0x00},
    {0, 0x03, 0x80},
    {1, 0x12, 0x80},
    {1, 0x13, 0x80},
    {1, 0x14, 0x00},   // 关闭发射
    {2, 0x28, GSPOUT}, // 设置发射能量
    {7, 0x32, 0x70},
    {8, 0x3b, 0x01},  // mag detect enable
    {9, 0x3a, 0x6d},  // picc config,except picc enable
    {9, 0x3b, 0x11},  // 设置rxdata解调上下限
    {9, 0x3d, 0xf0},  // 设置rx增益
    {9, 0x3e, 0xb3},  // 选择解调方案
    {10, 0x3a, 0x00}, // 场检测时间500us
};

struct picc_config_t picc_conf = {
    .uid = {0xAC, 0xDC, 0x2A, 0x6A, 0x64, 0x34, 0x80},
    .coll_level = 2,
    .enable_14443_4 = 0,
    .atqa = 0x0044,
    .ats = {
        .TL = sizeof(struct picc_ats),
        .T0 = 0x75,
        .TA = 0x77,
        .TB = 0x81,
        .TC = 0x02,
    },
};

void picc_init(void)
{
    // 把 PICC 相关状态机、寄存器、FIFO 或上下文恢复到初始状态。
    picc_reset();

    // 加载一套 PICC 专用寄存器配置表。
    nfc_config(picc_configs, sizeof(picc_configs) / sizeof(hal_nfc_regval_t));

    // 设置卡 UID
    picc_uid_set(picc_conf.uid, picc_conf.coll_level == 2 ? 7 : 4);

    // ATQA 是 Type A 卡在防冲突初期返回给读卡器的信息，所以这句已经非常能说明：
    // 这就是在配置“我这张卡要怎么回应 REQA / WUPA”。
    picc_set_atqa_hbyte(picc_conf.atqa >> 8);
    picc_set_atqa_lbyte(picc_conf.atqa & 0x1F);

    // 决定要不要支持 ISO14443-4。
    picc_14443_4_enable(picc_conf.enable_14443_4);

    picc_field_in_irq(1);
    picc_field_out_irq(1);

    picc_active_irq(1);

    // picc_rx_irq(1);
}

