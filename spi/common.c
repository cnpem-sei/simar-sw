/*! @file common.c
 * @brief Common functions for SPI operations
 */

#include "common.h"

#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

static uint32_t gpio_addresses[4] = {GPIO0_ADDR, GPIO1_ADDR, GPIO2_ADDR, GPIO3_ADDR};
static volatile uint32_t* gpio_base[4] = {NULL};

int _bits, _speed, _delay, _mode, fd;
int mod_bits = 8;
int mod_mode = SPI_MODE_3;

gpio_t cs_pin = {.pin = P9_17};
gpio_t ds_pin = {.pin = P9_14};

void mmio_set_output(gpio_t gpio) {
  gpio.base[MMIO_OE_ADDR / 4] &= (0xFFFFFFFF ^ (1 << gpio.number));
}

void mmio_set_input(gpio_t gpio) {
  gpio.base[MMIO_OE_ADDR / 4] |= (1 << gpio.number);
}

void mmio_set_high(gpio_t gpio) {
  gpio.base[MMIO_GPIO_SETDATAOUT / 4] = 1 << gpio.number;
}

void mmio_set_low(gpio_t gpio) {
  gpio.base[MMIO_GPIO_CLEARDATAOUT / 4] = 1 << gpio.number;
}

uint32_t mmio_input(gpio_t gpio) {
  return gpio.base[MMIO_GPIO_DATAIN / 4] & (1 << gpio.number);
}

int mmio_get_gpio(gpio_t* gpio) {
  // Validate input parameters.
  int base = gpio->pin / 32;
  int number = gpio->pin % 32;

  if (gpio == NULL)
    return MMIO_ERROR_ARGUMENT;
  if (base < 0 || base > 3)
    return MMIO_ERROR_ARGUMENT;
  if (number < 0 || number > 31)
    return MMIO_ERROR_ARGUMENT;
  if (gpio_base[base] == NULL) {
    int mfd = open("/dev/mem", O_RDWR | O_SYNC);
    if (mfd == -1)
      return MMIO_ERROR_DEVMEM;

    gpio_base[base] = (uint32_t*)mmap(NULL, GPIO_LENGTH, PROT_READ | PROT_WRITE, MAP_SHARED, mfd,
                                      gpio_addresses[base]);
    if (gpio_base[base] == MAP_FAILED) {
      gpio_base[base] = NULL;
      return MMIO_ERROR_MMAP;
    }
  }

  memset(gpio, 0, sizeof(*gpio));
  gpio->base = gpio_base[base];
  gpio->number = number;
  return MMIO_SUCCESS;
}

int spi_open(const char* device, uint32_t* mode, uint8_t* bits, uint32_t* speed) {
  fd = open(device, O_RDWR);

  ioctl(fd, SPI_IOC_WR_MODE, mode);
  ioctl(fd, SPI_IOC_RD_MODE, mode);

  ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, bits);
  ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, bits);

  ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, speed);
  ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, speed);

  _bits = *bits;
  _speed = *speed;
  _mode = *mode;

  mmio_get_gpio(&cs_pin);
  mmio_get_gpio(&ds_pin);

  mmio_set_output(cs_pin);
  mmio_set_output(ds_pin);

  return fd;
}

int spi_close() {
  return close(fd);
}

int spi_transfer(const char* tx, const char* rx, int len) {
  int ret;
  struct spi_ioc_transfer tr = {
      .tx_buf = (unsigned long)tx,
      .rx_buf = (unsigned long)rx,
      .len = len,
      .delay_usecs = _delay,
      .speed_hz = _speed,
      .bits_per_word = _bits,
  };

  ret = ioctl(fd, SPI_IOC_MESSAGE(len), &tr);

  return ret < 0 ? -1 : 0;
}

/**
 * @brief Calculates even parity bit
 * @param[in] Value to calculate parity for
 * @returns Parity bit
 */
int calculate_parity(int x) {
  int y = x ^ (x >> 1);
  y ^= y >> 2;
  y ^= y >> 4;
  y ^= y >> 8;
  y ^= y >> 16;

  return y & 1;
}

int spi_mod_comm(char* tx, char* rx, int len) {
  int ret;

  if (_mode != SPI_MODE_3)
    ioctl(fd, SPI_IOC_WR_MODE, &mod_mode);
  if (_bits != 8)
    ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &mod_bits);

  // Reusing device file descriptor, but creating custom struct to prevent
  // confusion
  struct spi_ioc_transfer tr = {
      .tx_buf = (unsigned long)tx,
      .rx_buf = (unsigned long)rx,
      .len = len,
      .delay_usecs = 0,
      .speed_hz = _speed,
      .bits_per_word = mod_bits,
  };

  mmio_set_low(ds_pin);
  ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
  mmio_set_high(ds_pin);

  if (_mode != SPI_MODE_3)
    ioctl(fd, SPI_IOC_WR_MODE, &_mode);
  if (_bits != 8)
    ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &_bits);

  return ret < 0 ? -1 : 0;
}

int select_module(int address, int module) {
  unsigned long msg;
  int parity = calculate_parity(address);

  msg = (parity << 4) | address;
  msg = (msg << 3) | module;

  char msg_c[1] = {msg};

  return spi_mod_comm(msg_c, msg_c, 1);
}

int transfer_module(char* data, int len) {
  return spi_mod_comm(data, data, len);
}

int write_data(int address, char* data, int len) {
  int ret;

  select_module(address, 1);

  if (_mode != SPI_MODE_3)
    ioctl(fd, SPI_IOC_WR_MODE, &mod_mode);
  if (_bits != 8)
    ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &mod_bits);

  mmio_set_high(cs_pin);
  mmio_set_low(cs_pin);

  mmio_set_high(cs_pin);
  ret = write(fd, data, len);
  mmio_set_low(cs_pin);

  if (_mode != SPI_MODE_3)
    ioctl(fd, SPI_IOC_WR_MODE, &_mode);
  if (_bits != 8)
    ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &_bits);

  return ret;
}

int read_data(int address, char* rx, int len) {
  int ret;
  char dummy_data[1] = "";

  select_module(address, 2);
  spi_transfer(dummy_data, dummy_data, 1);
  select_module(address, 3);
  spi_transfer(dummy_data, dummy_data, 1);

  if (_mode != SPI_MODE_3)
    ioctl(fd, SPI_IOC_WR_MODE, &mod_mode);
  if (_bits != 8)
    ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &mod_bits);

  ret = read(fd, rx, len);

  if (_mode != SPI_MODE_3)
    ioctl(fd, SPI_IOC_WR_MODE, &_mode);
  if (_bits != 8)
    ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &_bits);

  return ret;
}
