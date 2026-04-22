#ifndef _USER_REFINE_H_
#define _USER_REFINE_H_

static inline void hal_nfc_clear_register_bit(uint32_t reg, uint32_t bit)
{
    nfc_clear_reg_bit(reg, bit);
}

static inline uint32_t get_tick(void)
{
    return HAL_GetTick();
}

static inline void hal_nfc_write_register(uint32_t reg, uint32_t value)
{
    nfc_write_reg(reg, value);
}

static inline void mdelay(uint32_t ms)
{
    HAL_Delay(ms);
}

static inline void hal_nfc_set_register_bit(uint32_t reg, uint32_t bit)
{
    nfc_set_reg_bit(reg, bit);
}

static inline void hal_nfc_bit_set(uint8_t page, uint8_t addr, uint8_t value, uint8_t page_sel)
{
    nfc_set_bit(page, addr, value, page_sel);
}

static inline void hal_nfc_bit_clear(uint8_t page, uint8_t addr, uint8_t value, uint8_t page_sel)
{
    nfc_clear_bit(page, addr, value, page_sel);
}

static inline void hal_nfc_set_register(uint8_t page, uint8_t addr, uint8_t value, uint8_t page_sel)
{
    nfc_set_reg(page, addr, value, page_sel);
}

static inline uint8_t hal_nfc_get_register(uint8_t page, uint8_t addr, uint8_t page_sel)
{
    return nfc_get_reg(page, addr, page_sel);
}

static inline void hal_nfc_read_register(uint8_t addr, uint8_t *value)
{
    nfc_read_reg(addr, value);
}

#define NFC_DEBUG_PRINTF 0

#if NFC_DEBUG_PRINTF
#define NFC_LOG(...) printf(__VA_ARGS__)
#else
#define NFC_LOG(...) do {} while (0)
#endif

// #define printf(...) do {} while (0)

#endif /* _USER_REFINE_H_ */
