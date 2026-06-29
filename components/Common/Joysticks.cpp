#include "Joysticks.h"
// --------------------------------------------------------------
#include "pmw3360.h"
#include <cstring>
//------------------------

uint32_t map(uint32_t x, uint32_t in_min, uint32_t in_max, uint32_t out_min, uint32_t out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

uint16_t read_joystick_n64_retroblue(int pot_value, bool reverse_axis)
{
    if (pot_value >= N64_JOYSTICK_DEADZONE_MIN && pot_value <= N64_JOYSTICK_DEADZONE_MAX)
        return ANALOG_CENTER;

    if (reverse_axis)
        return (uint16_t)map(pot_value, N64_JOYSTICK_MIN, N64_JOYSTICK_MAX, ANALOG_MAX, ANALOG_MIN);
    else
        return (uint16_t)map(pot_value, N64_JOYSTICK_MIN, N64_JOYSTICK_MAX, ANALOG_MIN, ANALOG_MAX);
}

uint16_t read_joystick_retroblue(adc1_channel_t adc_channel, bool reverse_axis)
{
    uint32_t raw = 0;

    for(int i = 0; i < NUMBER_SAMPLES; i++)
    {
        raw += adc1_get_raw(adc_channel);
        vTaskDelay(5);
    }

    raw = raw / NUMBER_SAMPLES;

    if(raw >= ANALOG_DEADZONE_MIN && raw <= ANALOG_DEADZONE_MAX)
        return ANALOG_CENTER;

    return reverse_axis ? (uint16_t)map(raw, ANALOG_MIN, ANALOG_MAX, ANALOG_MAX, ANALOG_MIN) : raw;
}

int16_t read_joystick_n64(int pot_value, bool reverse_axis)
{
    if (pot_value >= N64_JOYSTICK_DEADZONE_MIN && pot_value <= N64_JOYSTICK_DEADZONE_MAX)
        return JOYSTICK_CEN;

    return reverse_axis ? pot_value * -1 : pot_value;
}

int16_t read_joystick(adc1_channel_t adc_channel, bool reverse_axis)
{
    uint32_t raw = 0;

    for(int i = 0; i < NUMBER_SAMPLES; i++)
    {
        raw += adc1_get_raw(adc_channel);
        vTaskDelay(5);
    }

    raw = raw / NUMBER_SAMPLES;

    if(raw >= ANALOG_DEADZONE_MIN && raw <= ANALOG_DEADZONE_MAX)
        return JOYSTICK_CEN;

    return reverse_axis ? (int16_t)map(raw, ANALOG_MIN, ANALOG_MAX, JOYSTICK_MAX, JOYSTICK_MIN) : raw;
}

bool read_trigger_button(adc1_channel_t adc_channel, bool reverse_trigger)
{
    return (bool)read_trigger(adc_channel, reverse_trigger);
}

int16_t read_trigger(adc1_channel_t adc_channel, bool reverse_trigger)
{
    uint32_t raw = 0;

    for(int i = 0; i < NUMBER_SAMPLES; i++)
    {
        raw += adc1_get_raw(adc_channel);
        vTaskDelay(5);
    }

    raw = raw / NUMBER_SAMPLES;

    if (reverse_trigger)
    {
        if (raw >= (ANALOG_MAX - ANALOG_DEADZONE_TRIGGER) && raw <= ANALOG_MAX)
            return JOYSTICK_MIN;
        else
            return (int16_t)map(raw, ANALOG_MIN, ANALOG_MAX, JOYSTICK_MAX, JOYSTICK_MIN);
    }
    else
    {
        if (raw > -1 && raw <= ANALOG_DEADZONE_TRIGGER)
            return JOYSTICK_MIN;
        else
            return raw;
    }
}
// -----------------------------------------------------------------

pmw_joystick_s pmw_joystick;

void pmw_joystick_init()
{
    strncpy(pmw_joystick.name, "PMW_JOY", 12);
    pmw_joystick.current_state_axis_x  = 0;
    pmw_joystick.current_state_axis_y  = 0;
    pmw_joystick.previous_state_axis_x = 0;
    pmw_joystick.previous_state_axis_y = 0;

    pmw3360_init();
}

void pmw_joystick_update()
{
   int16_t acc_x = 0;
    int16_t acc_y = 0;
    uint8_t motion_detected = 0;

    // sample over a window — mirrors NUMBER_SAMPLES pattern
    for (int i = 0; i < NUMBER_SAMPLES; i++)
    {
        pmw3360_motion_t motion;
        pmw3360_read_motion(&motion);

        if (motion.motion & 0x80)
        {
            acc_x += motion.delta_x;
            acc_y += motion.delta_y;
            motion_detected = 1;
        }

        vTaskDelay(1);   // shorter than joystick's 5ms — sensor needs fast polling
    }

    // ESP_LOGI("PMW_UPDATE", "motion=%d acc_x=%d acc_y=%d joy_x=%.2f joy_y=%.2f",
    //     motion_detected,
    //     acc_x,
    //     acc_y,
    //     pmw_joystick.current_state_axis_x,
    //     pmw_joystick.current_state_axis_y);

    if (!motion_detected) return;

    float dx = (float)acc_x;
    float dy = (float)acc_y;

    if (dx > -PMW_DEADZONE && dx < PMW_DEADZONE) dx = 0;
    if (dy > -PMW_DEADZONE && dy < PMW_DEADZONE) dy = 0;

    pmw_joystick.previous_state_axis_x = pmw_joystick.current_state_axis_x;
    pmw_joystick.previous_state_axis_y = pmw_joystick.current_state_axis_y;

    pmw_joystick.current_state_axis_x += dx * PMW_SENSITIVITY;
    pmw_joystick.current_state_axis_y += dy * PMW_SENSITIVITY;

    pmw_joystick.current_state_axis_x = constrain(
        pmw_joystick.current_state_axis_x, PMW_AXIS_MIN, PMW_AXIS_MAX);
    pmw_joystick.current_state_axis_y = constrain(
        pmw_joystick.current_state_axis_y, PMW_AXIS_MIN, PMW_AXIS_MAX);

    if (abs(pmw_joystick.current_state_axis_x) < PMW_EDGE_ZONE)
        pmw_joystick.current_state_axis_x *= PMW_DECAY;
    if (abs(pmw_joystick.current_state_axis_y) < PMW_EDGE_ZONE)
        pmw_joystick.current_state_axis_y *= PMW_DECAY;
}

void pmw_joystick_recenter()
{
    pmw_joystick.current_state_axis_x  = 0;
    pmw_joystick.current_state_axis_y  = 0;
    pmw_joystick.previous_state_axis_x = 0;
    pmw_joystick.previous_state_axis_y = 0;
}

int16_t pmw_get_axis_x()
{
    return (int16_t)pmw_joystick.current_state_axis_x;
}

int16_t pmw_get_axis_y()
{
    return (int16_t)pmw_joystick.current_state_axis_y;
}
// --------------------------------------------------------------