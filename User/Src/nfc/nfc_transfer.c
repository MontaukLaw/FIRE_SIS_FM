#include "main.h"

pcd_info_s g_pcd_module_info = {0};

uint8_t NFC_DEBUG_LOG = 0;
uint8_t EMV_DEBUG_LOG = 0;

void pcd_default_info(void) // COS_TEST
{
    memset(&g_pcd_module_info, 0, sizeof(g_pcd_module_info));
    g_pcd_module_info.ui_fsc = 32;
    g_pcd_module_info.ui_fwi = 4;
    g_pcd_module_info.ui_sfgi = 0;
    g_pcd_module_info.uc_nad_en = 0;
    g_pcd_module_info.uc_cid_en = 0;
    g_pcd_module_info.uc_wtxm = 1; // multi
}

uint8_t get_fifo_level(void)
{
    uint8_t val;
    nfc_read_reg(REG_FIFOLEVEL, &val);
    return val & 0x7F;
}

int pcd_com_transceive(hal_nfc_transceive_t *pi)
{

    uint8_t recebyte;
    int status = HAL_STATUS_OK;
    uint8_t irq_en;
    uint8_t wait_for;
    uint8_t last_bits;
    uint8_t j;
    uint8_t val;
    uint8_t err;
    uint16_t len_rest;
    uint8_t len;
    uint8_t WATER_LEVEL;
    uint8_t tmp;

    len = 0;
    len_rest = 0;
    err = 0;
    recebyte = 0;
    irq_en = 0;
    wait_for = 0;

    switch (pi->mf_command)
    {
    case PCD_IDLE:
        irq_en = 0x00;
        wait_for = 0x00;
        break;
    case PCD_AUTHENT:
        irq_en = IDEL_IEN | TIMER_IEN;
        wait_for = IDEL_IRQ;
        break;
    case PCD_RECEIVE:
        irq_en = RX_IEN | IDEL_IEN;
        wait_for = RX_IRQ;
        recebyte = 1;
        break;
    case PCD_TRANSMIT:
        irq_en = TX_IEN | IDEL_IEN;
        wait_for = TX_IRQ;
        break;
    case PCD_TRANSCEIVE:
        irq_en = RX_IEN | IDEL_IEN | TIMER_IEN | TX_IEN;
        wait_for = RX_IRQ;
        recebyte = 1;
        break;
    default:
        pi->mf_command = HAL_STATUS_UNKNOWN_CMD;
        break;
    }

    nfc_read_reg(REG_WATERLEVEL, &WATER_LEVEL);

    if (pi->mf_command != HAL_STATUS_UNKNOWN_CMD && (((pi->mf_command == PCD_TRANSCEIVE || pi->mf_command == PCD_TRANSMIT) && pi->mf_length > 0) || (pi->mf_command != PCD_TRANSCEIVE && pi->mf_command != PCD_TRANSMIT)))
    {
        nfc_write_reg(REG_COMMAND, PCD_IDLE);
        nfc_write_reg(REG_COMIEN, /*irq_inv |*/ irq_en | BIT(0)); //
        nfc_write_reg(REG_COMIRQ, 0x7F);                          // Clear INT
        nfc_write_reg(REG_DIVIRQ, 0x7F);                          // Clear INT
        // Flush Fifo
        nfc_set_reg_bit(REG_FIFOLEVEL, BIT(7));
        if (pi->mf_command == PCD_TRANSCEIVE || pi->mf_command == PCD_TRANSMIT || pi->mf_command == PCD_AUTHENT)
        {
            if (NFC_DEBUG_LOG)
                printf(" PCD_tx:");
            for (j = 0; j < pi->mf_length; j++)
            {

                if (NFC_DEBUG_LOG)
                    printf("%02x ", pi->mf_data[j]);
            }
            if (NFC_DEBUG_LOG)
                printf("\n");
            if (EMV_TEST)
            {
                if (EMV_DEBUG_LOG)
                {
                    printf("C ->: ");
                    for (j = 0; j < pi->mf_length; j++)
                    {
                        if (j > 4)
                        {
                            printf("..");
                            break;
                        }
                        else
                        {
                            printf("%02X ", (uint8_t)pi->mf_data[j]);
                        }
                    }
                    printf("l=%d", pi->mf_length);
                    printf("\n");
                }
            }
            len_rest = pi->mf_length;
            if (len_rest >= FIFO_SIZE)
            {
                len = FIFO_SIZE;
            }
            else
            {
                len = len_rest;
            }

            for (j = 0; j < len; j++)
            {
                nfc_write_reg(REG_FIFODATA, pi->mf_data[j]);
            }
            len_rest -= len; // Rest bytes
            if (len_rest != 0)
            {
                nfc_write_reg(REG_COMIRQ, BIT(2));   // clear LoAlertIRq
                nfc_set_reg_bit(REG_COMIEN, BIT(2)); // enable LoAlertIRq
            }

            nfc_write_reg(REG_COMMAND, pi->mf_command);
            if (pi->mf_command == PCD_TRANSCEIVE)
            {
                nfc_set_reg_bit(REG_BITFRAMING, 0x80);
            }

            while (len_rest != 0)
            {
                while (HAL_GPIO_ReadPin(G_SENSOR_INT_PORT, G_SENSOR_INT_PIN) == 0)
                    ; // Wait LoAlertIRq

                if (len_rest > (FIFO_SIZE - WATER_LEVEL))
                {
                    len = FIFO_SIZE - WATER_LEVEL;
                }
                else
                {
                    len = len_rest;
                }
                for (j = 0; j < len; j++)
                {
                    nfc_write_reg(REG_FIFODATA, pi->mf_data[pi->mf_length - len_rest + j]);
                }

                nfc_write_reg(REG_COMIRQ, BIT(2)); //

                // printf("\n8 comirq=%02x,ien=%02x,INT= %d \n", (u16)read_reg(ComIrqReg), (u16)read_reg(ComIEnReg), (u16)INT_PIN);
                len_rest -= len; // Rest bytes
                if (len_rest == 0)
                {
                    nfc_clear_reg_bit(REG_COMIEN, BIT(2)); // disable LoAlertIRq
                    // printf("\n9 comirq=%02x,ien=%02x,INT= %d \n", (u16)read_reg(ComIrqReg), (u16)read_reg(ComIEnReg), (u16)INT_PIN);
                }
            }
            // Wait TxIRq
            while (HAL_GPIO_ReadPin(G_SENSOR_INT_PORT, G_SENSOR_INT_PIN) == 0)
            {
            }

            nfc_read_reg(REG_COMIRQ, &tmp);
            if (tmp & TX_IRQ)
            {
                nfc_write_reg(REG_COMIRQ, TX_IRQ);
                if (pi->mf_command == PCD_TRANSMIT)
                {
                    return 0;
                }
            }
        }
        if (PCD_RECEIVE == pi->mf_command)
        {
            nfc_write_reg(REG_COMMAND, PCD_RECEIVE);
            nfc_clear_reg_bit(REG_TMODE, BIT(7));
            nfc_set_reg_bit(REG_CONTROL, BIT(6)); // TStartNow
            return 0;
        }

        len_rest = 0;                        // bytes received
        nfc_write_reg(REG_COMIRQ, BIT(3));   // clear HoAlertIRq
        nfc_set_reg_bit(REG_COMIEN, BIT(3)); // enable HoAlertIRq

        if (!EMV_TEST)
        {
            while (status != HAL_STATUS_TIMEOUT)
            {
                while (HAL_GPIO_ReadPin(G_SENSOR_INT_PORT, G_SENSOR_INT_PIN) == 0)
                {
                }
                while (1)
                {
                    while (HAL_GPIO_ReadPin(G_SENSOR_INT_PORT, G_SENSOR_INT_PIN) == 0)
                        ;
                    nfc_read_reg(REG_COMIRQ, &val);

                    if ((val & BIT(3)) && !(val & BIT(5)))
                    {
                        for (j = 0; j < FIFO_SIZE - WATER_LEVEL; j++)
                        {
                            nfc_read_reg(REG_FIFODATA, &pi->mf_data[len_rest + j]);
                        }
                        nfc_write_reg(REG_COMIRQ, BIT(3)); //
                        len_rest += FIFO_SIZE - WATER_LEVEL;
                    }
                    else
                    {
                        nfc_clear_reg_bit(REG_COMIEN, BIT(3)); // disable HoAlertIRq
                        break;
                    }
                }

                nfc_read_reg(REG_COMIRQ, &val);

                nfc_write_reg(REG_COMIRQ, val);

                if (val & BIT(0))
                {
                    status = HAL_STATUS_TIMEOUT;
                }
                else
                {
                    nfc_read_reg(REG_ERROR, &err);

                    status = HAL_STATUS_ERR;
                    if ((val & wait_for) && (val & irq_en))
                    {
                        if (!(val & ERR_IRQ))
                        { //
                            status = HAL_STATUS_OK;

                            if (recebyte)
                            {
                                nfc_read_reg(REG_FIFOLEVEL, &val);
                                val = 0x7F & val;
                                //  printf("%02X ", pi->mf_data[j]);
                                nfc_read_reg(REG_RXMODE, &tmp);
                                if (tmp & BIT(2))
                                    val = val - 1;
                                nfc_read_reg(REG_CONTROL, &last_bits);
                                last_bits = last_bits & 0x07;

                                if (last_bits && val) //
                                {
                                    pi->mf_length = (val - 1) * 8 + last_bits;
                                }
                                else
                                {
                                    pi->mf_length = val * 8;
                                }
                                pi->mf_length += len_rest * 8;

                                if (NFC_DEBUG_LOG)
                                    printf(" RX:len=%02x,dat:", pi->mf_length);

                                if (val == 0)
                                {
                                    val = 1;
                                }
                                for (j = 0; j < val; j++)
                                {
                                    nfc_read_reg(REG_FIFODATA, &pi->mf_data[len_rest + j]);
                                }

                                if (NFC_DEBUG_LOG)
                                {
                                    for (j = 0; j < pi->mf_length / 8 + !!(pi->mf_length % 8); j++)
                                    {
                                        if (j > 24)
                                        {
                                            printf("..");
                                            break;
                                        }
                                        else
                                        {
                                            printf("%02X ", pi->mf_data[j]);
                                        }
                                    }
                                    printf("l=%d", pi->mf_length / 8 + !!(pi->mf_length % 8));
                                    printf("\n");
                                }
                            }
                        }
                        else if (err & COLL_ERR)
                        {
                            printf("COLL ERR\r\n");
                            status = HAL_STATUS_COLLERR;
                            if (pi->type == HAL_TECH_TYPE_A)
                            {
                                // a bit-collision is detected
                                if (recebyte)
                                {
                                    val = get_fifo_level();
                                    nfc_read_reg(REG_CONTROL, &last_bits);
                                    last_bits = last_bits & 0x07;
                                    if (len_rest + val > MAX_TRX_BUF_SIZE)
                                    { //
                                        if (NFC_DEBUG_LOG)
                                            printf("COLL RX_LEN > 255B\n");
                                    }
                                    else
                                    {
                                        if (last_bits && val) //
                                        {
                                            pi->mf_length = (val - 1) * 8 + last_bits;
                                        }
                                        else
                                        {
                                            pi->mf_length = val * 8;
                                        }
                                        pi->mf_length += len_rest * 8;
                                        if (NFC_DEBUG_LOG)
                                            printf(" RX: pi_cmd=%02x,pi_len=%02x,pi_dat:", pi->mf_command, pi->mf_length);
                                        if (val == 0)
                                        {
                                            val = 1;
                                        }
                                        for (j = 0; j < val; j++)
                                        {
                                            nfc_read_reg(REG_FIFODATA, &pi->mf_data[len_rest + j + 1]);
                                        }
                                        if (NFC_DEBUG_LOG)
                                        {
                                            for (j = 0; j < pi->mf_length / 8 + !!(pi->mf_length % 8); j++)
                                            {
                                                printf("%02X ", pi->mf_data[j + 1]);
                                            }
                                            printf("\n");
                                        }
                                    }
                                }
                                /* The location where the collision occurred is in the first 32 bits, otherwise it is outside the 32 bits */
                                nfc_read_reg(REG_COLL, &val);
                                if (!(val & BIT(5)))
                                {
                                    nfc_read_reg(REG_COLL, &val);
                                    pi->mf_data[0] = val & COLLPOS;
                                    if (pi->mf_data[0] == 0)
                                    {
                                        pi->mf_data[0] = 32;
                                    }
                                }
                                else
                                { // This value is used to tell the upper layer that the conflict position is more than 32 bits
                                    pi->mf_data[0] = 33;
                                }
                                if (NFC_DEBUG_LOG)
                                {
                                    printf("\n COLL_DET pos=%02x\n", pi->mf_data[0]);
                                }

                                pi->mf_data[0]--; //
                            }
                        }
                        else if (err & (PROTOCOL_ERR))
                        {
                            if (NFC_DEBUG_LOG)
                                printf("protocol err=%02x\n", err);
                            status = HAL_STATUS_FRAMINGERR;
                        }
                        else if ((err & (CRC_ERR | PARITY_ERR)) && !(err & PROTOCOL_ERR))
                        {
                            // EMV  parity err EMV 307.2.3.4
                            val = get_fifo_level();
                            nfc_read_reg(REG_CONTROL, &last_bits);
                            last_bits = last_bits & 0x07;
                            if (len_rest + val > MAX_TRX_BUF_SIZE)
                            {
                                status = HAL_STATUS_ERR;
                                if (NFC_DEBUG_LOG)
                                    printf("RX_LEN > 255B\n");
                            }
                            else
                            {
                                if (last_bits && val)
                                {
                                    pi->mf_length = (val - 1) * 8 + last_bits;
                                }
                                else
                                {
                                    pi->mf_length = val * 8;
                                }
                                pi->mf_length += len_rest * 8;
                            }
                            if (NFC_DEBUG_LOG)
                            {
                                printf("crc-parity err=%02x\n", err);
                                printf("l=%d\n", pi->mf_length);
                            }

                            status = HAL_STATUS_INTEGRITY_ERR;
                        }
                        else
                        {
                            if (NFC_DEBUG_LOG)
                                printf("unknown ErrorReg=%02x\n", err);
                            status = HAL_STATUS_INTEGRITY_ERR;
                        }
                    }
                    else
                    {
                        status = HAL_STATUS_ERR;
                        if (NFC_DEBUG_LOG)
                            printf(" MI_COM_ERR\n");
                    }
                }

                nfc_read_reg(REG_RXMODE, &val);
                if (!(val & BIT(2)))
                {
                    break;
                }
                else if ((status == HAL_STATUS_TIMEOUT) && (pi->mf_length != 0))
                {
                    status = HAL_STATUS_OK;
                    break;
                }
                else
                {
                    nfc_read_reg(REG_RXMODE, &val);
                    if (val & BIT(2))
                    {
                        break;
                    }
                    nfc_write_reg(REG_FIFOLEVEL, BIT(7)); // Flush Fifo
                }
            }
        } //! emv_test end

        else
        { // emv_test start
            while (1)
            {
                while (0 == HAL_GPIO_ReadPin(G_SENSOR_INT_PORT, G_SENSOR_INT_PIN))
                    ;

                nfc_read_reg(REG_COMIRQ, &val);
                if ((val & BIT(3)) && !(val & BIT(5)))
                {
                    for (j = 0; j < FIFO_SIZE - WATER_LEVEL; j++)
                    {
                        nfc_read_reg(REG_FIFODATA, &pi->mf_data[len_rest + j]);
                    }
                    nfc_write_reg(REG_COMIRQ, BIT(3)); //
                    len_rest += FIFO_SIZE - WATER_LEVEL;
                    // if emd enable,timer must stop
                    hal_nfc_bit_set(0, 0x0c, 0x80, 0);
                }
                else
                {
                    nfc_clear_reg_bit(REG_COMIEN, BIT(3)); // disable HoAlertIRq
                    break;
                }
            }
            nfc_read_reg(REG_COMIRQ, &val);
            nfc_write_reg(REG_COMIRQ, val); //

            if (val & BIT(0))
            { //
                status = HAL_STATUS_TIMEOUT;
            }
            else
            {
                nfc_read_reg(REG_ERROR, &err);

                status = HAL_STATUS_ERR;
                if ((val & wait_for) && (val & irq_en))
                {
                    if (!(val & ERR_IRQ))
                    { //
                        status = HAL_STATUS_OK;

                        if (recebyte)
                        {
                            val = get_fifo_level();
                            nfc_read_reg(REG_RXMODE, &tmp);
                            if (tmp & BIT(2))
                                val = val - 1;
                            nfc_read_reg(REG_CONTROL, &tmp);
                            last_bits = tmp & 0x07;
                            if (last_bits && val) //
                            {
                                pi->mf_length = (val - 1) * 8 + last_bits;
                            }
                            else
                            {
                                pi->mf_length = val * 8;
                            }
                            pi->mf_length += len_rest * 8;

                            if (val == 0)
                            {
                                val = 1;
                            }
                            for (j = 0; j < val; j++)
                            {
                                nfc_read_reg(REG_FIFODATA, &pi->mf_data[len_rest + j]);
                            }
                            if (EMV_DEBUG_LOG)
                            {
                                printf("R <-: ");
                                for (j = 0; j < pi->mf_length / 8 + !!(pi->mf_length % 8); j++)
                                {
                                    if (j > 4)
                                    {
                                        printf("..");
                                        break;
                                    }
                                    else
                                    {
                                        printf("%02X ", (uint8_t)pi->mf_data[j]);
                                    }
                                }
                                printf("l=%d", pi->mf_length / 8 + !!(pi->mf_length % 8));
                                printf("\n");
                            }
                        } // receivebyte end
                    }
                    else if (err & COLL_ERR)
                    {
                        status = HAL_STATUS_COLLERR;
                        if (pi->type == HAL_TECH_TYPE_A)
                        {
                            // a bit-collision is detected
                            if (recebyte)
                            {
                                val = get_fifo_level();

                                nfc_read_reg(REG_CONTROL, &tmp);

                                last_bits = tmp & 0x07;
                                if (last_bits && val) //
                                {
                                    pi->mf_length = (val - 1) * 8 + last_bits;
                                }
                                else
                                {
                                    pi->mf_length = val * 8;
                                }
                                pi->mf_length += len_rest * 8;
                                if (val == 0)
                                {
                                    val = 1;
                                }
                                for (j = 0; j < val; j++)
                                {
                                    nfc_read_reg(REG_FIFODATA, &pi->mf_data[len_rest + j + 1]);
                                }
                            }
                            /* The location where the collision occurred is in the first 32 bits, otherwise it is outside the 32 bits */
                            nfc_read_reg(REG_COLL, &tmp);
                            if (!(tmp & BIT(5)))
                            {
                                nfc_read_reg(REG_COLL, &pi->mf_data[0]);
                                pi->mf_data[0] &= COLLPOS;
                                if (pi->mf_data[0] == 0)
                                {
                                    pi->mf_data[0] = 32;
                                }
                            }
                            else
                            { // This value is used to tell the upper layer that the conflict position is more than 32 bits
                                pi->mf_data[0] = 33;
                            }
                            pi->mf_data[0]--; //
                        }
                        else if (pi->type == HAL_TECH_TYPE_B)
                        {
                        }
                    }
                    else if (err & (PROTOCOL_ERR))
                    {
                        status = HAL_STATUS_FRAMINGERR;
                    }
                    else if ((err & (CRC_ERR | PARITY_ERR)) && !(err & PROTOCOL_ERR))
                    {
                        // EMV  parity err EMV 307.2.3.4
                        val = get_fifo_level();

                        nfc_read_reg(REG_CONTROL, &tmp);
                        last_bits = tmp & 0x07;
                        if (last_bits && val)
                        {
                            pi->mf_length = (val - 1) * 8 + last_bits;
                        }
                        else
                        {
                            pi->mf_length = val * 8;
                        }
                        pi->mf_length += len_rest * 8;

                        status = HAL_STATUS_INTEGRITY_ERR;
                    }
                    else
                    {
                        status = HAL_STATUS_INTEGRITY_ERR;
                    }
                }
                else
                {
                    status = HAL_STATUS_ERR;
                }
            }
        } // eme_test end
        nfc_set_reg_bit(REG_CONTROL, BIT(7));
        nfc_write_reg(REG_COMIRQ, 0x7F);
        nfc_write_reg(REG_DIVIRQ, 0x7F);
        nfc_clear_reg_bit(REG_COMIEN, 0x7F);
        nfc_clear_reg_bit(REG_DIVIEN, 0x7F);
        nfc_write_reg(REG_COMMAND, PCD_IDLE);

    } // all command if
    else // all command if 's else
    {
        status = HAL_STATUS_USER_ERR;
        if (NFC_DEBUG_LOG)
            printf("USER_ERROR\n");
    }

    if (NFC_DEBUG_LOG)
        printf(" pcd_com: sta=%d,err=%02x\n", status, err);
    return status;
}
