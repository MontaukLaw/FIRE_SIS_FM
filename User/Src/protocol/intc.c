#include "main.h"

volatile uint8_t read_fifo_flag = 0;
volatile uint8_t reader_mode_flag = 0;

process_handle event_cb[EVENT_NUM];

void intc_irq_register(intc_event_e e, process_handle cb)
{
    event_cb[e] = cb;
}

void intc_irq_unregister(intc_event_e e)
{
    event_cb[e] = NULL;
}
