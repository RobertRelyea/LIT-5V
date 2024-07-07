#include "lit5v_gpio.h"

uint32_t pwm_set_freq_duty(uint slice_num,
                           uint chan,uint32_t f, int d)
{
    uint32_t clock = 125000000;
    uint32_t divider16 = clock / f / 4096 + (clock % (f * 4096) != 0);
    if (divider16 / 16 == 0)
        divider16 = 16;
    uint32_t wrap = clock * 16 / divider16 / f - 1;
    pwm_set_clkdiv_int_frac(slice_num, divider16/16,
                            divider16 & 0xF);
    pwm_set_wrap(slice_num, wrap);
    pwm_set_chan_level(slice_num, chan, wrap * d / 100);
    return wrap;
}

void handle_pin_event(uint32_t events, bool* pin_bool)
{
    if (events & GPIO_IRQ_EDGE_RISE)
        *pin_bool = true;
    else if (events & GPIO_IRQ_EDGE_FALL)
        *pin_bool = false;
}

void set_motor_pwm(uint32_t duty_cycle, bool direction, uint fwd_pin, uint rev_pin)
{
    uint32_t fwd_dc = 0;
    uint32_t rev_dc = 0;

    uint forward_slice_num = pwm_gpio_to_slice_num(fwd_pin);
    uint forward_channel = pwm_gpio_to_channel(fwd_pin);

    uint reverse_slice_num = pwm_gpio_to_slice_num(rev_pin);
    uint reverse_channel = pwm_gpio_to_channel(rev_pin);

    if (direction)
    {
        fwd_dc = duty_cycle;
        rev_dc = 0;
    }
    else
    {
        fwd_dc = 0;
        rev_dc = duty_cycle;
    }

    // PWM freq for motor controls should be at or above audible range (20KHz)
    pwm_set_freq_duty(forward_slice_num, forward_channel, 20000, fwd_dc);
    pwm_set_freq_duty(reverse_slice_num, reverse_channel, 20000, rev_dc);
}

// Convenience function for tonearm transport motor
void set_tonearm_transport_dc(uint32_t duty_cycle, bool direction)
{
    set_motor_pwm(duty_cycle, direction, TRANS_FWD_PIN, TRANS_REV_PIN);
}

// Convenience function for tonearm list motor
void set_tonearm_lift_dc(uint32_t duty_cycle, bool direction)
{
    set_motor_pwm(duty_cycle, direction, LIFT_FWD_PIN, LIFT_REV_PIN);
}

void set_turntable_dc(uint32_t duty_cycle)
{
    uint forward_slice_num = pwm_gpio_to_slice_num(TT_FWD_PIN);
    uint forward_channel = pwm_gpio_to_channel(TT_FWD_PIN);
    pwm_set_freq_duty(forward_slice_num, forward_channel, 5, duty_cycle);
}

void init_turntable_gpio()
{
    //// Turntable motor
    // Set up turntable motor control pwm pins
    gpio_set_function(TT_FWD_PIN, GPIO_FUNC_PWM);

    // Find out which PWM slice is connected
    uint forward_slice_num = pwm_gpio_to_slice_num(TT_FWD_PIN);

    // Set the PWM running
    pwm_set_enabled(forward_slice_num, true);

}

void init_tonearm_tracking_gpio()
{
    adc_init();

    adc_gpio_init(TRACK_IN_PIN);
    gpio_pull_down(TRACK_IN_PIN);

    adc_gpio_init(TRACK_OUT_PIN);
    gpio_pull_down(TRACK_OUT_PIN);
}

void init_tonearm_lift_gpio(bool* down_pin_bool, bool* up_pin_bool)
{
    //// Tonearm Lift
    // Set up lift motor control pwm pins
    gpio_set_function(LIFT_FWD_PIN, GPIO_FUNC_PWM);
    gpio_set_function(LIFT_REV_PIN, GPIO_FUNC_PWM);

    // Find out which PWM slice is connected for both outputs
    uint forward_slice_num = pwm_gpio_to_slice_num(LIFT_FWD_PIN);
    uint reverse_slice_num = pwm_gpio_to_slice_num(LIFT_REV_PIN);

    // Set the PWM running
    pwm_set_enabled(forward_slice_num, true);
    pwm_set_enabled(reverse_slice_num, true);

    // Set up tonearm transport positioning GPIO
    // Set up rest microswitch pin
    gpio_init(LIFT_UP_PIN);
    gpio_set_dir(LIFT_UP_PIN, GPIO_IN);
    gpio_pull_down(LIFT_UP_PIN);
    gpio_set_slew_rate(LIFT_UP_PIN, GPIO_SLEW_RATE_SLOW);
    gpio_set_irq_enabled(LIFT_UP_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    if (gpio_get(LIFT_UP_PIN))
        *up_pin_bool = true;

    // Set up transport stop photodiode pin
    gpio_init(LIFT_DOWN_PIN);
    gpio_set_dir(LIFT_DOWN_PIN, GPIO_IN);
    gpio_pull_down(LIFT_DOWN_PIN);
    gpio_set_slew_rate(LIFT_DOWN_PIN, GPIO_SLEW_RATE_SLOW);
    gpio_set_irq_enabled(LIFT_DOWN_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    if (gpio_get(LIFT_DOWN_PIN))
        *down_pin_bool = true;
}

void init_tonearm_transport_gpio(bool* rest_pin_bool,
                                 bool* stop_pin_bool,
                                 bool* leadin_pin_bool)
{
    //// Tonearm transport
    // Set up transport motor control pwm pins
    gpio_set_function(TRANS_FWD_PIN, GPIO_FUNC_PWM);
    gpio_set_function(TRANS_REV_PIN, GPIO_FUNC_PWM);

    // Find out which PWM slice is connected for both outputs
    uint forward_slice_num = pwm_gpio_to_slice_num(TRANS_FWD_PIN);
    uint reverse_slice_num = pwm_gpio_to_slice_num(TRANS_REV_PIN);

    // Set the PWM running
    pwm_set_enabled(forward_slice_num, true);
    pwm_set_enabled(reverse_slice_num, true);

    // Set up tonearm transport positioning GPIO
    // Set up rest microswitch pin
    gpio_init(POS_REST_PIN);
    gpio_set_dir(POS_REST_PIN, GPIO_IN);
    gpio_pull_down(POS_REST_PIN);
    gpio_set_irq_enabled(POS_REST_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    if (gpio_get(POS_REST_PIN))
        *rest_pin_bool = true;

    // Set up transport stop photodiode pin
    gpio_init(POS_STOP_PIN);
    gpio_set_dir(POS_STOP_PIN, GPIO_IN);
    gpio_disable_pulls(POS_STOP_PIN);
    gpio_set_irq_enabled(POS_STOP_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    if (gpio_get(POS_STOP_PIN))
        *stop_pin_bool = true;

    // Set up transport LP lead-in photodiode pin
    gpio_init(POS_LEADIN_PIN);
    gpio_set_dir(POS_LEADIN_PIN, GPIO_IN);
    gpio_disable_pulls(POS_LEADIN_PIN);
    gpio_set_irq_enabled(POS_LEADIN_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    if (gpio_get(POS_LEADIN_PIN))
        *leadin_pin_bool = true;
}

// Set up control button GPIO
void init_control_gpio(bool* cue_pin_bool,
                       bool* start_pin_bool,
                       bool* stop_pin_bool,
                       bool* repeat_pin_bool)
{
    // Set up cue/lift button
    gpio_init(CTRL_CUE_PIN);
    gpio_set_dir(CTRL_CUE_PIN, GPIO_IN);
    gpio_pull_down(CTRL_CUE_PIN);
    gpio_set_irq_enabled(CTRL_CUE_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    if (gpio_get(CTRL_CUE_PIN))
        *cue_pin_bool = true;

    // Set up start button
    gpio_init(CTRL_START_PIN);
    gpio_set_dir(CTRL_START_PIN, GPIO_IN);
    gpio_pull_down(CTRL_START_PIN);
    gpio_set_irq_enabled(CTRL_START_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    if (gpio_get(CTRL_START_PIN))
        *start_pin_bool = true;

    // Set up stop button
    gpio_init(CTRL_STOP_PIN);
    gpio_set_dir(CTRL_STOP_PIN, GPIO_IN);
    gpio_pull_down(CTRL_STOP_PIN);
    gpio_set_irq_enabled(CTRL_STOP_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    if (gpio_get(CTRL_STOP_PIN))
        *stop_pin_bool = true;

    // Set up repeat button
    gpio_init(CTRL_REPEAT_PIN);
    gpio_set_dir(CTRL_REPEAT_PIN, GPIO_IN);
    gpio_pull_down(CTRL_REPEAT_PIN);
    gpio_set_irq_enabled(CTRL_REPEAT_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    if (gpio_get(CTRL_REPEAT_PIN))
        *repeat_pin_bool = true;
}
