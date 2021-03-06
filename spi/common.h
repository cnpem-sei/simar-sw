/*! @file common.h
 * @brief Common declarations for SPI operations
 */

/*!
 * @defgroup spi SPI
 * @brief SPI and GPIO communication (including module selection)
 */

#ifndef SPIUTIL_H
#define SPIUTIL_H

#include <stdint.h>

#define GPIO_LENGTH 4096
#define GPIO0_ADDR 0x44E07000
#define GPIO1_ADDR 0x4804C000
#define GPIO2_ADDR 0x481AC000
#define GPIO3_ADDR 0x481AF000

#define MMIO_SUCCESS 0
#define MMIO_ERROR_ARGUMENT -1
#define MMIO_ERROR_DEVMEM -2
#define MMIO_ERROR_MMAP -3

#define MMIO_OE_ADDR 0x134
#define MMIO_GPIO_DATAOUT 0x13C
#define MMIO_GPIO_DATAIN 0x138
#define MMIO_GPIO_CLEARDATAOUT 0x190
#define MMIO_GPIO_SETDATAOUT 0x194

#define SENSOR_FAIL -2
#define DB_FAIL -3
#define BUS_FAIL -9

/// Convenience enum for translating common pin names to their respective integer values
enum pins {
  USR_0 = 53,  /// User LED 0
  USR_1 = 54,  /// User LED 1
  USR_2 = 55,  /// User LED 2
  USR_3 = 56,  /// User LED 3
  P9_11 = 30,
  P9_12 = 60,
  P9_13 = 31,
  P9_14 = 50,
  P9_15 = 48,
  P9_16 = 51,
  P9_17 = 5,
  P9_18 = 4,
  P9_19 = 13,
  P9_20 = 12,
  P9_21 = 3,
  P9_22 = 2,
  P9_23 = 49,
  P9_24 = 15,
  P9_25 = 117,
  P9_26 = 14,
  P9_27 = 115,
  P9_28 = 113,
  P9_29 = 111,
  P9_30 = 112,
  P9_31 = 110,
  P9_41 = 20,
  P9_42 = 7,
  P8_3 = 38,
  P8_4 = 39,
  P8_5 = 34,
  P8_6 = 35,
  P8_7 = 66,
  P8_8 = 67,
  P8_9 = 69,
  P8_10 = 68,
  P8_11 = 45,
  P8_12 = 44,
  P8_13 = 23,
  P8_14 = 26,
  P8_15 = 47,
  P8_16 = 46,
  P8_17 = 27,
  P8_18 = 65,
  P8_19 = 22,
  P8_20 = 63,
  P8_21 = 62,
  P8_22 = 37,
  P8_23 = 36,
  P8_24 = 33,
  P8_25 = 32,
  P8_26 = 61,
  P8_27 = 86,
  P8_28 = 88,
  P8_29 = 87,
  P8_30 = 89,
  P8_31 = 10,
  P8_32 = 11,
  P8_33 = 9,
  P8_34 = 81,
  P8_35 = 8,
  P8_36 = 80,
  P8_37 = 78,
  P8_38 = 79,
  P8_39 = 76,
  P8_40 = 77,
  P8_41 = 74,
  P8_42 = 75,
  P8_43 = 72,
  P8_44 = 73,
  P8_45 = 70,
  P8_46 = 71
};

/*!
 * @brief Beaglebone GPIO pin structure
 */
typedef struct {
  int pin;
  volatile uint32_t* base;
  int number;
} gpio_t;

/**
 * \ingroup spi
 * \defgroup spiComm Communication
 * @brief Communication methods
 */

/**
 * \ingroup spiComm
 * @brief Opens SPI bus
 * @param[in] device Device location (Ex.: /dev/spidev0.0)
 * @param[in] mode SPI mode
 * @param[in] bits Bits per word
 * @param[in] speed Speed (in Hz)
 * @param[in] cs CS Pin
 * @returns SPI bus open operation result
 * @retval 0 Success
 * @retval -1 Failure
 */
int spi_open(const char* device, uint32_t* mode, uint8_t* bits, uint32_t* speed);

/**
 * \ingroup spiComm
 * @brief Closes SPI bus
 * @returns SPI bus close operation result
 * @retval 0 Success
 * @retval -1 Failure
 */
int spi_close();

/**
 * \ingroup spiComm
 * @brief Transfers buffer through SPI with determined length
 * @param[in] tx TX buffer
 * @param[out] rx RX buffer
 * @param[in] len Buffer length
 * @returns SPI transfer operation result
 * @retval 0 Success
 * @retval -1 Failure
 */
int spi_transfer(const char* tx, const char* rx, int len);

void mmio_set_output(gpio_t gpio);
void mmio_set_input(gpio_t gpio);
void mmio_set_high(gpio_t gpio);
void mmio_set_low(gpio_t gpio);
uint32_t mmio_input(gpio_t gpio);
int mmio_get_gpio(gpio_t* gpio);

/**
 * \ingroup spi
 * \defgroup spiModule Module
 * @brief SPI module settings and communication methods
 */

/**
 * @brief Selects module at given address
 * @param[in] address Address
 * @param[in] module Module value
 * @returns SPI transfer operation result
 * @retval 0 Success
 * @retval 1 Failure
 */
int select_module(int address, int module);

/**
 * \ingroup spiModule
 * @brief Writes data directly to SPI (module selection)
 * @param[in] address Data
 * @param[in] len Length of data
 * @returns SPI transfer operation result
 * @retval 0 Success
 * @retval 1 Failure
 */
int transfer_module(char* data, int len);

/**
 * \ingroup spiModule
 * @brief Writes digital data at given address
 * @param[in] address address
 * @param[in] data Data to write
 * @param[in] len Data buffer length
 * @returns SPI transfer operation result
 * @retval >=0 Success
 * @retval <0 Failure
 */
int write_data(int address, char* data, int len);

/**
 * \ingroup spiModule
 * @brief Reads digital data at given address
 * @param[in] address address
 * @param[out] rx Buffer to write data to
 * @returns SPI transfer operation result
 * @retval >=0 Success
 * @retval <0 Failure
 */
int read_data(int address, char* rx, int len);

/**
 * \ingroup spiModule
 * @brief Writes directly to module selector (bypassing parity)
 * @param[in] tx Information to transmit
 * @param[out] rx Buffer to write data to
 * @param[in] rx Length of message to transmit
 * @returns SPI transfer operation result
 * @retval >=0 Success
 * @retval <0 Failure
 */
int spi_mod_comm(char* tx, char* rx, int len);

#endif
