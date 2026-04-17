/**
 ****************************************************************
 * @file iso14443b.h
 *
 * @brief
 *
 * @author
 *
 *
 ****************************************************************
 */
#ifndef ISO_14443B_H
#define ISO_14443B_H

#include <stdint.h>

/////////////////////////////////////////////////////////////////////
// ISO14443B COMMAND
/////////////////////////////////////////////////////////////////////
#define ISO14443_3B_REQIDL 0x00 // Idle card
#define ISO14443_3B_REQALL 0x08 // All card
#define ISO14443B_ANTICOLLISION 0x05
#define ISO14443B_ATTRIB 0x1D
#define ISO14443B_HLTB 0x50

char select_sr(uint8_t *chip_id);
char read_sr176(uint8_t addr, uint8_t *readdata);
char write_sr176(uint8_t addr, uint8_t *writedata);
char protect_sr176(uint8_t lockreg);
char completion_sr(void);

int pcd_request_b(uint8_t req_code, uint8_t AFI, uint8_t N, uint8_t *ATQB);
int pcd_slot_marker(uint8_t N, uint8_t *ATQB);
int pcd_pps_rate_b(uint8_t CID, uint8_t *ATQB, uint8_t rate);
int pcd_attri_b(uint8_t *PUPI, uint8_t dsi_dri, uint8_t pro_type, uint8_t CID, uint8_t *answer);
int get_idcard_num(uint8_t *ATQB);
int pcd_halt_b(uint8_t *PUPI);

#endif
