#ifndef PMW3360_H
#define PMW3360_H

#include "driver/spi_master.h"
#include "driver/gpio.h"

// SPI Pins — change to match your wiring
#define PMW3360_MOSI    GPIO_NUM_23
#define PMW3360_MISO    GPIO_NUM_19
#define PMW3360_SCK     GPIO_NUM_18
#define PMW3360_CS      GPIO_NUM_15

// Registers
#define REG_MOTION          0x02
#define REG_DELTA_X_L       0x03
#define REG_DELTA_X_H       0x04
#define REG_DELTA_Y_L       0x05
#define REG_DELTA_Y_H       0x06
#define REG_POWER_UP_RESET  0x3A
#define REG_MOTION_BURST    0x50
#define REG_PRODUCT_ID      0x00
#define REG_CONFIG1         0x4F

typedef struct {
    int16_t delta_x;
    int16_t delta_y;
    uint8_t motion;
} pmw3360_motion_t;

extern spi_device_handle_t pmw3360_spi;

void    pmw3360_init();
void    pmw3360_write(uint8_t reg, uint8_t data);
uint8_t pmw3360_read(uint8_t reg);
void    pmw3360_read_motion(pmw3360_motion_t *motion);
void    pmw3360_set_cpi(uint16_t cpi);

#endif