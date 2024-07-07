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
