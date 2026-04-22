#include "main.h"

tag_info g_tag_info;

static void print_card_info(uint8_t sak)
{
    USART1_Send_String("A ");
    USART1_Send_Hex8(g_tag_info.tag_type_bytes[0]);
    USART1_Send_Hex8(g_tag_info.tag_type_bytes[1]);
    USART1_Send_Byte(' ');
    USART1_Send_Hex8(sak);
    USART1_Send_Byte(' ');

    for (uint8_t i = 0; i < g_tag_info.uid_length; i++)
    {
        USART1_Send_Hex8(g_tag_info.serial_num[i]);
        if (i + 1 < g_tag_info.uid_length)
        {
            USART1_Send_Byte(':');
        }
    }
    USART1_Send_String("\r\n");
}

static void print_reader_step(char tag, int status, const uint8_t *data, uint8_t len)
{
    USART1_Send_Byte(tag);
    USART1_Send_Byte(' ');
    USART1_Send_Hex8((uint8_t)status);

    for (uint8_t i = 0; i < len; i++)
    {
        USART1_Send_Byte(' ');
        USART1_Send_Hex8(data[i]);
    }

    USART1_Send_String("\r\n");
}

int com_reqa(u8 *pcmd)
{
    int status;
    u8 sak;
    //	u8  buf[10];
    // typeA寻卡
    //	g_statistics.reqa_cnt++;
    //    g_statistics.m1_cnt++;
    //	g_statistic_refreshed=TRUE;

    pcd_default_info();

    status = pcd_request(pcmd[1], g_tag_info.tag_type_bytes);
    print_reader_step('Q', status, g_tag_info.tag_type_bytes, 2);
    // 一次防冲突及选卡
    if ((status == MI_OK) || (status == MI_COLLERR))
    {
        g_tag_info.uid_length = UID_4;
        // make_packet(COM_PKT_CMD_CARD_TYPE, g_tag_info.tag_type_bytes, sizeof(g_tag_info.tag_type_bytes));
        // g_tag_info.tag_type = check_tag_type(g_tag_info.tag_type_bytes);
        status = pcd_cascaded_anticoll(PICC_ANTICOLL1, 0, &g_tag_info.serial_num[0]);
        print_reader_step('N', status, g_tag_info.serial_num, 4);

        if (status == MI_OK)
        {
            status = pcd_cascaded_select(PICC_ANTICOLL1, &g_tag_info.serial_num[0], &sak);
            print_reader_step('S', status, &sak, 1);
        }
    }
    // 二次防冲突及选卡
    if (status == MI_OK && (sak & BIT2))
    {
        g_tag_info.uid_length = UID_7;
        status = pcd_cascaded_anticoll(PICC_ANTICOLL2, 0, &g_tag_info.serial_num[4]);
        print_reader_step('n', status, &g_tag_info.serial_num[4], 3);
        if (status == MI_OK)
        {
            status = pcd_cascaded_select(PICC_ANTICOLL2, &g_tag_info.serial_num[4], &sak);
            print_reader_step('s', status, &sak, 1);
        }
    }
    // 回复UID
    if (status == MI_OK)
    {
        // buf[0] = g_tag_info.uid_length;
        // memcpy(buf+1, (g_tag_info.uid_length == UID_4 ? &g_tag_info.serial_num[0]:&g_tag_info.serial_num[1]), g_tag_info.uid_length);
        // mdelay(50);
        // make_packet(COM_PKT_CMD_REQA, buf, g_tag_info.uid_length + 1);
        // mdelay(50);
    }

    if (status == MI_OK)
    {
        print_card_info(sak);
    }
    else
    {
        // g_statistics.reqa_fail++;
        // g_statistics.m1_fail++;
        // rintf("reqa_fail\n");
    }

    return status;
}

/********
** PPH     2 byte
** CMD     1 byte
** DATALEN 1 byte
** DATA
****/
// uint8_t tx_buff[10]={0xaa,0xaa,1,1,1,1,1,1,0xcc,0xc};
int reader_mode(void)
{
    int status_a = 1;
    uint8_t cmd[2] = {0};

    nfc_antenna_on();
    HAL_Delay(10);

    cmd[1] = 0x26;
    status_a = com_reqa(cmd);

    return status_a;
}
