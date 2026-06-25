#include "retroblue_main.h"
#include "Joysticks.h"

uint32_t regread = 0;

void button_task()
{
    regread = REG_READ(GPIO_IN_REG) & PIN_BIT_MASK;

    // g_button_data.b_right  = read_button(regread, GPIO_BTN_CIRCLE);
    // g_button_data.b_down   = read_button(regread, GPIO_BTN_CROSS);
    // g_button_data.b_up     = false;
    // g_button_data.b_left   = read_button(regread, GPIO_BTN_SQUARE);
    // g_button_data.t_l      = read_button(regread, GPIO_BTN_L1);
    // g_button_data.t_zl     = read_button(regread, GPIO_BTN_L2);
    // g_button_data.t_r      = false;
    // g_button_data.t_zr     = false;
    // g_button_data.b_start  = read_button(regread, GPIO_BTN_START);
    // g_button_data.b_select = false;
    // g_button_data.b_home   = read_button(regread, GPIO_BTN_HOME);
    // g_button_data.b_capture= false;
    // g_button_data.sb_right = read_button(regread, GPIO_BTN_R3);
    // g_button_data.sb_left  = false;
    // g_button_data.d_up     = false;
    // g_button_data.d_down   = false;
    // g_button_data.d_left   = false;
    // g_button_data.d_right  = false;

    static bool modifier_active = false;
    static bool start_prev = false;
    bool start_held = read_button(regread, GPIO_BTN_TRIANGLE);

    // Toggle modifier on + button press (not hold)
    if (start_held && !start_prev)
    {
        modifier_active = !modifier_active;
    }
    start_prev = start_held;

    bool circle = read_button(regread, GPIO_BTN_CIRCLE);
    bool cross  = read_button(regread, GPIO_BTN_CROSS);
    bool square = read_button(regread, GPIO_BTN_SQUARE);
    bool l1     = read_button(regread, GPIO_BTN_L1);
    bool l2     = read_button(regread, GPIO_BTN_L2);
    bool start     = read_button(regread, GPIO_BTN_START);

    // If a non-soft-toggle button is pressed while modifier is active (e.g. an
    // accidental toggle press), cancel the modifier so subsequent presses of
    // circle/l1/l2 read normally instead of their modified mapping.
    if (modifier_active && (square || cross || start))
    {
        modifier_active = false;
    }

    if (!modifier_active)
    {
        g_button_data.b_right  = circle;
        g_button_data.b_down   = cross;
        g_button_data.b_left   = square;
        g_button_data.b_up     = false;
        g_button_data.t_l      = l1;
        g_button_data.t_zl     = l2;
        g_button_data.t_r      = false;
        g_button_data.t_zr     = false;
        g_button_data.b_start  = start;
    }
    else
    {
        g_button_data.b_right  = false;
        g_button_data.b_down   = cross;    // B stays B
        g_button_data.b_left   = square;   // Y stays Y
        g_button_data.b_up     = circle;   // X = A in modifier mode
        g_button_data.t_l      = false;
        g_button_data.t_zl     = false;
        g_button_data.t_r      = l1;       // R = L in modifier mode
        g_button_data.t_zr     = l2;       // ZR = ZL in modifier mode
        g_button_data.b_start  = false;

        static bool soft_prev = false;
        bool soft_held = circle || l1 || l2;
        if (!soft_held && soft_prev)
        {
            modifier_active = false;
        }
        soft_prev = soft_held;

    }

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
    g_stick_data.lsx = read_joystick_retroblue(ADC_STICK_RY, RBS_REVERSE_AXIS_LX); // D35 tracks left/right motion
    g_stick_data.lsy = read_joystick_retroblue(ADC_STICK_RX, RBS_REVERSE_AXIS_LY); // D34 tracks up/down motion
    g_stick_data.rsx = read_joystick_retroblue(ADC_STICK_RX, RBS_REVERSE_AXIS_RX);
    g_stick_data.rsy = read_joystick_retroblue(ADC_STICK_RY, RBS_REVERSE_AXIS_RY);

    pmw_joystick_update();
    g_stick_data.rsx = (uint16_t)map_i(
        pmw_get_axis_x(), PMW_AXIS_MIN, PMW_AXIS_MAX, ANALOG_MIN, ANALOG_MAX);
    g_stick_data.rsy = (uint16_t)map_i(
        pmw_get_axis_y(), PMW_AXIS_MIN, PMW_AXIS_MAX, ANALOG_MIN, ANALOG_MAX);
}

void retroblue_init()
{
    setup_gpios();
    initFlashBleGamepad();

    // ADD — init PMW before registering callbacks
    pmw_joystick_init();

    rb_register_button_callback(button_task);
    rb_register_stick_callback(stick_task);

    if (rb_api_startController() == RB_OK)
        setLed();
}
