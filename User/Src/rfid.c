#include "main.h"

tag_info g_tag_info;

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
    
    // 一次防冲突及选卡
    if ((status == MI_OK) || (status == MI_COLLERR))
    {
        g_tag_info.uid_length = UID_4;
        // make_packet(COM_PKT_CMD_CARD_TYPE, g_tag_info.tag_type_bytes, sizeof(g_tag_info.tag_type_bytes));
        // g_tag_info.tag_type = check_tag_type(g_tag_info.tag_type_bytes);
        status = pcd_cascaded_anticoll(PICC_ANTICOLL1, 0, &g_tag_info.serial_num[0]);
        if (status == MI_OK)
        {
            status = pcd_cascaded_select(PICC_ANTICOLL1, &g_tag_info.serial_num[0], &sak);
        }
    }
    // 二次防冲突及选卡
    if (status == MI_OK && (sak & BIT2))
    {
        g_tag_info.uid_length = UID_7;
        status = pcd_cascaded_anticoll(PICC_ANTICOLL2, 0, &g_tag_info.serial_num[4]);
        if (status == MI_OK)
        {
            status = pcd_cascaded_select(PICC_ANTICOLL2, &g_tag_info.serial_num[4], &sak);
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
        printf("reqa_ok\n");
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

u8 reader_mode(void)
{
    uint8_t status_a = 1;
    tick g_statistic_last_t = 0;
    uint8_t cmd[2] = {0}; /*********raqa****************/

    //	uint8_t rx_buff[64];
    //	uint8_t rx_length = 0;
    uint8_t times = 10;
    // uint8_t ats[2]={2,0x20};
    printf("reader_mode : \n");
    nfc_antenna_on();
    HAL_Delay(10);

    cmd[1] = 0x52;
    status_a = com_reqa(cmd); // 询所有A

    return status_a;
}
