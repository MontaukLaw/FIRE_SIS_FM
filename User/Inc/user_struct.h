
#ifndef _USER_STRUCT_H_
#define _USER_STRUCT_H_

typedef enum
{
    ST_IDLE = 0,
    ST_CONTACT_PENDING,
    ST_TRACKING,
    ST_ENDING
} TouchState;

typedef uint8_t wave_state_t;
typedef void (*process_handle)(uint32_t event);

enum
{
    WAVE_IDLE = 0,
    WAVE_ACTIVE,
    WAVE_REFRACTORY
};

typedef enum
{
    PICC_TimeIEn = 0x01,
    PICC_ErrIEn = 0x02,
    PICC_LoAlertIEn = 0x04,
    PICC_HiAlertIEn = 0x08,
    PICC_IdleIEn = 0x10,
    PICC_RxIEn = 0x20,
    PICC_TxIEn = 0x40,
    PICC_IRQInv = 0x80,
} ComIEnReg;

typedef struct tag_info
{
    u8 uncoded_key_is_a : 1;
    u8 opt_step;
    u8 uid_length;
    u8 tag_type;
    u8 tag_type_bytes[2];
    u8 serial_num[8];
    u8 uncoded_key[6];
    struct block
    {
        u8 num;
        u8 block_data[16];
    } block;
} tag_info;

typedef enum
{
    PICC_STATE_INIT = 0,
    PICC_STATE_STOP,
    PICC_STATE_START,
    PICC_STATE_WAIT,
    PICC_STATE_IDLE,
    PICC_STATE_HALT,
    PICC_STATE_ACTIVE,
    PICC_STATE_T2T,
    PICC_STATE_RATS,
    PICC_STATE_PPS,
    PICC_STATE_DESELECT,
    PICC_STATE_RECV_BLOCK,
    PICC_STATE_SEND,
    PICC_STATE_SEND_HALF_BYTE,

} picc_state_e;

typedef enum
{
    PICC_TimeIrq = 0x01,
    PICC_ErrIrq = 0x02,
    PICC_LoAlertIrq = 0x04,
    PICC_HiAlertIrq = 0x08,
    PICC_IdleIrq = 0x10,
    PICC_RxIrq = 0x20,
    PICC_TxIrq = 0x40,
    PICC_Set1 = 0x80,
} ComIrqReg;

enum
{
    PICC_MODE_DISABLE = 0,
    PICC_MODE_ENABLE,
};

typedef enum
{
    HAL_TECH_TYPE_A,
    HAL_TECH_TYPE_B,
    HAL_TECH_TYPE_F,
    HAL_TECH_TYPE_V,
} hal_nfc_tech_type_t;

enum
{
    STATE_READ_CARD = 0,
    STATE_READER_EXCHANGTE_DATA,
    STATE_PICC_INIT,
    STATE_PICC,
    STATE_CLEAR,
    STATE_MAX_TOP,
};

typedef enum
{
    TIMER_EVENT = 0,
    ERR_EVENT,
    LOALERT_EVENT,
    HIALERT_EVENT,
    IDLE_EVENT = 4,
    RX_EVENT,
    TX_EVENT,
    PICC_ACTIVE_EVENT,
    PICC_INACTIVE_EVENT = 8,
    PICC_FIELD_IN_EVENT,
    PICC_FIELD_OUT_EVENT,
    EVENT_NUM,
} intc_event_e;

typedef uint32_t tick;

struct picc_rats
{
    uint8_t cid;
    uint16_t fsdi;
};

struct picc_run_t
{
    uint8_t en;
    picc_state_e state;
    uint8_t rx_buf[64];
    uint16_t rx_len;
    uint8_t tx_buf[64];
    uint16_t tx_len;
    uint8_t rate;
    tick time;
    struct picc_rats rats;
    uint8_t local_blk_num;
};

struct g_baseline_tracking_t
{
    uint8_t initialized;
    uint8_t thresholds_ready;
    // 通道缓冲
    float base_line[MAX_SENSORS];
    float base_noise[MAX_SENSORS];

    float diff_f[MAX_SENSORS];
    float diff_norm_f[MAX_SENSORS]; // EMA 后的归一化值

    float val_over_threshold[MAX_SENSORS]; // 阈后强度（≥0）
    bool over_th[MAX_SENSORS];

    // 强度与质心
    float omiga_sig; // 幅度制总强度  Σ(sig)
    float sum_idx;   // 幅度制加权和  Σ(sig*i)
    float avg_th;    // 平均单通道门限
    uint16_t C_act;  // 活跃通道数

    float s_on, s_hold, s_off;                   // 幅度制
    uint32_t s_on_norm, s_hold_norm, s_off_norm; // 归一制
    uint16_t c_on, c_hold, c_off;

    uint32_t quiet_up_cnt[MAX_SENSORS];
    uint32_t over_th_cnt[MAX_SENSORS];
};

typedef struct
{
    wave_state_t state;

    // uint16_t th_on;
    // uint16_t th_off;

    uint8_t on_count;
    uint8_t off_count;

    uint8_t on_count_limit;  // 进入触发需连续点数
    uint8_t off_count_limit; // 退出触发需连续点数

    uint16_t width;
    uint16_t peak;
    uint32_t area;
    uint16_t activate_value; // 进入时的幅度值, 用于判断是否过小

    uint16_t min_width;
    uint16_t max_width;
    uint16_t min_peak;

    uint8_t refractory_cnt;
    uint8_t refractory_len;

    // 输出
    uint8_t event_flag; // 检测到一个完整波
    uint16_t event_peak;
    uint16_t event_width;
    uint32_t event_area;

} wave_detector_t;

typedef void (*picc_event)(uint32_t);

struct picc_ats
{
    uint8_t TL;
    union
    {
        uint8_t val;
        struct
        {
            uint8_t FSCI : 4;
            uint8_t Y1 : 3;
            uint8_t res : 1;
        } bitmap;
    } T0;
    union
    {
        uint8_t val;
        struct
        {
            uint8_t DR : 3;
            uint8_t res : 1;
            uint8_t DS : 3;
            uint8_t same : 1;
        } bitmap;
    } TA;
    union
    {
        uint8_t val;
        struct
        {
            uint8_t SFGI : 4;
            uint8_t FWI : 4;
        } bitmap;
    } TB;
    union
    {
        uint8_t val;
        struct
        {
            uint8_t NAD_EN : 1;
            uint8_t CID_EN : 1;
            uint8_t res : 6;
        } bitmap;
    } TC;
};

typedef struct
{
    uint8_t page;
    uint8_t addr;
    uint8_t value;
} hal_nfc_regval_t;

struct picc_config_t
{
    uint8_t uid[7];
    uint8_t coll_level;
    uint8_t enable_14443_4;
    uint16_t atqa;
    struct picc_ats ats;
    void (*app)(void);
    picc_event event[EVENT_NUM];
};

typedef struct
{
    union
    {
        uint8_t val;
        struct
        {
            uint8_t blk_num : 1;
            uint8_t fixed1 : 1;
            uint8_t nad_flw : 1;
            uint8_t cid_flw : 1;
            uint8_t chain_flg : 1;
            uint8_t fixed0 : 1;
            uint8_t blk_t : 2;
        } i_pcb;
        struct
        {
            uint8_t blk_num : 1;
            uint8_t fixed1_ : 1;
            uint8_t fixed0 : 1;
            uint8_t cid_flw : 1;
            uint8_t nak_flg : 1;
            uint8_t fixed1 : 1;
            uint8_t blk_t : 2;
        } r_pcb;
        struct
        {
            uint8_t fixed0_ : 1;
            uint8_t fixed1 : 1;
            uint8_t fixed0 : 1;
            uint8_t cid_flw : 1;
            uint8_t func : 2; // 00DESELECT;11WTX;01PPS
            uint8_t blk_t : 2;
        } s_pcb;
    } PCB;
    uint8_t cid;
    uint8_t nad;

} block_prologue_field_t;

typedef struct
{
    uint8_t valid; // 0:no recv 1:recv and ok  2:recv but err
    block_prologue_field_t prologue;
    uint8_t *data;
    uint8_t data_len;

} block_preprocess_t;

typedef struct
{
    uint16_t fid;         // 文件ID（如0xE103为CC文件）
    uint8_t type;         // 文件类型：1=目录，2=数据文件
    uint32_t size;        // 文件大小
    uint16_t start_block; // 起始存储块号
    uint8_t access_mode;  // 访问权限（0x01=读，0x02=写）
} file_entry;

typedef struct
{
    uint8_t file_count;
    file_entry *cur_file;
    file_entry files[4];    // 文件系统（包含CC文件+NDEF文件+预留）
    uint8_t ndef_data[64]; // NDEF数据存储区（含网页URI）
} t4t_entity;

#pragma pack(push, 1)
typedef struct
{
    uint16_t ndef_len;
    uint8_t header;     // 0xD1（MB=1, ME=1, TNF=0x01）
    uint8_t type_len;   // 类型长度=1
    uint8_t uri_len;    // 载荷长度
    uint8_t type;       // 'U'（URI类型）
    uint8_t uri_prefix; // URI前缀（0x01=http://www.）
    char uri[64];      // 网页地址（最大64字节）
} ndef_uri_record;
#pragma pack(pop)

typedef struct block_info_field
{
    uint8_t cla;
    uint8_t ins;
    uint8_t p1;
    uint8_t p2;
    // Lc类型：0-无Lc，1-短Lc（1字节），2-扩展Lc（3字节）
    uint8_t lc_type;
    union
    {
        uint8_t short_lc; // 短Lc（1字节，范围1-255）
        uint16_t ext_lc;  // 扩展Lc（3字节，格式0x00+2字节长度，范围0-65535）
    } lc;

    const uint8_t *data; // 数据域指针

    // Le类型：0-无Le，1-短Le（1字节），2-短Lc扩展Le（3字节），3-扩展Lc扩展Le（2字节）
    uint8_t le_type;
    union
    {
        uint8_t short_le; // 短Le（1字节，0表示256）
        uint16_t ext_le;  // 扩展Le（3字节，格式0x00+2字节长度）
    } le;

} c_apdu_t;

typedef struct
{
    uint8_t *data; // 响应数据
    uint16_t len;  // 实际数据长度
    uint16_t sw;   // 状态码
} r_apdu_t;

typedef struct pps
{
    union
    {
        uint8_t val;
        struct
        {
            uint8_t cid : 4;
            uint8_t cmd : 4;
        } bitmap;
    } PPSS;
    uint8_t pps0;
    uint8_t pps1;
} pps_t;

typedef struct
{
    unsigned char uc_nid;
    unsigned char uc_nad_en;
    unsigned char uc_cid;
    unsigned char uc_cid_en;
    unsigned int ui_fsc;
    unsigned int ui_fwi;
    unsigned int ui_sfgi;
    unsigned char uc_pcd_pcb;
    unsigned char uc_picc_pcb;
    unsigned int uc_pcd_txr_num;
    unsigned int uc_pcd_txr_lastbits;
    unsigned char uc_wtxm;
} pcd_info_s;

typedef struct
{
    int8_t mf_command;
    uint16_t mf_length;
    uint8_t mf_data[MAX_TRX_BUF_SIZE];
    uint8_t type;
    uint8_t scene;
} hal_nfc_transceive_t;

#endif // _USER_STRUCT_H_
