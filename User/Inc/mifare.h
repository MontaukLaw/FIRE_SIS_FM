/**
 ****************************************************************
 * @file mifare.h
 *
 * @brief
 *
 * @author
 *
 *
 ****************************************************************
 */
#ifndef MIFARE_H
#define MIFARE_H

#include <stdint.h>
#include "rfid.h"
/**
 * Mifare Error Codes
 * Each function returns a status value, which corresponds to
 * the mifare error
 * codes.
 ****************************************************************
 */
#define MI_OK 0
#define MI_CHK_OK 0
#define MI_CRC_ZERO 0

#define MI_CRC_NOTZERO 1

#define MI_NOTAGERR (-1)
#define MI_CHK_FAILED (-1)
#define MI_CRCERR (-2)
#define MI_CHK_COMPERR (-2)
#define MI_EMPTY (-3)
#define MI_AUTHERR (-4)
#define MI_PARITYERR (-5)
#define MI_CODEERR (-6)
#define MI_SERNRERR (-8)
#define MI_KEYERR (-9)
#define MI_NOTAUTHERR (-10)
#define MI_BITCOUNTERR (-11)
#define MI_BYTECOUNTERR (-12)
#define MI_IDLE (-13)
#define MI_TRANSERR (-14)
#define MI_WRITEERR (-15)
#define MI_INCRERR (-16)
#define MI_DECRERR (-17)
#define MI_READERR (-18)
#define MI_OVFLERR (-19)
#define MI_POLLING (-20)
#define MI_FRAMINGERR (-21)
#define MI_ACCESSERR (-22)
#define MI_UNKNOWN_COMMAND (-23)
#define MI_COLLERR (-24)
#define MI_RESETERR (-25)
#define MI_INITERR (-25)
#define MI_INTERFACEERR (-26)
#define MI_ACCESSTIMEOUT (-27)
#define MI_NOBITWISEANTICOLL (-28)
#define MI_QUIT (-30)
#define MI_INTEGRITY_ERR (-35) // 完整性错误(crc/parity/protocol)
#define MI_RECBUF_OVERFLOW (-50)
#define MI_SENDBYTENR (-51)
#define MI_SENDBUF_OVERFLOW (-53)
#define MI_BAUDRATE_NOT_SUPPORTED (-54)
#define MI_SAME_BAUDRATE_REQUIRED (-55)
#define MI_WRONG_PARAMETER_VALUE (-60)
#define MI_BREAK (-99)
#define MI_NY_IMPLEMENTED (-100)
#define MI_NO_MFRC (-101)
#define MI_MFRC_NOTAUTH (-102)
#define MI_WRONG_DES_MODE (-103)
#define MI_HOST_AUTH_FAILED (-104)
#define MI_WRONG_LOAD_MODE (-106)
#define MI_WRONG_DESKEY (-107)
#define MI_MKLOAD_FAILED (-108)
#define MI_FIFOERR (-109)
#define MI_WRONG_ADDR (-110)
#define MI_DESKEYLOAD_FAILED (-111)
#define MI_WRONG_SEL_CNT (-114)
#define MI_WRONG_TEST_MODE (-117)
#define MI_TEST_FAILED (-118)
#define MI_TOC_ERROR (-119)
#define MI_COMM_ABORT (-120)
#define MI_INVALID_BASE (-121)
#define MI_MFRC_RESET (-122)
#define MI_WRONG_VALUE (-123)
#define MI_VALERR (-124)
#define MI_COM_ERR (-125)

/// 用户使用错误
#define USER_ERROR (-127)

#define PICC_AUTHENT1A 0x60 // 验证A密钥
#define PICC_AUTHENT1B 0x61 // 验证B密钥
#define PICC_READ 0x30      // 读块
#define PICC_WRITE 0xA0     // 写块
#define PICC_DECREMENT 0xC0 // 扣款
#define PICC_INCREMENT 0xC1 // 充值
#define PICC_RESTORE 0xC2   // 调块数据到缓冲区
#define PICC_TRANSFER 0xB0  // 保存缓冲区中数据
#define PICC_RESET 0xE0     // 复位

// UltraLight
#define PICC_WRITE_ULTRALIGHT 0xA2 // 超轻卡写块

int pcd_auth_state(uint8_t auth_mode, uint8_t block, uint8_t *psnr, uint8_t *pkey);
int pcd_read(uint8_t addr, uint8_t *preaddata);
int pcd_write(uint8_t addr, uint8_t *pwritedata);
int pcd_write_ultralight(uint8_t addr, uint8_t *pwritedata);
int pcd_valueblock_operation(uint8_t mode, uint8_t addr, uint8_t *pwritedata);
int pcd_valueblock_transfer(uint8_t addr);
#endif
