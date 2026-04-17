/**
 ****************************************************************
 * @file iso14443a.h
 *
 * @brief
 *
 * @author
 *
 *
 ****************************************************************
 */
#ifndef ISO14443A_H
#define ISO14443A_H

#include "nfc_transfer.h"

/**
 * DEFINES ISO14443A COMMAND
 * commands which are handled by the tag,Each tag command is written to the
 * reader IC and transfered via RF
 ****************************************************************
 */

#define PICC_REQIDL 0x26    // 寻天线区内未进入休眠状态的卡片
#define PICC_REQALL 0x52    // 寻天线区内全部卡
#define PICC_ANTICOLL1 0x93 // 防冲撞
#define PICC_ANTICOLL2 0x95 // 防冲撞
#define PICC_ANTICOLL3 0x97 // 防冲撞
#define PICC_HLTA 0x50      // 休眠

#define RxAlign (BIT(4) | BIT(5) | BIT(6))
#define TxLastBits (BIT(0) | BIT(1) | BIT(2))

/*
 * FUNCTION DECLARATIONS
 ****************************************************************
 */

int pcd_request(uint8_t req_code, uint8_t *ptagtype);
int pcd_cascaded_anticoll(uint8_t select_code, uint8_t coll_position, uint8_t *psnr);
int pcd_cascaded_select(uint8_t select_code, uint8_t *psnr, uint8_t *psak);
int pcd_hlta(void);
int pcd_rats_a(uint8_t CID, uint8_t *ats);

int pcd_pps_rate(hal_nfc_transceive_t *pi, uint8_t *ATS, uint8_t CID, uint8_t rate);
int apdu(uint8_t *data, uint8_t tx_len, uint8_t *rx_data, uint8_t *rx_len);
int read_fifo(uint8_t *rx_data, uint8_t *rx_len);
int pcd_receive(void);
int pcd_transmit(uint8_t *data, uint8_t tx_len);
int apdu_exchange(uint8_t *data, uint8_t tx_len, uint8_t *rx_data, uint8_t *rx_len);

#endif
