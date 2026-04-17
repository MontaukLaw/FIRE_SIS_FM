#include "main.h"

void make_packet(uint8_t cmd, uint8_t *buf, uint8_t len)
{
    uint8_t sum = 0;

    sum += 0x68;
    drv_interface.hw_put_char(0x68);
    sum += len + 4;
    drv_interface.hw_put_char(len + 4);
    sum += cmd;
    drv_interface.hw_put_char(cmd);

    while (len--)
    {
        drv_interface.hw_put_char(*buf);
        sum += *buf++;
    }
    drv_interface.hw_put_char(sum);
}
