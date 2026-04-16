#include "main.h"

/* 关闭载波 */
static inline void nfc_antenna_off(void)
{
    nfc_set_reg(0, 0x14, 0, 1);
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
