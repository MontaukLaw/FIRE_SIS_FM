#include "main.h"

// 14443-4
#define PICC_RATS 0xE0 // Request for Answer To Select
#define PICC_NAK 0xB2  // no CID

extern uint8_t EMV_TEST;
extern uint8_t NFC_DEBUG_LOG;

unsigned short gausMaxFrameSizeTable[] = {
	16,
	24,
	32,
	40,
	48,
	64,
	96,
	128,
	256,
	256,
	256,
	256,
	256,
	256,
	256,
	256,
	256,
	256,
};

/**
 ****************************************************************
 * @brief pcd_request()
 *
 * 功能：寻卡
 *
 * @param: req_code[IN]:寻卡方式
 *                0x52 = 寻感应区内所有符合14443A标准的卡
 *                0x26 = 寻未进入休眠状态的卡
 * @param: ptagtype[OUT]卡片返回的ATQA, 根据该值判断卡片类型仅供参考
 *                0x4400 = Mifare_UltraLight
 *                0x4400 = Mifare_One(S50_0)
 *                0x0400 = Mifare_One(S50_3)
 *                0x0200 = Mifare_One(S70_0)
				  0x4200 = Mifare_One(S70_3)
 *                0x0800 = Mifare_Pro
 *                0x0403 = Mifare_ProX
 *                0x4403 = Mifare_DESFire
 *
 * @return: 返回成功HAL_STATUS_OK
 * @retval:
 ****************************************************************
 */
int pcd_request(uint8_t req_code, uint8_t *ptagtype)
{
	int status;

	hal_nfc_transceive_t pi;

	if (NFC_DEBUG_LOG)
		printf("REQA/WUPA:\n");
	nfc_write_reg(REG_BITFRAMING, 0x07); // Tx last bytes = 7

	nfc_clear_reg_bit(REG_TXMODE, BIT(7));	// 不使能crc
	nfc_clear_reg_bit(REG_RXMODE, BIT(7));	// 不使能crc
	nfc_clear_reg_bit(REG_STATUS2, BIT(3)); // 清零MF crypto1认证成功标记
	if (EMV_TEST)
	{
		// rxwait reconfig
		nfc_set_reg(1, 0x17, 0x92, 0);
		nfc_set_reg(4, 0x3f, 0x52, 1);
		// disable emd
		nfc_clear_bit(1, 0x18, 1, 0);
		nfc_set_reg(10, 0x3f, 0x80, 1);
		pcd_set_tmo_FWT_n9();
	}
	else
	{
		pcd_set_tmo(4);
	}
	pi.type = HAL_TECH_TYPE_A;
	pi.mf_command = PCD_TRANSCEIVE;
	pi.mf_length = 1;
	pi.mf_data[0] = req_code;

	status = pcd_com_transceive(&pi);

	if ((HAL_STATUS_OK == status) && pi.mf_length != 0x10)
	{
		printf("bit count err\r\n");
		status = HAL_STATUS_BITCOUNT_ERR;
	}

	if (EMV_TEST)
	{
		if (status == HAL_STATUS_OK &&
			(((pi.mf_data[1] & 0xF0) != 0) // RFU=0	"01 F0" //TA304.15
			 ))
		{
			printf("ATQA:protocol err\n");
			status = HAL_STATUS_PROTOCOL_ERR;
		}
	}
	*ptagtype = pi.mf_data[0];
	*(ptagtype + 1) = pi.mf_data[1];

	return status;
}

/**
 ****************************************************************
 * @brief pcd_cascaded_anticoll()
 *
 * 防冲撞函数
 * @param: select_code    0x93  cascaded level 1
 *                        0x95  cascaded level 2
 *                        0x97  cascaded level 3
 * @param: psnr 存放序列号(4byte)内存单元的首地址
 * @return: status HAL_STATUS_OK
 * @retval: psnr  得到的序列号放入指定内存单元
 ****************************************************************
 */
int pcd_cascaded_anticoll(uint8_t select_code, uint8_t coll_position, uint8_t *psnr)
{
	int status;
	uint8_t i;
	uint8_t temp;
	uint8_t bits;
	uint8_t bytes;
	uint8_t snr_check;
	uint8_t snr[5];
	hal_nfc_transceive_t pi;

	snr_check = 0;
	coll_position = 0;
	memset(snr, 0, sizeof(snr));
	if (NFC_DEBUG_LOG)
		printf("ANT:\n");
	nfc_write_reg(REG_BITFRAMING, 0x00);   // // Tx last bits = 0, rx align = 0
	nfc_clear_reg_bit(REG_TXMODE, BIT(7)); // 不使能crc
	nfc_clear_reg_bit(REG_RXMODE, BIT(7)); // 不使能crc
	nfc_set_reg(1, 0x17, 0x91, 0);
	nfc_set_reg(4, 0x3f, 0x52, 1);
	do
	{
		bits = coll_position % 8;
		if (bits != 0)
		{
			bytes = coll_position / 8 + 1;
			nfc_clear_reg_bit(REG_BITFRAMING, TxLastBits | RxAlign);
			nfc_set_reg_bit(REG_BITFRAMING, (TxLastBits & (bits)) | (RxAlign & (bits << 4))); // tx lastbits , rx align
		}
		else
		{
			bytes = coll_position / 8;
		}
		pi.type = HAL_TECH_TYPE_A;
		pi.mf_command = PCD_TRANSCEIVE;
		pi.mf_data[0] = select_code;
		pi.mf_data[1] = 0x20 + ((coll_position / 8) << 4) + (bits & 0x0F);
		for (i = 0; i < bytes; i++)
		{
			pi.mf_data[i + 2] = snr[i];
		}
		pi.mf_length = bytes + 2;
		if (EMV_TEST)
		{
			pcd_set_tmo_FWT_n9();
			for (int i = 0; i < 10000; i++)
				;
		}
		status = pcd_com_transceive(&pi);
		temp = snr[coll_position / 8];
		if (status == HAL_STATUS_COLLERR)
		{
			if (EMV_TEST)
			{
				return status;
			}

			else
			{
				for (i = 0; (5 >= coll_position / 8) && (i < 5 - (coll_position / 8)); i++)
				{
					snr[i + (coll_position / 8)] = pi.mf_data[i + 1];
				}
				snr[(coll_position / 8)] |= temp;
				if (pi.mf_data[0] >= bits)
				{
					coll_position += pi.mf_data[0] - bits; // UID0
				}
				else
				{
					if (NFC_DEBUG_LOG)
						printf("Err:coll_p  mf_data[0]=%02x < bits=%02x\n", pi.mf_data[0], bits);
				}

				/* 保留冲突位以前的有效位 */
				snr[(coll_position / 8)] &= (0xff >> (8 - (coll_position % 8)));
				/* 选择冲突bit位为1或是0的卡*/
				snr[(coll_position / 8)] |= 1 << (coll_position % 8); // 选择bit = 1的卡
				// snr[(coll_position / 8)] &=  ~(1 << (coll_position % 8));//选择bit = 0的卡
				coll_position++; // 冲突bit位增1
			}
		}
		else if (status == HAL_STATUS_OK)
		{
			// EMV_TEST
			if (pi.mf_length / 8 != 5)
			{
				return HAL_STATUS_PROTOCOL_ERR;
			}
			for (i = 0; i < (pi.mf_length / 8) && (i <= 4); i++) // 增加(i <= 4)防止snr[4-i]溢出
			{
				snr[4 - i] = pi.mf_data[pi.mf_length / 8 - i - 1];
			}
			snr[(coll_position / 8)] |= temp;
		}

	} while (status == HAL_STATUS_COLLERR);

	if (status == HAL_STATUS_OK)
	{
		for (i = 0; i < 4; i++)
		{
			*(psnr + i) = snr[i];
			snr_check ^= snr[i];
		}
		if (snr_check != snr[i])
		{
			status = HAL_STATUS_ERR;
		}
	}

	nfc_write_reg(REG_BITFRAMING, 0x00); // // Tx last bits = 0, rx align = 0

	return status;
}

/**
 ****************************************************************
 * @brief pcd_select()
 *
 * 选定一张卡
 * @param: select_code    0x93  cascaded level 1
 *                        0x95  cascaded level 2
 *                        0x97  cascaded level 3
 * @param: psnr 存放序列号(4byte)的内存单元首地址·
 * @return: status 值为HAL_STATUS_OK:成功
 * @retval: psnr  得到序列号放入指定单元
 * @retval: psak  得到的select acknolege 回复
 *
 *			  sak:
 *            Corresponding to the specification in ISO 14443, this function
 *            is able to handle extended serial numbers. Therefore more than
 *            one select_code is possible.
 *
 *            Select codes:
 *
 *            +----+----+----+----+----+----+----+----+
 *            | b8 | b7 | b6 | b5 | b4 | b3 | b2 | b1 |
 *            +-|--+-|--+-|--+-|--+----+----+----+-|--+
 *              |    |    |    |  |              | |
 *                                |              |
 *              1    0    0    1  | 001..std     | 1..bit frame anticoll
 *                                | 010..double  |
 *                                | 011..triple  |
 *
 *            SAK:
 *
 *            +----+----+----+----+----+----+----+----+
 *            | b8 | b7 | b6 | b5 | b4 | b3 | b2 | b1 |
 *            +-|--+-|--+-|--+-|--+-|--+-|--+-|--+-|--+
 *              |    |    |    |    |    |    |    |
 *                        |              |
 *                RFU     |      RFU     |      RFU
 *
 *                        1              0 .. UID complete, ATS available
 *                        0              0 .. UID complete, ATS not available
 *                        X              1 .. UID not complete
 *
 ****************************************************************
 */
int pcd_cascaded_select(uint8_t select_code, uint8_t *psnr, uint8_t *psak)
{
	uint8_t i;
	int status;
	uint8_t snr_check;

	hal_nfc_transceive_t pi;
	snr_check = 0;
	if (NFC_DEBUG_LOG)
		printf("SELECT:\n");
	nfc_write_reg(REG_BITFRAMING, 0x00); // // Tx last bits = 0, rx align = 0
	nfc_set_reg_bit(REG_TXMODE, BIT(7)); // 使能发送crc
	nfc_set_reg_bit(REG_RXMODE, BIT(7)); // 使能接收crc
	nfc_set_reg(1, 0x17, 0x92, 0);
	nfc_set_reg(4, 0x3f, 0x52, 1);
	if (EMV_TEST)
	{
		pcd_set_tmo_FWT_n9();
	}
	else
	{
		pcd_set_tmo(4);
	}
	pi.type = HAL_TECH_TYPE_A;
	pi.mf_command = PCD_TRANSCEIVE;
	pi.mf_length = 7;
	pi.mf_data[0] = select_code;
	pi.mf_data[1] = 0x70;
	for (i = 0; i < 4; i++)
	{
		snr_check ^= *(psnr + i);
		pi.mf_data[i + 2] = *(psnr + i);
	}
	pi.mf_data[6] = snr_check;

	status = pcd_com_transceive(&pi);

	if (status == HAL_STATUS_OK)
	{
		if (pi.mf_length != 0x8)
		{
			status = HAL_STATUS_BITCOUNT_ERR;
		}
		else
		{
			*psak = pi.mf_data[0];
		}
	}

	return status;
}

/**
 ****************************************************************
 * @brief pcd_hlta()
 *
 * 功能：命令卡进入休眠状态
 *
 * @param:
 * @return: status 值为HAL_STATUS_OK成功
 *
 ****************************************************************
 */
int pcd_hlta(void)
{
	int status = HAL_STATUS_OK;

	hal_nfc_transceive_t pi;

	if (NFC_DEBUG_LOG)
		printf("HALT:\n");
	nfc_write_reg(REG_BITFRAMING, 0x00);   // // Tx last bits = 0, rx align = 0
	nfc_set_reg_bit(REG_TXMODE, BIT(7));   // 使能发送crc
	nfc_clear_reg_bit(REG_RXMODE, BIT(7)); // 不使能接收crc

	pcd_delay_sfgi(g_pcd_module_info.ui_sfgi);
	if (EMV_TEST)
	{
		pcd_set_tmo_FWT_n9();
	}
	else
	{
		pcd_set_tmo(2); // according to 14443-3 1ms
	}

	pi.mf_command = PCD_TRANSCEIVE;
	pi.mf_length = 2;
	pi.mf_data[0] = PICC_HLTA;
	pi.mf_data[1] = 0;

	status = pcd_com_transceive(&pi);
	if (status)
	{
		if (status == HAL_STATUS_TIMEOUT || status == HAL_STATUS_ACCESSTIMEOUT)
		{
			status = HAL_STATUS_OK;
		}
	}
	return status;
}

int pcd_rats_a(uint8_t CID, uint8_t *ats)
{
	int status = HAL_STATUS_OK;
	hal_nfc_transceive_t pi;
	uint8_t *ATS;
	uint8_t ta, tb, tc;

	ta = tb = tc = 0;

	/*initialiszed the PCB*/
	g_pcd_module_info.uc_pcd_pcb = 0x02;
	g_pcd_module_info.uc_picc_pcb = 0x03;
	g_pcd_module_info.uc_cid = CID;
	pcd_delay_sfgi(g_pcd_module_info.ui_sfgi);
	if (EMV_TEST)
	{
		// emd enable
		hal_nfc_bit_set(1, 0x18, 1, 0);
		nfc_set_reg(10, 0x3f, 0x00, 1);
		// rxwait reconfig
		nfc_set_reg(1, 0x17, 0x91, 0);
		nfc_set_reg(4, 0x3f, 0x52, 1);
		pcd_set_tmo_FWT_ACTVITION();
	}
	else
	{
		pcd_set_tmo(4); // according to 14443-4 4.8ms
	}

	nfc_write_reg(REG_BITFRAMING, 0x00); // // Tx last bits = 0, rx align = 0
	nfc_set_reg_bit(REG_TXMODE, BIT(7)); // 使能发送crc
	nfc_set_reg_bit(REG_RXMODE, BIT(7)); // 使能接收crc

	pi.mf_command = PCD_TRANSCEIVE;
	pi.mf_length = 2;
	pi.mf_data[0] = PICC_RATS;		   // Start byte
	pi.mf_data[1] = (FSDI << 4) + CID; // Parameter

	status = pcd_com_transceive(&pi);

	if (EMV_TEST)
	{
		// TA340.0 TA340.1 如果deaf time功能正常，则肯定收到的数据不是8的整数倍。就会造成timeout。

		// EMV_TEST TA307.1 TA307.3  TA307.4  TA340.1 -2014-12-03
		if ((status == HAL_STATUS_OK && (pi.mf_length % 8) != 0) || (status == HAL_STATUS_COLLERR) || (status == HAL_STATUS_INTEGRITY_ERR && pi.mf_length / 8 < 4)

		)
		{
			status = HAL_STATUS_TIMEOUT;
		}
	}
	else
	{
		if ((status == HAL_STATUS_OK && (pi.mf_length % 8) != 0) || (status == HAL_STATUS_COLLERR) || (status == HAL_STATUS_INTEGRITY_ERR && pi.mf_length / 8 < 4) || (status == HAL_STATUS_INTEGRITY_ERR && pi.mf_length / 8 >= 4 && (pi.mf_length % 8) != 0))
		{
			status = HAL_STATUS_TIMEOUT;
		}
	}
	if (HAL_STATUS_OK == status)
	{
		// ATS
		ATS = pi.mf_data;
		memcpy(ats, ATS, ATS[0]);

		if (pi.mf_length / 8 < 1 || ATS[0] != pi.mf_length / 8)
		{ // at least 1bytes, and	TL = length

			return HAL_STATUS_PROTOCOL_ERR;
		}

		if ((ATS[0] < (2 + ((ATS[1] & BIT(4)) >> 4) + ((ATS[1] & BIT(5)) >> 5) + ((ATS[1] & BIT(6)) >> 6))))
		{ // ERR:TL length
		  // return PROTOCOL_ERR;
		}
		else
		{
			if (!(ATS[1] & BIT(7)))
			{ // T0.7 = 0
				if (ATS[1] & BIT(4))
				{
					ta = 1;
				}
				if (ATS[1] & BIT(5))
				{
					tb = 1;
					g_pcd_module_info.ui_fwi = (ATS[2 + (uint8_t)ta] & 0xF0) >> 4;
					g_pcd_module_info.ui_sfgi = ATS[2 + (uint8_t)ta] & 0x0f;
				}
				if (ATS[1] & BIT(6))
				{
					tc = 1;
					g_pcd_module_info.uc_cid_en = 0; //?
					g_pcd_module_info.uc_nad_en = 0; //?
				}
				g_pcd_module_info.ui_fsc = gausMaxFrameSizeTable[ATS[1] & 0x0F];

				// FSC
				if (pi.mf_length / 8 < (2 + (uint8_t)ta + (uint8_t)tb + (uint8_t)tc))
				{ // 长度错误
					return PROTOCOL_ERR;
				}

				pcd_set_tmo(g_pcd_module_info.ui_fwi);
			}
			else
			{
				// status = PROTOCOL_ERR;
			}
		}
	}

	return status;
}

/**
 ****************************************************************
 * @brief pcd_pps_rate()
 *
 * 功能：设置卡片速率
 *
 *
 ****************************************************************
 */
int pcd_pps_rate(hal_nfc_transceive_t *pi, uint8_t *ATS, uint8_t CID, uint8_t rate)
{
	uint8_t DRI, DSI;
	int status = HAL_STATUS_OK;

	if (NFC_DEBUG_LOG)
		printf("PPS:\n");

	DRI = 0;
	DSI = 0;

	if ((ATS[0] > 1) && (ATS[1] & BIT(5)))
	{ // TA(1) transmited
		if (rate == 1)
		{
		}
		else if (rate == 2)
		{
			if (NFC_DEBUG_LOG)
				printf("212K\n");
			if ((ATS[2] & BIT(0)) && (ATS[2] & BIT(4)))
			{ // DS=2,DR=2 supported 212kbps
				DRI = 1;
				DSI = 1;
			}
			else
			{
				if (NFC_DEBUG_LOG)
					printf(",Unsupport\n");
				return HAL_STATUS_USER_ERR;
			}
		}
		else if (rate == 3)
		{
			if (NFC_DEBUG_LOG)
				printf("424K\n");
			if ((ATS[2] & BIT(1)) && (ATS[2] & BIT(5)))
			{ // DS=4,DR=4 supported 424kbps
				DRI = 2;
				DSI = 2;
			}
			else
			{
				if (NFC_DEBUG_LOG)
					printf(",Unsupport\n");
				return HAL_STATUS_USER_ERR;
			}
		}
		else if (rate == 4)
		{
			if (NFC_DEBUG_LOG)
				printf("848K\n");
			if ((ATS[2] & BIT(2)) && (ATS[2] & BIT(6)))
			{ // DS=4,DR=4 supported 424kbps
				DRI = 3;
				DSI = 3;
			}
			else
			{
				if (NFC_DEBUG_LOG)
					printf(",Unsupport\n");
				return HAL_STATUS_USER_ERR;
			}
		}
		else
		{
			if (NFC_DEBUG_LOG)
				printf("USER:No Rate select\n");
			return HAL_STATUS_USER_ERR;
		}
		nfc_write_reg(REG_BITFRAMING, 0x00); // // Tx last bits = 0, rx align = 0
		nfc_set_reg_bit(REG_TXMODE, BIT(7)); // 使能发送crc
		nfc_set_reg_bit(REG_RXMODE, BIT(7)); // 使能接收crc

		pi->mf_command = PCD_TRANSCEIVE;
		pi->mf_length = 3;
		pi->mf_data[0] = (0x0D << 4) + CID; // Start byte
		pi->mf_data[1] = 0x01 | BIT(4);		// PPS0 ;BIT4:PPS1 transmited
		pi->mf_data[2] = (DSI << 2) | DRI;	// PPS1
		status = pcd_com_transceive(pi);
		if (status == HAL_STATUS_OK)
		{
			if (pi->mf_length == 8 && pi->mf_data[0] == ((0x0D << 4) + CID))
			{ // PPS ok
				if (NFC_DEBUG_LOG)
					printf("pcd_pps_rate OK\n");
				if (rate == 1)
				{
					// pps到106k
				}
				else if (rate == 2)
				{
					// pps到212k
					pcd_set_rate('2', 'A'); // 212kbps
				}
				else if (rate == 3)
				{
					// pps到424k
					pcd_set_rate('4', 'A'); // 424kbps
				}
				else if (rate == 4)
				{
					// pps到848k
					pcd_set_rate('8', 'A'); // 848kbps
				}
			}
		}
	}

	return status;
}

/**
 * @brief PCD发送数据
 * @param data 发送数据缓冲区
 * @param tx_len 发送数据长度
 * @return 0:成功, <0:失败
 */
int pcd_transmit(uint8_t *data, uint8_t tx_len)
{
	uint8_t i;
	int status;

	hal_nfc_transceive_t pi;
	if (NFC_DEBUG_LOG)
		printf("pcd_transmit   : \n");
	nfc_write_reg(REG_BITFRAMING, 0x00);   // // Tx last bits = 0, rx align = 0
	nfc_set_reg_bit(REG_TXMODE, BIT(7));   // 不使能发送crc
	nfc_clear_reg_bit(REG_RXMODE, BIT(7)); // 不使能接收crc

	pcd_set_tmo(5);

	pi.type = HAL_TECH_TYPE_A;
	pi.mf_command = PCD_TRANSMIT;
	pi.mf_length = tx_len;
	memcpy(&pi.mf_data[0], data, tx_len);

	status = pcd_com_transceive(&pi);

	return status;
}

/**
 * @brief PCD接收数据（启动接收）
 * @return 0:成功, <0:失败
 */
int pcd_receive(void)
{
	uint8_t i;
	int status;

	hal_nfc_transceive_t pi;

	pi.type = HAL_TECH_TYPE_A;
	pi.mf_command = PCD_RECEIVE;
	pi.mf_length = 0;

	pcd_set_tmo(7);
	status = pcd_com_transceive(&pi);

	return status;
}

/**
 * @brief 从FIFO读取接收数据
 * @param rx_data 接收数据缓冲区
 * @param rx_len 接收数据长度（输入：缓冲区大小，输出：实际接收长度）
 * @return 0:成功, <0:失败
 */
int read_fifo(uint8_t *rx_data, uint8_t *rx_len)
{
	uint8_t val;
	uint8_t val_num;
	uint16_t data_length = 0;
	int status = 0;
	uint8_t last_bits;
	uint8_t j;
	uint16_t len_rest = 0;

	uint8_t temp = 0;
	nfc_read_reg(REG_COMIRQ, &val);

	if (val & RX_IRQ)
	{
		if (!(val & ERR_IRQ))
		{
			hal_nfc_read_register(REG_FIFOLEVEL, &val_num);
			
			val_num = 0x7F & val_num;
			hal_nfc_read_register(REG_RXMODE, &temp);
			if (temp & BIT(7))
				val_num = val_num - 1;
			hal_nfc_read_register(REG_CONTROL, &temp);
			last_bits = temp & 0x07;
			if (last_bits && val_num) //
			{
				data_length = (val_num - 1) * 8 + last_bits;
			}
			else
			{
				data_length = val_num * 8;
			}
			data_length += len_rest * 8;
			if (val_num == 0)
			{
				val_num = 1;
			}
			for (j = 0; j < val_num; j++)
			{
				hal_nfc_read_register(REG_FIFODATA, &rx_data[len_rest + j]);
			}
			*rx_len = data_length / 8 + !!(data_length % 8);

			return HAL_STATUS_OK;
		}
		else
		{
			status = HAL_STATUS_ERR;
		}
	}
	else if (val & TIMER_IRQ)
		return HAL_STATUS_TIMEOUT;
	else
		return HAL_STATUS_ERR;

	return status;
}

/**
 * @brief APDU数据交换
 * @param data 发送数据缓冲区
 * @param tx_len 发送数据长度
 * @param rx_data 接收数据缓冲区
 * @param rx_len 接收数据长度（输入：缓冲区大小，输出：实际接收长度）
 * @return 0:成功, <0:失败
 */
int apdu_exchange(uint8_t *data, uint8_t tx_len, uint8_t *rx_data, uint8_t *rx_len)
{
	int status;
	int irq_status = HAL_STATUS_TIMEOUT;
	tick system_tick;

	printf("apdu_exchange   : \n");
	reader_mode_flag = 1;
	// NVIC_EnableIRQ(EXIT_IRQ_NUM);
	status = pcd_transmit(data, tx_len);
	printf("pcd_transmit status   : %d\n", status);
	if (status != 0)
		return status;
	system_tick = get_tick();
	status = pcd_receive();
	while (!is_timeout(system_tick, 1000))
	{
		/****do other process****/
		printf("wait irq   : \n");
		mdelay(30);

		if (read_fifo_flag)
		{
			irq_status = read_fifo(rx_data, rx_len);
			printf("read_fifo_flag irq_status   : %d\n", irq_status);
			return irq_status;
		}
	};
	reader_mode_flag = 0;
	return status;
}

int apdu(uint8_t *data, uint8_t tx_len, uint8_t *rx_data, uint8_t *rx_len)
{
	uint8_t i;
	int status;
	uint8_t snr_check;

	hal_nfc_transceive_t pi;

	if (NFC_DEBUG_LOG)
		printf("apdu   : \n");
	;
	nfc_write_reg(REG_BITFRAMING, 0x00);   // // Tx last bits = 0, rx align = 0
	nfc_set_reg_bit(REG_TXMODE, BIT(7));   // 使能发送crc
	nfc_clear_reg_bit(REG_RXMODE, BIT(7)); // 不使能接收crc
	nfc_set_reg(1, 0x17, 0x92, 0);
	nfc_set_reg(4, 0x3f, 0x52, 1);

	pcd_set_tmo(14);

	pi.type = HAL_TECH_TYPE_A;
	pi.mf_command = PCD_TRANSCEIVE;
	pi.mf_length = tx_len;
	memcpy(&pi.mf_data[0], data, tx_len);

	status = pcd_com_transceive(&pi);
	if (status == MI_OK)
	{
		*rx_len = (pi.mf_length / 8) - 2;
		printf("rx_len %d\r\n", *rx_len);
		for (int i = 0; i < *rx_len; i++)
		{
			rx_data[i] = pi.mf_data[i];
		}
	}
	return status;
}
