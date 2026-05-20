#ifndef GPIOS_H
#define GPIOS_H

#include "driver/adc.h"
#include "driver/gpio.h"
#include "Parameters.h"

// ADC channels for right analog stick
#define ADC_STICK_RX        ADC1_CHANNEL_6  // GPIO 34
#define ADC_STICK_RY        ADC1_CHANNEL_7  // GPIO 35

// Button GPIO pins
#define GPIO_BTN_R3         GPIO_NUM_3      // Right stick click
#define GPIO_BTN_CIRCLE     GPIO_NUM_5      // A
#define GPIO_BTN_CROSS      GPIO_NUM_12     // B
#define GPIO_BTN_START      GPIO_NUM_13     // +
#define GPIO_BTN_SYNC       GPIO_NUM_15     // Sync
#define GPIO_BTN_HOME       GPIO_NUM_16     // Home
#define GPIO_BTN_SQUARE     GPIO_NUM_17     // Y
#define GPIO_BTN_TRIANGLE   GPIO_NUM_4      // X (Switch)
#define GPIO_BTN_L1         GPIO_NUM_25     // L
#define GPIO_BTN_L2         GPIO_NUM_26     // ZL

// LEDs
#define GPIO_LED_SYNC_BLE   GPIO_NUM_0
#define GPIO_LED_SYNC       GPIO_NUM_1

extern uint64_t PIN_BIT_MASK;

void setup_gpios();

#endif
