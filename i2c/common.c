/*! @file common.c
 * @brief Common functions for I2C operations
 */

#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include "common.h"

gpio_t mux0 = {.pin = P9_15};  // LSB
gpio_t mux1 = {.pin = P9_16};  // MSB

int8_t pins_configured = 0;

uint8_t ext_addr = -1;

/**
 * @brief Selects an available I2C channel through the digital interface board (0 to 4)
 * @param[in] id Desired channel ID
 * @return void
 */
void direct_mux(uint8_t id) {
  if ((id >> 0) & 1)
    bbb_mmio_set_high(mux0);
  else
    bbb_mmio_set_low(mux0);

  if ((id >> 1) & 1)
    bbb_mmio_set_high(mux1);
  else
    bbb_mmio_set_low(mux1);
}

/**
 * @brief Selects an available I2C channel through the SPI and I2C extender boards (0 to 8)
 * @param[in] id Desired channel ID
 * @param[in] addr Designed extender board address
 * @return void
 */
void direct_ext_mux(uint8_t id) {
  char* rx;
  rx = malloc(1 * sizeof(char));

  char ext_mux_id[1] = {id};

  select_module(ext_addr, 2);
  spi_transfer(ext_mux_id, rx, 1);

  free(rx);
}

/**
 * @brief Sets the SPI extender board address
 * @param[in] addr Board address
 * @retval 0 OK
 * @retval -1 Invalid board address
 */
int8_t set_ext_addr(uint8_t addr) {
  if (addr > 15 || addr == 0)
    return -1;
  ext_addr = addr;
  return 0;
}

/*!
 *  @brief Function for reading the sensor's registers through I2C bus.
 *
 *  @param[in] reg_addr       : Register address.
 *  @param[out] data          : Pointer to the data buffer to store the read
 * data.
 *  @param[in] len            : No of bytes to read.
 *  @param[in, out] intf_ptr  : Void pointer that can enable the linking of
 * descriptors for interface related call backs.
 *
 *  @return Status of execution
 *
 *  @retval 0 -> Success
 *  @retval -1 -> Failure
 *
 */
int8_t i2c_read(uint8_t reg_addr, uint8_t* reg_data, uint32_t length, void* intf_ptr) {
  struct identifier id;
  id = *((struct identifier*)intf_ptr);

  if (reg_addr != 0)
    write(id.fd, &reg_addr, 1);

  return read(id.fd, reg_data, length) < 0 ? -1 : 0;
}

/*!
 *  @brief Function for writing the sensor's registers through I2C bus.
 *
 *  @param[in] reg_addr       : Register address.
 *  @param[in] data           : Pointer to the data buffer whose value is to be
 * written.
 *  @param[in] len            : No of bytes to write.
 *  @param[in, out] intf_ptr  : Void pointer that can enable the linking of
 * descriptors for interface related call backs
 *
 *  @return Status of execution
 *
 *  @retval BME280_OK -> Success
 *  @retval BME280_E_COMM_FAIL -> Communication failure.
 *
 */
int8_t i2c_write(uint8_t reg_addr, const uint8_t* reg_data, uint32_t length, void* intf_ptr) {
  uint8_t* buf;

  // Sensirion adds CRC using their own methods, so we'll assume the address is already added if it
  // is null
  uint8_t address_offset = reg_addr != 0 ? 1 : 0;

  struct identifier id;
  id = *((struct identifier*)intf_ptr);

  buf = malloc(length + address_offset);

  if (address_offset)
    buf[0] = reg_addr;

  memcpy(buf + address_offset, reg_data, length);

  if (write(id.fd, buf, length + 1) < (uint16_t)length)
    return -2;

  free(buf);

  return 0;
}

/**
 * @brief Unselects the I2C extender (and SPI extender, by proxy)
 * @return void
 */
void unselect_i2c_extender() {
  char* rx;
  rx = malloc(1 * sizeof(char));

  spi_mod_comm("\x00", rx, 1);

  free(rx);
}

/**
 * @brief Configures pins for the digital interface board multiplexing function
 * @return void
 */
int8_t configure_mux() {
  int8_t rslt = 0;

  if (!pins_configured) {
    rslt |= bbb_mmio_get_gpio(&mux0);
    bbb_mmio_set_output(mux0);
    rslt |= bbb_mmio_get_gpio(&mux1);
    bbb_mmio_set_output(mux0);

    pins_configured = !rslt;
  }

  return rslt;
}

/**
 * @brief Delays execution for n us
 * @param[in] period Microsseconds to stop for
 */
void delay_us(uint32_t period, void* intf_ptr) {
  nanosleep((const struct timespec[]){{0, period * 1000}}, NULL);
}

int8_t i2c_open(int8_t* fd, uint8_t addr) {
  if (!*fd) {
    *fd = open("/dev/i2c-2", O_RDWR);
    if (*fd < 0)
      return -2;
    if (ioctl(*fd, I2C_SLAVE, addr) < 0)
      return -2;
  }
  return 0;
}
