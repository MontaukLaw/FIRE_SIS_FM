#ifndef _NFC_CONFIG_H_
#define _NFC_CONFIG_H_

void nfc_config(hal_nfc_regval_t *config, uint8_t size);

void pcd_set_rate(uint8_t rate, uint8_t type);

#endif // _NFC_CONFIG_H_
