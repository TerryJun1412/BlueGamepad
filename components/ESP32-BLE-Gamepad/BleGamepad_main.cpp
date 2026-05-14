#include "BleGamepad_main.h"

BleGamepad bleGamepad("BlueGamepad", "EOLVERA85", 100);

uint32_t regread_button = 0;

buttons_s buttons[NUM_PHYSICAL_BTNS] =
{
    { "A",    GPIO_BTN_CIRCLE, BTN_NUM_A,     false, false },
    { "B",    GPIO_BTN_CROSS,  BTN_NUM_B,     false, false },
    { "Y",    GPIO_BTN_SQUARE, BTN_NUM_Y,     false, false },
    { "L",    GPIO_BTN_L1,     BTN_NUM_L1,    false, false },
    { "ZL",   GPIO_BTN_L2,     BTN_NUM_L2,    false, false },
    { "+",    GPIO_BTN_START,  BTN_NUM_START, false, false },
    { "Home", GPIO_BTN_HOME,   BTN_NUM_Z,     false, false },
    { "R3",   GPIO_BTN_R3,     BTN_NUM_R3,    false, false },
};

joysticks_s joysticks[NUM_OF_JOYSTICKS] =
{
    { "Joystick R", ADC_STICK_RX, JOYSTICK_CEN, JOYSTICK_CEN, BLE_REVERSE_AXIS_RX,
                    ADC_STICK_RY, JOYSTICK_CEN, JOYSTICK_CEN, BLE_REVERSE_AXIS_RY }
};

void button_task(void * pvParameters)
{
    const char* TAG = "button_task";

    while (true)
    {
        regread_button = REG_READ(GPIO_IN_REG) & PIN_BIT_MASK;

        if (bleGamepad.isConnected())
        {
            for (int i = 0; i < NUM_PHYSICAL_BTNS; i++)
            {
                buttons[i].current_state = read_button(regread_button, buttons[i].gpio_num);

                if (buttons[i].current_state != buttons[i].previous_state)
                {
                    if (buttons[i].current_state)
                    {
                        ESP_LOGI(TAG, "Press Button: %s", buttons[i].name);
                        bleGamepad.press(buttons[i].physical_button);
                    }
                    else
                    {
                        ESP_LOGI(TAG, "Release Button: %s", buttons[i].name);
                        bleGamepad.release(buttons[i].physical_button);
                    }

                    bleGamepad.sendReport();
                    buttons[i].previous_state = buttons[i].current_state;
                }
            }
        }

        syncController(regread_button);

        vTaskDelay(5);
    }
}

void joystick_task(void * pvParameters)
{
    const char* TAG = "joystick_task";

    while (true)
    {
        if (bleGamepad.isConnected())
        {
            joysticks[0].current_state_axis_x = read_joystick(joysticks[0].adc_channel_axis_x, joysticks[0].reverse_axis_x);
            joysticks[0].current_state_axis_y = read_joystick(joysticks[0].adc_channel_axis_y, joysticks[0].reverse_axis_y);

            if (joysticks[0].current_state_axis_x != joysticks[0].previous_state_axis_x ||
                joysticks[0].current_state_axis_y != joysticks[0].previous_state_axis_y)
            {
                ESP_LOGI(TAG, "Joystick R X: %d", joysticks[0].current_state_axis_x);
                ESP_LOGI(TAG, "Joystick R Y: %d", joysticks[0].current_state_axis_y);

                bleGamepad.setRightThumb(joysticks[0].current_state_axis_x, joysticks[0].current_state_axis_y);
                bleGamepad.sendReport();

                joysticks[0].previous_state_axis_x = joysticks[0].current_state_axis_x;
                joysticks[0].previous_state_axis_y = joysticks[0].current_state_axis_y;
            }
        }

        vTaskDelay(5);
    }
}

void blegamepad_init()
{
    setup_gpios();
    initFlashBleGamepad();

    BleGamepadConfiguration bleGamepadConfig;
    bleGamepadConfig.setAutoReport(false);
    bleGamepadConfig.setAxesMin(JOYSTICK_MIN);
    bleGamepadConfig.setAxesMax(JOYSTICK_MAX);
    bleGamepadConfig.setButtonCount(NUM_OF_BUTTONS);
    bleGamepadConfig.setHatSwitchCount(1);      // Switch requires a hat switch entry
    bleGamepadConfig.setIncludeXAxis(true);     // Left stick X (reports center, no hardware)
    bleGamepadConfig.setIncludeYAxis(true);     // Left stick Y (reports center, no hardware)
    bleGamepadConfig.setIncludeZAxis(true);     // Right stick X
    bleGamepadConfig.setIncludeRzAxis(true);    // Right stick Y
    bleGamepadConfig.setIncludeRxAxis(false);
    bleGamepadConfig.setIncludeRyAxis(false);

    bleGamepad.begin(&bleGamepadConfig);

    xTaskCreatePinnedToCore(joystick_task, "joystick_task", 4096, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(button_task,   "button_task",   4096, NULL, 1, NULL, 0);

    while (true)
    {
        if (bleGamepad.isConnected())
        {
            setLed();
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
