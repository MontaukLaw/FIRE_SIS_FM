#ifndef _USER_DEEFINE_H_
#define _USER_DEEFINE_H_

#include "py32f0xx_hal.h"
#include <stdbool.h>
#include <math.h>

/* =========================
 * 板级硬件引脚定义
 * ========================= */
#define LED_W_PORT GPIOC
#define LED_W_PIN GPIO_PIN_1

#define RM_RST_PORT LED_W_PORT
#define RM_RST_PIN LED_W_PIN

#define VOICE_PORT GPIOB
#define VOICE_PIN GPIO_PIN_2

#define RM_VDD_PORT GPIOB
#define RM_VDD_PIN GPIO_PIN_7

#define SC7A20_VDD_PORT GPIOA
#define SC7A20_VDD_PIN GPIO_PIN_6

/* =========================
 * 传感/算法基础参数
 * ========================= */
#define TOTAL_SENSOR_NUMBER 4
#define INIT_ZERO_TIMES 100
#define NOISE_MIN 1.0f

/* K_TH_F 越小越灵敏，越大越保守。 */
#define K_TH_F 5.0f
#define K_TH_OFF_F 2.0f

#define S_NORM_ON 80
#define S_NORM_HOLD_NUM 3
#define S_NORM_HOLD_DEN 4
#define S_NORM_OFF_NUM 1
#define S_NORM_OFF_DEN 2

#define C_ON 2
#define C_HOLD 1
#define C_OFF 0

#define SON_NUM 3.0f
#define SON_DEN 2.0f
#define HOLD_NUM 3.0f
#define HOLD_DEN 3.0f
#define OFF_NUM 3.0f
#define OFF_DEN 4.0f

#define QUITE_TIMES 30
#define OVER_TH_TIMES 200

#define ACTIVATE_BASE_BETA_NUM 1
#define ACTIVATE_BASE_BETA_DEN 3
#define ACTIVATE_NOISE_BETA_NUM 1
#define ACTIVATE_NOISE_BETA_DEN 5

#define BASE_BETA_NUM 1
#define BASE_BETA_DEN 10
#define NOISE_IDEL_NUM 1
#define NOISE_IDEL_DEN 100

#define MAX_SENSORS TOTAL_SENSOR_NUMBER
#define MAX_CH_NMB MAX_SENSORS

/* =========================
 * 中断与休眠
 * ========================= */
#define G_SENSOR_INT_PORT GPIOA
#define G_SENSOR_INT_PIN GPIO_PIN_7

#define SLEEP_CHECK_MAX 5000

/* =========================
 * SPI / NFC 硬件引脚
 * ========================= */
#define SPI_SCK_PORT GPIOB
#define SPI_SCK_PIN GPIO_PIN_0

#define SPI_MISO_PORT GPIOA
#define SPI_MISO_PIN GPIO_PIN_1

#define SPI_MOSI_PORT GPIOA
#define SPI_MOSI_PIN GPIO_PIN_0

#define SPI_NSS_PORT GPIOB
#define SPI_NSS_PIN GPIO_PIN_5

#define NFC_INT_PORT GPIOA
#define NFC_INT_PIN GPIO_PIN_5

#define NFC_PWDN_PORT GPIOB
#define NFC_PWDN_PIN GPIO_PIN_1

/* =========================
 * PCD 命令字
 * ========================= */
#define PCD_IDLE 0x00
#define PCD_AUTHENT 0x0E
#define PCD_RECEIVE 0x08
#define PCD_TRANSMIT 0x04
#define PCD_TRANSCEIVE 0x0C
#define PCD_RESETPHASE 0x0F
#define PCD_CALCRC 0x03

/* =========================
 * 寄存器位定义
 * ========================= */
#define TX_IEN BIT(6)
#define RX_IEN BIT(5)
#define IDEL_IEN BIT(4)
#define ERR_IEN BIT(1)
#define TIMER_IEN BIT(0)

#define TX_IRQ BIT(6)
#define RX_IRQ BIT(5)
#define IDEL_IRQ BIT(4)
#define ERR_IRQ BIT(1)
#define TIMER_IRQ BIT(0)

#define COLL_ERR BIT(3)
#define CRC_ERR BIT(2)
#define PARITY_ERR BIT(1)
#define PROTOCOL_ERR BIT(0)

#define COLLPOS 0x1F

#define RX_ALIGN (BIT(4) | BIT(5) | BIT(6))
#define TX_LAST_BITS (BIT(0) | BIT(1) | BIT(2))

/* =========================
 * HAL 收发状态码
 * ========================= */
#define HAL_STATUS_OK (0)
#define HAL_STATUS_TIMEOUT (-1)
#define HAL_STATUS_AUTH_ERR (-4)
#define HAL_STATUS_CODE_ERR (-6)
#define HAL_STATUS_NOAUTH_ERR (-10)
#define HAL_STATUS_BITCOUNT_ERR (-11)
#define HAL_STATUS_WRITE_ERR (-15)
#define HAL_STATUS_FRAMINGERR (-21)
#define HAL_STATUS_UNKNOWN_CMD (-23)
#define HAL_STATUS_COLLERR (-24)
#define HAL_STATUS_ACCESSTIMEOUT (-27)
#define HAL_STATUS_INTEGRITY_ERR (-35)
#define HAL_STATUS_PARAM_VAL_ERR (-60)
#define HAL_STATUS_WRITE_REG_ERR (-61)
#define HAL_STATUS_ERR (-125)
#define HAL_STATUS_PROTOCOL_ERR (-126)
#define HAL_STATUS_USER_ERR (-127)

/* =========================
 * NFC 芯片寄存器地址
 * ========================= */
/* PAGE 0 */
#define REG_COMMAND 0x01
#define REG_COMIEN 0x02
#define REG_DIVIEN 0x03
#define REG_COMIRQ 0x04
#define REG_DIVIRQ 0x05
#define REG_ERROR 0x06
#define REG_STATUS1 0x07
#define REG_STATUS2 0x08
#define REG_FIFODATA 0x09
#define REG_FIFOLEVEL 0x0A
#define REG_WATERLEVEL 0x0B
#define REG_CONTROL 0x0C
#define REG_BITFRAMING 0x0D
#define REG_COLL 0x0E

/* PAGE 1 */
#define REG_MODE 0x11
#define REG_TXMODE 0x12
#define REG_RXMODE 0x13
#define REG_TXCONTROL 0x14
#define REG_TXASK 0x15
#define REG_TXAUTO 0x15
#define REG_TXSEL 0x16
#define REG_RXSEL 0x17
#define REG_RXTHRESHOLD 0x18
#define REG_DEMOD 0x19
#define REG_MFTX 0x1C
#define REG_MFRX 0x1D
#define REG_TYPEB 0x1E
#define REG_SERIALSPEED 0x1F

/* PAGE 2 */
#define REG_CRCRESULTM 0x21
#define REG_CRCRESULTL 0x22
#define REG_MODWIDTH 0x24
#define REG_RFCFG 0x26
#define REG_GSN 0x27
#define REG_CWGSP 0x28
#define REG_MODGSP 0x29
#define REG_TMODE 0x2A
#define REG_TPRESCALER 0x2B
#define REG_TRELOADH 0x2C
#define REG_TRELOADL 0x2D
#define REG_TCOUNTERVALUEH 0x2E
#define REG_TCOUNTERVALUEL 0x2F

/* PAGE 3 */
#define REG_TESTSEL1 0x31
#define REG_TESTSEL2 0x32
#define REG_TESTPINEN 0x33
#define REG_TESTPINVALUE 0x34
#define REG_TESTBUS 0x35
#define REG_AUTOTEST 0x36
#define REG_VERSION 0x37
#define REG_ANALOGTEST 0x38
#define REG_TESTDAC1 0x39
#define REG_TESTDAC2 0x3A
#define REG_TESTADC 0x3B
#define REG_SPECIAL 0x3F

#define REG_PAGESEL 0x37
#define GSPOUT 0x01

/* =========================
 * ISO14443 块格式掩码
 * ========================= */
#define PCD_CMD_I_BLOCK_Msk 0xe2
#define PCD_CMD_I_BLOCK 0x02

#define PCD_CMD_R_BLOCK_Msk 0xe6
#define PCD_CMD_R_BLOCK 0xa2
#define PCD_CMD_R_CID_Msk 0x08

#define PCD_CMD_S_BLOCK_Msk 0xc7
#define PCD_CMD_S_BLOCK 0xc2

/* =========================
 * 通用位宏
 * ========================= */
#define BIT0 (0x00000001U)
#define BIT1 (0x00000002U)
#define BIT2 (0x00000004U)
#define BIT3 (0x00000008U)
#define BIT4 (0x00000010U)
#define BIT5 (0x00000020U)
#define BIT6 (0x00000040U)
#define BIT7 (0x00000080U)
#define BIT8 (0x00000100U)
#define BIT9 (0x00000200U)
#define BIT10 (0x00000400U)
#define BIT11 (0x00000800U)
#define BIT12 (0x00001000U)
#define BIT13 (0x00002000U)
#define BIT14 (0x00004000U)
#define BIT15 (0x00008000U)
#define BIT16 (0x00010000U)
#define BIT17 (0x00020000U)
#define BIT18 (0x00040000U)
#define BIT19 (0x00080000U)
#define BIT20 (0x00100000U)
#define BIT21 (0x00200000U)
#define BIT22 (0x00400000U)
#define BIT23 (0x00800000U)
#define BIT24 (0x01000000U)
#define BIT25 (0x02000000U)
#define BIT26 (0x04000000U)
#define BIT27 (0x08000000U)
#define BIT28 (0x10000000U)
#define BIT29 (0x20000000U)
#define BIT30 (0x40000000U)
#define BIT31 (0x80000000U)

#define BIT(n) (1UL << (n))

/* =========================
 * 语音分级与基础阈值
 * ========================= */
#define LEVEL_1 20
#define LEVEL_2 100
#define LEVEL_3 250

#define TH_MIN_ABS 3.0f

/* =========================
 * SC7A20 基础参数
 * ========================= */
#define SC7A20_INIT_MAX 100

#define GSENSOR_INT_PROFILE_VERY_SLOW 0
#define GSENSOR_INT_PROFILE_SLOW 1
#define GSENSOR_INT_PROFILE_BALANCED 2
#define GSENSOR_INT_PROFILE_SENSITIVE 3

#ifndef GSENSOR_INT_PROFILE
#define GSENSOR_INT_PROFILE GSENSOR_INT_PROFILE_BALANCED
#endif

#define GSENSOR_INT_CTRL_REG4 0x88
#define GSENSOR_INT_CTRL_REG2 0x31
#define GSENSOR_INT_CTRL_REG3 0x40
#define GSENSOR_INT_CTRL_REG6 0x00
#define GSENSOR_INT_CTRL_REG5 0x00
#define GSENSOR_INT_AOI1_CFG 0x2A

#if (GSENSOR_INT_PROFILE == GSENSOR_INT_PROFILE_VERY_SLOW)
#define GSENSOR_INT_CTRL_REG1 0x57
#define GSENSOR_INT_AOI1_THS 0x1A
#define GSENSOR_INT_AOI1_DUR 0x02
#elif (GSENSOR_INT_PROFILE == GSENSOR_INT_PROFILE_SLOW)
#define GSENSOR_INT_CTRL_REG1 0x57
#define GSENSOR_INT_AOI1_THS 0x12
#define GSENSOR_INT_AOI1_DUR 0x01
#elif (GSENSOR_INT_PROFILE == GSENSOR_INT_PROFILE_BALANCED)
#define GSENSOR_INT_CTRL_REG1 0x57
#define GSENSOR_INT_AOI1_THS 0x10
#define GSENSOR_INT_AOI1_DUR 0x01
#elif (GSENSOR_INT_PROFILE == GSENSOR_INT_PROFILE_SENSITIVE)
#define GSENSOR_INT_CTRL_REG1 0x57
#define GSENSOR_INT_AOI1_THS 0x0C
#define GSENSOR_INT_AOI1_DUR 0x00
#else
#error "Invalid GSENSOR_INT_PROFILE"
#endif

/* =========================
 * GSENSOR 软件判定参数
 * ========================= */
#define GSENSOR_INT_EVENT_COOLDOWN_MS 600U
#define GSENSOR_INT_PLAY_COOLDOWN_MS 900U
#define GSENSOR_CONFIRM_WINDOW_MS 120U
#define GSENSOR_FEATURE_PEAK_TH 60
#define GSENSOR_FEATURE_ENERGY_TH 250
#define GSENSOR_FEATURE_REBOUND_TH 25
#define GSENSOR_FEATURE_ACTIVE_TH 20
#define GSENSOR_FEATURE_ACTIVE_COUNT 3

#define GSENSOR_AXIS_MODE_WEIGHTED 0
#define GSENSOR_AXIS_MODE_Z_ONLY 1
#define GSENSOR_AXIS_MODE GSENSOR_AXIS_MODE_WEIGHTED
#define GSENSOR_AXIS_WEIGHT_X 15U
#define GSENSOR_AXIS_WEIGHT_Y 15U
#define GSENSOR_AXIS_WEIGHT_Z 70U

#define GSENSOR_UART_STREAM_ENABLE 0
#define GSENSOR_UART_STREAM_MS 300U
#define GSENSOR_EVENT_DEBUG_PRINTF 0

#define GSENSOR_UART_HEADER_0 0xA5
#define GSENSOR_UART_HEADER_1 0x5A
#define GSENSOR_UART_TYPE_RAW 0x01
#define GSENSOR_UART_TYPE_EVENT 0x02

#define GSENSOR_FAIL_PEAK BIT0
#define GSENSOR_FAIL_ENERGY BIT1
#define GSENSOR_FAIL_ACTIVE BIT2
#define GSENSOR_FAIL_REBOUND BIT3
#define GSENSOR_FAIL_COOLDOWN BIT4

#define G_THRESHOLD 250.0f

/* =========================
 * 通用类型别名
 * ========================= */
#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t

#define UID_4 4
#define UID_7 7
#define UID_10 10

#define PICC_CID 0x00

#define SEND_DATA_NUM_MAX 100
#define MAX_TRX_BUF_SIZE 64

#define FSDI 8
#define FIFO_SIZE 64
#define COLLPOS 0x1F

#define TP_FWT_302us 2048
#define TP_dFWT 192

#endif
