#ifndef _PICC_H_
#define _PICC_H_

void picc_14443_4_enable(uint8_t en);

void picc_set_atqa_hbyte(uint8_t atqa_h);

void picc_set_atqa_lbyte(uint8_t atqa_l);

void picc_init(void);

void picc_blk_num_set(uint8_t num);

void picc_reset(void);

void picc_uid_set(uint8_t *uid, uint8_t len);

void picc_set_halt(void);

void picc_field_in_irq(uint8_t en);

void picc_field_out_irq(uint8_t en);

void picc_active_irq(uint8_t en);

void picc_rx_irq(uint8_t en);

void picc_write(uint8_t *buf, uint16_t len);


#endif /* _PICC_H_ */
