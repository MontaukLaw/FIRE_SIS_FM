#ifndef _GPIO_I2C_H_
#define _GPIO_I2C_H_

#define I2C_SCL_PORT GPIOA
#define I2C_SCL_PIN GPIO_PIN_1

#define I2C_SDA_PORT GPIOA
#define I2C_SDA_PIN GPIO_PIN_0

// #define GPIO_SET(gpio, val)                                     \
//     do                                                          \
//     {                                                           \
//         uint8_t _p = ((gpio) >> 4);                             \
//         uint8_t _n = ((gpio) & 0x0F);                           \
//         *GPIO_DATA[_p] =                                        \
//             (*GPIO_DATA[_p] & ~(1U << _n)) | ((!!(val)) << _n); \
//     } while (0)

// #define SCL_H HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_SET); // GPIO_WriteHigh(SCL_PORT, SCL_PIN);
// #define SCL_L HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_RESET); // GPIO_WriteLow(SCL_PORT, SCL_PIN);

// #define SDA_IN gpio_config(I2C_SDA_PORT, I2C_SDA_PIN, INPUT, PULL_HIGH);
// #define SDA_OUT gpio_config(I2C_SDA_PORT, I2C_SDA_PIN, OUTPUT, PULL_NONE);

#define I2C_SDA_HIGH() HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, GPIO_PIN_SET);
#define I2C_SDA_LOW() HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, GPIO_PIN_RESET);

#define I2C_SCL_LOW() HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_RESET);
#define I2C_SCL_HIGH() HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_SET);

#define I2C_SDA_READ() (HAL_GPIO_ReadPin(I2C_SDA_PORT, I2C_SDA_PIN) == GPIO_PIN_SET)

void i2c_read_reg(uint8_t RAddr, uint8_t *RData, uint8_t slave_addr);

void i2c_read_bytes(uint8_t RAddr, uint8_t *RData, uint8_t RLen, uint8_t slave_addr);

void i2c_write_bytes(uint8_t RAddr, uint8_t *WData, uint8_t WLen, uint8_t slave_addr);

void i2c_write_reg(uint8_t RAddr, uint8_t WData, uint8_t slave_addr);

#endif
