#include "main.h"

extern uint8_t EMV_TEST;
extern uint8_t NFC_DEBUG_LOG;


unsigned short gausMaxFrameSizeTableb[] = 
{
    16,  24,  32,  40,  48,  64,  96,  128, 256, 
    256, 256, 256, 256, 256, 256, 256, 256, 256,
};

int pcd_request_b(uint8_t req_code, uint8_t AFI, uint8_t N, uint8_t *ATQB)
{
	int  status;
	
	hal_nfc_transceive_t pi;

    if(NFC_DEBUG_LOG)
    {
       printf("REQB/WUPB:\n"); 
    }
    if(EMV_TEST)
    {
        // disable emd
        hal_nfc_bit_clear(1, 0x18, 1, 0);
        hal_nfc_set_register(10, 0x3f, 0x80, 1);
        pcd_set_tmo_FWT_ATQB();
    }
    else
    {
        pcd_set_tmo(5);
    }
	pi.type = HAL_TECH_TYPE_B;
	pi.mf_command = PCD_TRANSCEIVE;
    pi.mf_length = 3;
    pi.mf_data[0] = ISO14443B_ANTICOLLISION;     	       // APf code
    pi.mf_data[1] = AFI;                // 
    pi.mf_data[2] = (req_code & 0x08) | (N&0x07);  // PARAM
 
	status = pcd_com_transceive(&pi);

    if (status == HAL_STATUS_OK && pi.mf_length != 96)
    {       
		status = HAL_STATUS_ERR;   
	}
    if (status == HAL_STATUS_OK) 
    {	
    	memcpy(ATQB, &pi.mf_data[0], 16);
        if (ATQB[0] != 0x50)
        {
            status = HAL_STATUS_PROTOCOL_ERR;
        }
        if (ATQB[10] & BIT3)
        {//protocol error TB304.14, b4=0 in polling,b4=1 in collision
            status = HAL_STATUS_PROTOCOL_ERR;
        }      
        pcd_set_tmo(ATQB[11]>>4); // set FWT 
		g_pcd_module_info.ui_fsc = gausMaxFrameSizeTableb[((ATQB[10] & 0xF0 ) >> 4)];
        g_pcd_module_info.ui_fwi = (ATQB[11]>>4);
			if(pi.mf_length / 8 >= 13)//Byte 4 support
			{
				g_pcd_module_info.ui_sfgi = (ATQB[12] >> 4);
			}
			else
			{
				g_pcd_module_info.ui_sfgi = 0;
			}      
    }
    if(NFC_DEBUG_LOG)
    {
        printf(" sta=%d\n", status);
    }
    return status;
}                      

int pcd_slot_marker(uint8_t N, uint8_t *ATQB)
{
    int status;
	
	hal_nfc_transceive_t pi;

    if(NFC_DEBUG_LOG)
    {
        printf("SLOT:\n");
    }
	
	pcd_set_tmo(5);

    if(!N || N>15)
	{
		status = HAL_STATUS_PARAM_VAL_ERR;	
    }
	else
    {
		pi.type = HAL_TECH_TYPE_B;
		pi.mf_command = PCD_TRANSCEIVE;
		pi.mf_length = 1;
		pi.mf_data[0] = 0x05 |(N << 4); // APn code

		status = pcd_com_transceive(&pi);

	    if (status == HAL_STATUS_OK && pi.mf_length != 96)
	    {   
			status = HAL_STATUS_ERR;   
		}
		if (status == HAL_STATUS_OK) 
		{	
		  memcpy(ATQB, &pi.mf_data[0], 16);
		  pcd_set_tmo(ATQB[11]>>4); // set FWT 
		  g_pcd_module_info.ui_fsc = gausMaxFrameSizeTableb[(ATQB[10] >> 4)];
		  g_pcd_module_info.ui_fwi = (ATQB[11]>>4);
		} 	
    }
	
    return status;
}                      

/*
 * @brief: 修改通信速率并选择PICC
 *
 * @param: 
 * 			CID			寻卡时确定的卡片ID
 * 			ATQB		寻卡返回的数据
 * 			rate		交互速率
 * 			at_attrib	返回给用户的ATTRIB的返回数据
 * 
 * @return: HAL_STATUS_OK -- 成功；其他值 -- 失败
 */
int pcd_pps_rate_b(uint8_t CID, uint8_t *ATQB, uint8_t rate)
{
	int status;
	uint8_t dsi_dri;
	
	if (rate == 1)
	{
		dsi_dri = 0;
	}
	else if (rate == 2)
	{
      if(NFC_DEBUG_LOG)
      {
         printf("212K\n"); 
      }
		
		dsi_dri = BIT(2) | BIT(0);
	}
	else if (rate == 4)
	{
     if(NFC_DEBUG_LOG)
     {
         printf("424K\n");
     }
		
		dsi_dri = BIT(3) | BIT(1);
	}
	else if (rate == 8)
	{
      if(NFC_DEBUG_LOG)
      {
          printf("848K\n");
      }
		
		dsi_dri = BIT(3) | BIT(2) | BIT(1) | BIT(0);
	}
	else
	{
     if(NFC_DEBUG_LOG)
     {
           printf("USER:No Rate select\n");
     }
		
		return HAL_STATUS_USER_ERR;
	}

	status = pcd_attri_b(&ATQB[1], dsi_dri, ATQB[10]&0x0f, CID, ATQB);

	if (status == HAL_STATUS_OK)
	{
      if(NFC_DEBUG_LOG)
      {
           printf("pps ok\n");	
      }
		
		if (rate == 1)
		{
			pcd_set_rate('1', 'B');// 106kbps
		}
		else if (rate == 2)
		{
			pcd_set_rate('2', 'B');// 212kbps
		}
		else if (rate == 4)
		{
			pcd_set_rate('4', 'B');// 424kbps
		}
		else if (rate == 8)
		{
			pcd_set_rate('8', 'B');// 848kbps
		}
	}
	else
	{
		printf("pps fail\n");
	}
	
	
	return status;
}             

/*
 * ATTRIB
 *
 * @brief: 选择PICC
 * 
 * @param: 
 * 			*PUPI			4字节PICC标识符
 * 			dis_dri			PCD <--> PICC 速率选择
 * 			pro_type		支持的协议，由请求回应中的ProtocolType指定
 * 			*answer			ATTRIB命令的返回
 * 
 * @return: HAL_STATUS_OK -- 成功；其他值 -- 失败
 */
int pcd_attri_b(uint8_t *PUPI, uint8_t dsi_dri, uint8_t pro_type, uint8_t CID, uint8_t *answer)
{
    int  status;
	
	hal_nfc_transceive_t pi;
	pro_type = pro_type;

    if(NFC_DEBUG_LOG)
    {
         printf("ATTRIB:\n");
    }
	
	
	/*initialiszed the PCB*/
    g_pcd_module_info.uc_pcd_pcb  = 0x02;
    g_pcd_module_info.uc_picc_pcb = 0x03;
    g_pcd_module_info.uc_cid = CID;

    if( EMV_TEST)
    {
        //enable emd
        hal_nfc_bit_set(1, 0x18, 1, 0);
        hal_nfc_set_register(10, 0x3f, 0x00, 1);
        //according to 14443-3the waiting time for the Answer to ATTRIB command is a fixed value given by the following formula: 
        //Answer to ATTRIB waiting time = (256 x 16 / fc ) x 2^4,(~ 4,8 ms)
        if (g_pcd_module_info.ui_fwi > 4)//dFWT
        {
            pcd_set_tmo(g_pcd_module_info.ui_fwi);// set FWT after Attrib
        }
    }
    else
    {
        pcd_delay_sfgi(g_pcd_module_info.ui_sfgi);
        pcd_set_tmo(g_pcd_module_info.ui_fwi);
    }

    pi.mf_command = PCD_TRANSCEIVE;
    pi.mf_length  = 9;
    pi.mf_data[0] = ISO14443B_ATTRIB;
    memcpy(&pi.mf_data[1], PUPI, 4);
    
    pi.mf_data[5] = 0x00;  	    // EOF/SOF required, default TR0/TR1
    hal_nfc_set_register_bit(REG_TYPEB, BIT(7) | BIT(6)); //EOF SOF required
    
    pi.mf_data[6] = ((dsi_dri << 4) | FSDI); //FSDI; // Max frame 64 
    pi.mf_data[7] = 0x01; //pro_type & 0x0f;  //;  ISO/IEC 14443-4 compliant?;
    pi.mf_data[8] = (CID & 0x0f); //   	    // CID ,0 - 14
    
    status  = pcd_com_transceive(&pi);

    if(EMV_TEST)
    {
        //TB305.5
        if (status != HAL_STATUS_OK && status != HAL_STATUS_PROTOCOL_ERR && pi.mf_length == 0)
        {
            pcd_delay_sfgi(7);//EMV2.6 tMIN,RETRANSMISSION 3 ms TB305.5
            status = HAL_STATUS_TIMEOUT;
        }
        //TB306.11
        if (status == HAL_STATUS_OK && pi.mf_length != 8)
        {
            status = HAL_STATUS_PROTOCOL_ERR;
        }
        //TB306.10
        if (status == HAL_STATUS_OK && (pi.mf_data[0] & 0x0F) != CID)
        {
            status = HAL_STATUS_PROTOCOL_ERR;
        }
    }
    if (status == HAL_STATUS_OK)
    {	
    	if(answer!=NULL)
    	{
            answer[0] = pi.mf_length/8;
            memcpy(&answer[1], pi.mf_data, answer[0]);
        }
    } 	

    if(NFC_DEBUG_LOG)
    {
        printf(" sta=%d\n", status);
    }
    

    return status;
} 
//////////////////////////////////////////////////////////////////////
//REQUEST B
//////////////////////////////////////////////////////////////////////
int get_idcard_num(uint8_t *pid)
{
    int status;
	
	hal_nfc_transceive_t pi;

    if(NFC_DEBUG_LOG)
    {
        printf("ID_NUM:\n");
    }
	
	//pcd_set_tmo(5);
	
    pi.mf_command = PCD_TRANSCEIVE;
    pi.mf_length  =5;
    pi.mf_data[0] =0x00; //ISO14443B_ANTICOLLISION;     	       // APf code
    pi.mf_data[1] =0x36;// AFI;                // 
    pi.mf_data[2] =0x00; //((req_code<<3)&0x08) | (N&0x07);  // PARAM
	pi.mf_data[3] =0x00;
	pi.mf_data[4] =0x08;
 
    status = pcd_com_transceive(&pi);

    if (status == HAL_STATUS_OK) 
    {	
    	memcpy(pid, &pi.mf_data[0], 10);
    } 
    return status;
}       

int pcd_halt_b(uint8_t *PUPI)
{
    int status;
	
	hal_nfc_transceive_t pi;
	
    if(NFC_DEBUG_LOG)
    {
         printf("HALTB:\n");
    }
	
    
    hal_nfc_write_register(REG_BITFRAMING,0x00);	// // Tx last bits = 0, rx align = 0
	hal_nfc_set_register_bit(REG_TXMODE, BIT(7)); //使能发送crc
	hal_nfc_set_register_bit(REG_RXMODE, BIT(7)); //使能接收crc

    pcd_set_tmo(g_pcd_module_info.ui_fwi);
				                               // disable, ISO/IEC3390 enable	
    pi.mf_command = PCD_TRANSCEIVE;
    pi.mf_length  = 5;
    pi.mf_data[0] = ISO14443B_HLTB;
    memcpy(&pi.mf_data[1], PUPI, 4);
    
    status = pcd_com_transceive(&pi);
    {
        if(NFC_DEBUG_LOG)
        {
             printf(" sta=%d\n", status);
        }
    }
	

    return status;
}  

/**
 ****************************************************************
 * @brief select_sr() 
 *
 * 防冲撞函数
 * @param: 
 * @param: 
 * @return: status 值为MI_OK:成功
 * @retval: chip_id  得到的SR卡片的chip_id
 ****************************************************************
 */
char select_sr(uint8_t *chip_id)
{
	int status;
	hal_nfc_transceive_t pi;

    {
        if(NFC_DEBUG_LOG)
        {
             printf("SELECT_SR:\n");
        }
    }
	
    pcd_set_tmo(5);
    pi.mf_command = PCD_TRANSCEIVE;
    pi.mf_length  = 2;
    pi.mf_data[0] = 0x06;     	       //initiate card
    pi.mf_data[1] = 0;                
    
    status = pcd_com_transceive(&pi);

    if (status!=MI_OK && status!=MI_NOTAGERR) 
    {   status = MI_COLLERR;   }          // collision occurs
    
    if(pi.mf_length != 8)
    {   status = MI_COM_ERR;   }
    
    if (status == MI_OK)
    {	
         pcd_set_tmo(5);
         pi.mf_command = PCD_TRANSCEIVE;
         pi.mf_length  = 2;
         pi.mf_data[1] = pi.mf_data[0];     	       
         pi.mf_data[0] = 0x0E;                 // Slect card
         
         status = pcd_com_transceive(&pi); 
         
         if (status!=MI_OK && status!=MI_NOTAGERR)  // collision occurs
         {   status = MI_COLLERR;   }               // collision occurs
         if (pi.mf_length != 8) 
         {   status = MI_COM_ERR;     }
         if (status == MI_OK)
         {  *chip_id = pi.mf_data[0];  }
    } 	
    if(NFC_DEBUG_LOG)
    {
         printf(" sta=%d\n", status);
    }
    
    return status;
}  

//////////////////////////////////////////////////////////////////////
//SR176卡读块
//////////////////////////////////////////////////////////////////////
char read_sr176(uint8_t addr, uint8_t *readdata)
{
    int status;
	hal_nfc_transceive_t pi;
	
    if(NFC_DEBUG_LOG)
    {
         printf("read_sr176()->\n");
    }
	
    pcd_set_tmo(5);
    pi.mf_command = PCD_TRANSCEIVE;
    pi.mf_length  = 2;
    pi.mf_data[0] = 0x08;
    pi.mf_data[1] = addr;
  
    status = pcd_com_transceive(&pi);
  
    if ((status==MI_OK) && (pi.mf_length!=16))
    {   status = MI_BITCOUNTERR;    }
    if (status == MI_OK)
    {
        *readdata     = pi.mf_data[0];
        *(readdata+1) = pi.mf_data[1];
    }
    if(NFC_DEBUG_LOG)
    {
        printf(" sta=%d\n", status);
    }
	

    return status;  
}  
//////////////////////////////////////////////////////////////////////
//SR176卡写块
//////////////////////////////////////////////////////////////////////
char write_sr176(uint8_t addr, uint8_t *writedata)
{
    int status;
	hal_nfc_transceive_t pi;
    if(NFC_DEBUG_LOG)
    {
        printf("write_sr176()->\n");
    }
	
    pcd_set_tmo(5);
    pi.mf_command = PCD_TRANSMIT;
    pi.mf_length  = 4;
    pi.mf_data[0] = 9;
    pi.mf_data[1] = addr;
    pi.mf_data[2] = *writedata;
    pi.mf_data[3] = *(writedata+1);
    status = pcd_com_transceive(&pi);
    if(NFC_DEBUG_LOG)
    {
         printf(" sta=%d\n", status);
    }
	

    return status;  
}   

//////////////////////////////////////////////////////////////////////
//SR176卡块锁定
//////////////////////////////////////////////////////////////////////
char protect_sr176(uint8_t lockreg)
{
    int status;
	hal_nfc_transceive_t pi;

    if(NFC_DEBUG_LOG)
    {
         printf("protect_sr176()->\n");
    }
	

    pcd_set_tmo(5);
    pi.mf_command = PCD_TRANSMIT;
    pi.mf_length  = 4;
    pi.mf_data[0] = 0x09;
    pi.mf_data[1] = 0x0F;
    pi.mf_data[2] = 0;
    pi.mf_data[3] = lockreg;
    status = pcd_com_transceive(&pi);
	
    if(NFC_DEBUG_LOG)
    {
         printf(" sta=%d\n", status);
    }
	

    return status;  
}   

//////////////////////////////////////////////////////////////////////
//COMPLETION ST
//////////////////////////////////////////////////////////////////////
char completion_sr(void)
{
    int status;
	hal_nfc_transceive_t pi;
    if(NFC_DEBUG_LOG)
    {
         printf("completion_sr()->\n");
    }
	
    pcd_set_tmo(5);
    pi.mf_command = PCD_TRANSMIT;
    pi.mf_length  = 1;
    pi.mf_data[0] = 0x0F;
    status = pcd_com_transceive(&pi);
    if(NFC_DEBUG_LOG)
    {
       printf(" sta=%d\n", status); 
    }
	
    return status;  
}    

