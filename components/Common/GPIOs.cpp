#include "GPIOs.h"
#include "Buttons.h"
#include "Joysticks.h"

uint64_t PIN_BIT_MASK;

void setup_gpios()
{
    PIN_BIT_MASK = GPIO_BITMASK_BUTTONS;

    // Right analog stick ADC
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC_STICK_RX, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(ADC_STICK_RY, ADC_ATTEN_DB_11);

    // Button inputs with internal pull-ups
    gpio_config_t io_conf = {};
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = PIN_BIT_MASK;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);

    // LED outputs
    gpio_config_t io_conf_led = {};
    io_conf_led.mode = GPIO_MODE_OUTPUT;
    io_conf_led.pin_bit_mask = GPIO_BITMASK_LED;
    io_conf_led.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf_led.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf_led.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf_led);
}
