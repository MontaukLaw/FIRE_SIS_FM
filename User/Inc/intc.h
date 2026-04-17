#ifndef _INTC_H_
#define _INTC_H_

extern volatile uint8_t read_fifo_flag;

extern volatile uint8_t reader_mode_flag;

extern process_handle event_cb[];

void intc_irq_register(intc_event_e e, process_handle cb);

void intc_irq_unregister(intc_event_e e);


#endif // _INTC_H_
