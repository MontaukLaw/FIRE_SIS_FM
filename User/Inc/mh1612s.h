#ifndef _MH1612S_H_
#define _MH1612S_H_

void mh1612s_init(void);

void nfc_set_bit(uint8_t page, uint8_t addr, uint8_t value, uint8_t page_sel);

void nfc_clear_bit(uint8_t page, uint8_t addr, uint8_t value, uint8_t page_sel);

uint8_t nfc_check_bit(uint8_t page, uint8_t addr, uint8_t value, uint8_t page_sel);

void nfc_set_reg(uint8_t page, uint8_t addr, uint8_t value, uint8_t page_sel);

// HAL_StatusTypeDef nfc_write_reg(uint8_t addr, uint8_t value);

HAL_StatusTypeDef nfc_read_reg(uint8_t addr, uint8_t *value);

void nfc_clear_reg_bit(uint8_t addr, uint8_t mask);

void nfc_set_reg_bit(uint8_t addr, uint8_t mask);

uint8_t nfc_get_reg(uint8_t page, uint8_t addr, uint8_t page_sel);

HAL_StatusTypeDef nfc_write_reg(uint8_t addr, const uint8_t value);

void hal_nfc_pcd_reset(void);


#endif // _MH1612S_H_
