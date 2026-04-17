#ifndef _NFC_TRANSFER_H_
#define _NFC_TRANSFER_H_

void pcd_default_info(void); // COS_TEST

int pcd_com_transceive(hal_nfc_transceive_t *pi);

extern pcd_info_s g_pcd_module_info;

#endif // _NFC_TRANSFER_H_

