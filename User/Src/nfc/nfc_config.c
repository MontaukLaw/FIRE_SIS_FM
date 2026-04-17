#include "main.h"

uint8_t EMV_TEST = 0;

/* 关闭载波 */
void nfc_antenna_off(void)
{
    nfc_set_reg(0, 0x14, 0, 1);
}

/* 开启载波 */
void nfc_antenna_on(void)
{
    nfc_set_reg(0, 0x14, 0x08, 1);
}

void pcd_delay_sfgi(uint8_t sfgi)
{
    // SFGT = (SFGT+dSFGT) = [(256 x 16/fc) x 2^SFGI] + [384/fc x 2^SFGI]
    // dSFGT =  384 x 2^FWI / fc
    hal_nfc_write_register(REG_TPRESCALER, (TP_FWT_302us + TP_dFWT) & 0xFF);
    hal_nfc_write_register(REG_TMODE, BIT(7) | (((TP_FWT_302us + TP_dFWT) >> 8) & 0xFF));

    if (sfgi > 14 || sfgi < 1)
    { // FDTA,PCD,MIN = 6078 * 1 / fc
        sfgi = 1;
    }

    hal_nfc_write_register(REG_TRELOADL, (1 << sfgi) & 0xFF);
    hal_nfc_write_register(REG_TRELOADH, ((1 << sfgi) >> 8) & 0xFF);

    hal_nfc_write_register(REG_COMIRQ, 0x7F); // 清除中断
    hal_nfc_write_register(REG_COMIEN, BIT(0));
    hal_nfc_clear_register_bit(REG_TMODE, BIT(7)); // clear TAuto
    hal_nfc_set_register_bit(REG_CONTROL, BIT(6)); // set TStartNow

    while (HAL_GPIO_ReadPin(G_SENSOR_INT_PORT, G_SENSOR_INT_PIN) == GPIO_PIN_RESET)
        ; // wait new INT
    // hal_nfc_set_register_bit(TModeReg,BIT(7));// recover TAuto
    pcd_set_tmo(g_pcd_module_info.ui_fwi); // recover timeout set
}

void pcd_set_tmo(uint8_t fwi)
{
    uint32_t tmp;

    if (EMV_TEST)
    {
        // FDT = FWT + dFWT
        hal_nfc_write_register(REG_TPRESCALER, (TP_FWT_302us /* + TP_dFWT*/) & 0xFF);
        hal_nfc_write_register(REG_TMODE, BIT(7) | (((TP_FWT_302us /* + TP_dFWT*/) >> 8) & 0xFF));
        if (fwi > 14)
        {
            fwi = 4;
        }

        tmp = g_pcd_module_info.uc_wtxm * (1 << fwi);

        if (tmp > (1 << 14))
        { // FDTA,PICC,EXT <= (FWT+dFWT)Max //EMV CL 10.2.2.3 10.2.2.7
            tmp = (1 << 14);
        }
        tmp += (25 + 12); // 25+12 = (FC_dTPCD + FC_dFWT)/4096; //TA204 ±¾À´¾Í²»Ó¦¸ÃÍ¨¹ý//20211203_EMV3.1ÐÂÔö"+20"£¬²Å¿ÉÍ¨¹ýTA412¼°Ò»ÏµÁÐcase
        hal_nfc_write_register(REG_TRELOADL, tmp & 0xFF);
        hal_nfc_write_register(REG_TRELOADH, (tmp & 0xFF00) >> 8);
    }
    else
    {
        tmp = 1 * (1 << fwi);

        hal_nfc_write_register(REG_TPRESCALER, (TP_FWT_302us) & 0xFF);
        hal_nfc_write_register(REG_TMODE, BIT(7) | (((TP_FWT_302us) >> 8) & 0xFF));

        hal_nfc_write_register(REG_TRELOADL, tmp & 0xFF);
        hal_nfc_write_register(REG_TRELOADH, (tmp & 0xFF00) >> 8);
    }
}

void nfc_config(hal_nfc_regval_t *config, uint8_t size)
{
    uint8_t i = 0;

    nfc_antenna_off();
    HAL_Delay(7);

    nfc_set_reg((config + i)->page, (config + i)->addr, (config + i)->value, 1);

    for (i = 1; i < size; i++)
    {
        if ((config + i)->addr == 0)
            break;

        nfc_set_reg((config + i)->page, (config + i)->addr, (config + i)->value, ((config + i)->page != (config + i - 1)->page));
    }
}

void pcd_set_tmo_FWT_n9(void)
{
    //(n x 128 + 84) / fc           ,   n = 9 ,  9*128+84=1236 clock  < 92 us, dN9 = 0.4 us = 5.4 clock
    // 这个设置有点小，暂且以0.604 us作为FWT£，也可改为302us试一下
    // (0.302 ms) FWI=0
    nfc_write_reg(REG_TPRESCALER, 0x7F); // T=18.80 us
    nfc_write_reg(REG_TMODE, 0x80);
    nfc_write_reg(REG_TRELOADL, 0x0F); //  18.80 * (Reload + 1)
    nfc_write_reg(REG_TRELOADH, 0);
}

/*
 * @brief pcd_set_rate
 * 设置非接通信速率（A/B/F）
 *
 * @param
 *      rate: 速率
 *              '1'= 106kbps
 *              '2'= 212kbps
 *              '4'= 424kbps
 *              '8'= 848kbps
 *      type: 协议类型（A/B/F）
 *
 * @return: None
 */
void pcd_set_rate(uint8_t rate, uint8_t type)
{
    uint8_t val, rxwait;
    switch (rate)
    {
    case '1':
        nfc_clear_reg_bit(REG_TXMODE, BIT(4) | BIT(5) | BIT(6));
        nfc_clear_reg_bit(REG_RXMODE, BIT(4) | BIT(5) | BIT(6));
        nfc_write_reg(REG_MODWIDTH, 0x26); // Miller Pulse Length

        nfc_write_reg(REG_RXSEL, 0x88);

        break;

    case '2':
        nfc_clear_reg_bit(REG_TXMODE, BIT(4) | BIT(5) | BIT(6));
        nfc_set_reg_bit(REG_TXMODE, BIT(4));
        nfc_clear_reg_bit(REG_RXMODE, BIT(4) | BIT(5) | BIT(6));
        nfc_set_reg_bit(REG_RXMODE, BIT(4));
        nfc_write_reg(REG_MODWIDTH, 0x12); // Miller Pulse Length
        // rxwait相对于106基本速率需要增加相应倍数
        nfc_read_reg(REG_RXSEL, &val);
        rxwait = ((val & 0x3F) * 2);
        if (rxwait > 0x3F)
        {
            rxwait = 0x3F;
        }
        nfc_write_reg(REG_RXSEL, (rxwait | (val & 0xC0)));
        if (type == 'F')
        {
            nfc_write_reg(REG_RXSEL, 0x98);
            nfc_write_reg(REG_MODWIDTH, 0x15);
        }
        break;

    case '4':
        nfc_clear_reg_bit(REG_TXMODE, BIT(4) | BIT(5) | BIT(6));
        nfc_set_reg_bit(REG_TXMODE, BIT(5));
        nfc_clear_reg_bit(REG_RXMODE, BIT(4) | BIT(5) | BIT(6));
        nfc_set_reg_bit(REG_RXMODE, BIT(5));
        nfc_write_reg(REG_MODWIDTH, 0x0A); // Miller Pulse Length
        // rxwait相对于106基本速率需要增加相应倍数
        nfc_read_reg(REG_RXSEL, &val);
        rxwait = ((val & 0x3F) * 4);
        if (rxwait > 0x3F)
        {
            rxwait = 0x3F;
        }
        nfc_write_reg(REG_RXSEL, (rxwait | (val & 0xC0)));
        if (type == 'B')
        {
            nfc_write_reg(0x37, 0xAE);
            nfc_write_reg(0x32, 0x6D);
            nfc_write_reg(0x37, 0x00);
        }
        else if (type == 'F') // ZZQ20220304-->F此处仅配置了速率，CRC及felica使能请在初始化进行配置
        {
            nfc_write_reg(REG_RXSEL, 0x98);
            nfc_write_reg(REG_MODWIDTH, 0x0A);
        }
        break;
    case '8':
        nfc_clear_reg_bit(REG_TXMODE, BIT(4) | BIT(5) | BIT(6));
        nfc_set_reg_bit(REG_TXMODE, BIT(4) | BIT(5));
        nfc_clear_reg_bit(REG_RXMODE, BIT(4) | BIT(5) | BIT(6));
        nfc_set_reg_bit(REG_RXMODE, BIT(4) | BIT(5));
        if (type == 'B')
        {
            nfc_write_reg(0x37, 0xAE);
            nfc_write_reg(0x32, 0x6D);
            nfc_write_reg(0x37, 0x00);
        }
        nfc_write_reg(REG_MODWIDTH, 0x04); // Miller Pulse Length
        // rxwait相对于106基本速率需要增加相应倍数
        nfc_read_reg(REG_RXSEL, &val);
        rxwait = ((val & 0x3F) * 8);
        if (rxwait > 0x3F)
        {
            rxwait = 0x3F;
        }
        nfc_write_reg(REG_RXSEL, (rxwait | (val & 0xC0)));

        break;

    default:
        nfc_clear_reg_bit(REG_TXMODE, BIT(4) | BIT(5) | BIT(6));
        nfc_clear_reg_bit(REG_RXMODE, BIT(4) | BIT(5) | BIT(6));
        nfc_write_reg(REG_MODWIDTH, 0x26); // Miller Pulse Length

        break;
    }
}

void pcd_set_tmo_FWT_ATQB(void)
{
    uint32_t tmp;
    // 4096
    hal_nfc_write_register(REG_TPRESCALER, (TP_FWT_302us /* + TP_dFWT*/) & 0xFF);
    hal_nfc_write_register(REG_TMODE, BIT(7) | (((TP_FWT_302us /* + TP_dFWT*/) >> 8) & 0xFF));
    /* tmp = 27 太长了，所以在FWT-ATQB中FC_dTPCD不能太长 */
    tmp = 2; //(7680 + FC_dTPCD)/4096 =  1.875 + 25; //FWTACTIVATION + ΔTPCD
    tmp = 4; // 针对NVC_V15 需要增加这个时间
    hal_nfc_write_register(REG_TRELOADL, tmp & 0xFF);
    hal_nfc_write_register(REG_TRELOADH, (tmp & 0xFF00) >> 8);
}

void pcd_set_tmo_FWT_ACTVITION(void)
{
    uint32_t tmp;
    // 4096
    hal_nfc_write_register(REG_TPRESCALER, (TP_FWT_302us /* + TP_dFWT*/) & 0xFF);
    hal_nfc_write_register(REG_TMODE, BIT(7) | (((TP_FWT_302us /* + TP_dFWT*/) >> 8) & 0xFF));
    tmp = 42; //(71680 + FC_dTPCD)/4096 =  17.5 + 25; //FWTACTIVATION + ¦¤TPCD

    hal_nfc_write_register(REG_TRELOADL, tmp & 0xFF);
    hal_nfc_write_register(REG_TRELOADH, (tmp & 0xFF00) >> 8);
}
