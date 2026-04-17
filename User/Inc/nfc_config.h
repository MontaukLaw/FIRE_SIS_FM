#ifndef _NFC_CONFIG_H_
#define _NFC_CONFIG_H_

void nfc_config(hal_nfc_regval_t *config, uint8_t size);

void pcd_set_rate(uint8_t rate, uint8_t type);

void nfc_antenna_on(void);

void nfc_antenna_off(void);

void pcd_set_tmo_FWT_n9(void);

void pcd_set_tmo(uint8_t fwi);

void pcd_set_tmo_FWT_ATQB(void);

void pcd_delay_sfgi(uint8_t sfgi);

void pcd_set_tmo_FWT_ACTVITION(void);

extern uint8_t EMV_TEST;

#endif // _NFC_CONFIG_H_
