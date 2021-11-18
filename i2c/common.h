#ifndef I2C_COMMON_H
#define I2C_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#define WINDOW_SIZE 5
#define MAX_NAME_LEN 16

#include "../spi/common.h"

struct identifier {
  /*! Demux channel */
  uint8_t mux_id;

  /*! File descriptor */
  uint8_t fd;
};

int8_t i2c_read(uint8_t reg_addr, uint8_t* reg_data, uint32_t length, void* intf_ptr);
int8_t i2c_write(uint8_t reg_addr, const uint8_t* reg_data, uint32_t length, void* intf_ptr);
int8_t i2c_open(int8_t* fd, uint8_t addr);

void direct_mux(uint8_t id);
int8_t configure_mux();
void delay_us(uint32_t period, void* intf_ptr);

#ifdef __cplusplus
}
#endif

#endif