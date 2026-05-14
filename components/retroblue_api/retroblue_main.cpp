#include "retroblue_main.h"
#include "Joysticks.h"

uint32_t regread = 0;

void button_task()
{
    regread = REG_READ(GPIO_IN_REG) & PIN_BIT_MASK;

    g_button_data.b_right  = read_button(regread, GPIO_BTN_CIRCLE);
    g_button_data.b_down   = read_button(regread, GPIO_BTN_CROSS);
    g_button_data.b_up     = read_button(regread, GPIO_BTN_SQUARE);
    g_button_data.b_left   = false;
    g_button_data.t_l      = read_button(regread, GPIO_BTN_L1);
    g_button_data.t_zl     = read_button(regread, GPIO_BTN_L2);
    g_button_data.t_r      = false;
    g_button_data.t_zr     = false;
    g_button_data.b_start  = read_button(regread, GPIO_BTN_START);
    g_button_data.b_select = false;
    g_button_data.b_home   = read_button(regread, GPIO_BTN_HOME);
    g_button_data.b_capture= false;
    g_button_data.sb_right = read_button(regread, GPIO_BTN_R3);
    g_button_data.sb_left  = false;
    g_button_data.d_up     = false;
    g_button_data.d_down   = false;
    g_button_data.d_left   = false;
    g_button_data.d_right  = false;

    regread = 0;
}

void stick_task()
{
    g_stick_data.lsx = ANALOG_CENTER;
    g_stick_data.lsy = ANALOG_CENTER;
    g_stick_data.rsx = read_joystick_retroblue(ADC_STICK_RX, RBS_REVERSE_AXIS_RX);
    g_stick_data.rsy = read_joystick_retroblue(ADC_STICK_RY, RBS_REVERSE_AXIS_RY);
}

void retroblue_init()
{
    setup_gpios();
    initFlashBleGamepad();

    rb_register_button_callback(button_task);
    rb_register_stick_callback(stick_task);

    if (rb_api_startController() == RB_OK)
        setLed();
}
