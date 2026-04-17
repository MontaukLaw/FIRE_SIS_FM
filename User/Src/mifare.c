/**
 ****************************************************************
 * @file mifare.c
 *
 * @brief  mifare protocol driver
 *
 * @author
 *
 *
 ****************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************
 */
#include "main.h"

extern uint8_t NFC_DEBUG_LOG;

/*
 * @brief: 用存放在FIFO中的密钥和卡上的密钥进行验证
 *
 * @param:
 *      auth_mode:      验证方式
 *                      0x60: 验证A密钥
 *                      0x61: 验证B密钥
 *      block:          要验证的绝对块号
 *      psnr:           序列号首地址
 *      pkey:           密钥
 *
 * @return: HAL_STATUS_OK成功，否则失败；
 */
int pcd_auth_state(uint8_t auth_mode, uint8_t block, uint8_t *psnr, uint8_t *pkey)
{
    int status;
    uint8_t i;

    hal_nfc_transceive_t pi;

    if (NFC_DEBUG_LOG)
    {
        printf("AUTH:\n");
    }

    nfc_write_reg(REG_BITFRAMING, 0x00); // // Tx last bits = 0, rx align = 0

    pcd_set_tmo(4);

    pi.mf_command = PCD_AUTHENT;
    pi.mf_length = 12;
    pi.mf_data[0] = auth_mode;
    pi.mf_data[1] = block;
    for (i = 0; i < 6; i++)
    {
        pi.mf_data[2 + i] = pkey[i];
    }
    memcpy(&pi.mf_data[8], psnr, 4);

    status = pcd_com_transceive(&pi);

    uint8_t temp = 0;
    if (HAL_STATUS_OK == status)
    {
        nfc_read_reg(REG_STATUS2, &temp);
        if (temp & BIT(3)) // MFCrypto1On
        {
            status = HAL_STATUS_OK;
        }
        else
        {
            status = HAL_STATUS_AUTH_ERR;
            g_statistics.authenticate_fail++;
        }
    }

    return status;
}

/*
 * @brief: 读mifare_one卡上一块（block）数据（16字节）
 *
 * @param:
 *      addr:       要读的绝对块号
 *      *preaddata  存放读出的数据缓存区首地址
 *
 * @return: HAL_STATUS_OK成功，否则失败；
 */
int pcd_read(uint8_t addr, uint8_t *preaddata)
{
    int status;

    hal_nfc_transceive_t pi;

    if (NFC_DEBUG_LOG)
    {
        printf("READ:\n");
    }

    nfc_write_reg(REG_BITFRAMING, 0x00); // // Tx last bits = 0, rx align = 0
    nfc_set_reg_bit(REG_TXMODE, BIT(7)); // 使能发送crc
    nfc_set_reg_bit(REG_RXMODE, BIT(7)); // 使能接收crc
    pcd_set_tmo(4);

    pi.mf_command = PCD_TRANSCEIVE;
    pi.mf_length = 2;
    pi.mf_data[0] = PICC_READ;
    pi.mf_data[1] = addr;

    status = pcd_com_transceive(&pi);
    if (status == HAL_STATUS_OK)
    {
        if (pi.mf_length != 0x80)
        {
            status = HAL_STATUS_BITCOUNT_ERR;
        }
        else
        {
            memcpy(preaddata, &pi.mf_data[0], 16);
        }
    }

    return status;
}

/*
 * @brief: 写数据到卡上的一块（block）
 *
 * @param:
 *      addr:       要写的绝对块号
 *      pwritedata  存放写的数据缓存区的首地址
 *
 * @return: HAL_STATUS_OK成功，否则失败；
 */
int pcd_write(uint8_t addr, uint8_t *pwritedata)
{
    int status;

    hal_nfc_transceive_t pi;

    if (NFC_DEBUG_LOG)
    {
        printf("WRITE:\n");
    }

    nfc_write_reg(REG_BITFRAMING, 0x00);            // // Tx last bits = 0, rx align = 0
    nfc_set_reg_bit(REG_TXMODE, BIT(7));            // 使能发送crc
    hal_nfc_clear_register_bit(REG_RXMODE, BIT(7)); // 不使能接收crc

    pcd_set_tmo(5);

    pi.mf_command = PCD_TRANSCEIVE;
    pi.mf_length = 2;
    pi.mf_data[0] = PICC_WRITE;
    pi.mf_data[1] = addr;

    status = pcd_com_transceive(&pi);
    if (status != HAL_STATUS_TIMEOUT)
    {
        if (pi.mf_length != 4)
        {
            status = HAL_STATUS_BITCOUNT_ERR;
        }
        else
        {
            pi.mf_data[0] &= 0x0F;
            switch (pi.mf_data[0])
            {
            case 0x00:
                printf("\r\nmi w err\r\n");
                status = HAL_STATUS_NOAUTH_ERR;
                break;
            case 0x0A:
                status = HAL_STATUS_OK;
                break;
            default:
                printf("\r\nmi w err\r\n");
                status = HAL_STATUS_CODE_ERR;
                break;
            }
        }
    }
    if (status == HAL_STATUS_OK)
    {
        pcd_set_tmo(5);

        pi.mf_command = PCD_TRANSCEIVE;
        pi.mf_length = 16;
        memcpy(&pi.mf_data[0], pwritedata, 16);

        status = pcd_com_transceive(&pi);
        if (status != HAL_STATUS_TIMEOUT)
        {
            pi.mf_data[0] &= 0x0F;
            switch (pi.mf_data[0])
            {
            case 0x00:
                status = HAL_STATUS_WRITE_ERR;
                break;
            case 0x0A:
                status = HAL_STATUS_OK;
                break;
            default:
                status = HAL_STATUS_CODE_ERR;
                break;
            }
        }
        pcd_set_tmo(4);
    }
    return status;
}

/*
 * @brief: 写数据块到卡上的一块（block）
 *
 * @param:
 *      addr:       要写的绝对块号
 *      pwritedata  存放写的数据缓存区的首地址
 *
 * @return: HAL_STATUS_OK成功，否则失败；
 */
int pcd_write_ultralight(uint8_t addr, uint8_t *pwritedata)
{
    int status;
    hal_nfc_transceive_t pi;

    if (NFC_DEBUG_LOG)
    {
        printf("WRITE_UL:\n");
    }

    nfc_write_reg(REG_BITFRAMING, 0x00);            // // Tx last bits = 0, rx align = 0
    nfc_set_reg_bit(REG_TXMODE, BIT(7));            // 使能发送crc
    hal_nfc_clear_register_bit(REG_RXMODE, BIT(7)); // 不使能接收crc
    pcd_set_tmo(5);

    pi.mf_command = PCD_TRANSCEIVE;
    pi.mf_length = 6; // a2h ADR D0 D1 D2 D3
    pi.mf_data[0] = PICC_WRITE_ULTRALIGHT;
    pi.mf_data[1] = addr;
    memcpy(&pi.mf_data[2], pwritedata, 4); // 4 字节数据

    status = pcd_com_transceive(&pi);

    if (status != HAL_STATUS_TIMEOUT)
    {
        pi.mf_data[0] &= 0x0F;
        switch (pi.mf_data[0])
        {
        case 0x00:
            status = HAL_STATUS_WRITE_ERR;
            break;
        case 0x0A:
            status = HAL_STATUS_OK;
            break;
        default:
            status = HAL_STATUS_CODE_ERR;
            break;
        }
    }
    pcd_set_tmo(4);

    return status;
}

int pcd_valueblock_operation(uint8_t mode, uint8_t addr, uint8_t *pwritedata)
{
    int status;

    hal_nfc_transceive_t pi;

    if (NFC_DEBUG_LOG)
    {
        printf("VALUE BLOCK OP:\n");
    }

    nfc_write_reg(REG_BITFRAMING, 0x00);
    nfc_set_reg_bit(REG_TXMODE, BIT(7));
    hal_nfc_clear_register_bit(REG_RXMODE, BIT(7));

    pcd_set_tmo(5);

    pi.mf_command = PCD_TRANSCEIVE;
    pi.mf_length = 2;
    pi.mf_data[0] = mode;
    pi.mf_data[1] = addr;

    status = pcd_com_transceive(&pi);
    if (status != HAL_STATUS_TIMEOUT)
    {
        if (pi.mf_length != 4)
        {
            status = HAL_STATUS_BITCOUNT_ERR;
        }
        else
        {
            pi.mf_data[0] &= 0x0F;
            switch (pi.mf_data[0])
            {
            case 0x00:
                status = HAL_STATUS_NOAUTH_ERR;
                break;
            case 0x0A:
                status = HAL_STATUS_OK;
                break;
            default:
                status = HAL_STATUS_CODE_ERR;
                break;
            }
        }
    }
    if (status == HAL_STATUS_OK)
    {
        pcd_set_tmo(5);

        pi.mf_command = PCD_TRANSCEIVE;
        pi.mf_length = 4;
        memcpy(&pi.mf_data[0], pwritedata, 4);

        status = pcd_com_transceive(&pi);
        if (status != HAL_STATUS_TIMEOUT)
        {
            pi.mf_data[0] &= 0x0F;
            switch (pi.mf_data[0])
            {
            case 0x00:
                status = HAL_STATUS_WRITE_ERR;
                break;
            case 0x0A:
                status = HAL_STATUS_OK;
                break;
            default:
                status = HAL_STATUS_CODE_ERR;
                break;
            }
        }
        if (status == HAL_STATUS_TIMEOUT)
            status = HAL_STATUS_OK;
    }
    return status;
}

int pcd_valueblock_transfer(uint8_t addr)
{
    int status;

    hal_nfc_transceive_t pi;

    if (NFC_DEBUG_LOG)
    {
        printf("VALUE BLOCK TRANS:\n");
    }

    nfc_write_reg(REG_BITFRAMING, 0x00);
    nfc_set_reg_bit(REG_TXMODE, BIT(7));
    hal_nfc_clear_register_bit(REG_RXMODE, BIT(7));

    pcd_set_tmo(5);

    pi.mf_command = PCD_TRANSCEIVE;
    pi.mf_length = 2;
    pi.mf_data[0] = PICC_TRANSFER;
    pi.mf_data[1] = addr;

    status = pcd_com_transceive(&pi);

    if (status != HAL_STATUS_TIMEOUT)
    {
        pi.mf_data[0] &= 0x0F;
        switch (pi.mf_data[0])
        {
        case 0x00:
            status = HAL_STATUS_WRITE_ERR;
            break;
        case 0x0A:
            status = HAL_STATUS_OK;
            break;
        default:
            status = HAL_STATUS_CODE_ERR;
            break;
        }
    }

    return status;
}
