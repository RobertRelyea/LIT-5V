/**
 * Main for LIT-5V
 */

// TODO: All these should be in an include

// Output PWM signals on pins 0 and 1

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"

//// Inputs
// Tonearm Positioning
#define POS_REST_PIN 18
#define POS_RECORD_PIN 19
#define POS_STOP_PIN 22
// Tonearm Tracking
#define TRACK_IN_PIN 28
#define TRACK_OUT_PIN 27
#define TRACK_IN_ADC 2
#define TRACK_OUT_ADC 1
// Tonearm lift
#define LIFT_UP_PIN 14
#define LIFT_DOWN_PIN 15

//// Outputs
// Tonearm Transport
#define TRANS_FWD_PIN 9
#define TRANS_REV_PIN 8

// Tonearm Lift
#define LIFT_FWD_PIN 10
#define LIFT_REV_PIN 11

// Turntable
#define TT_FWD_PIN 16

//// Buttons
// Lift/cue
#define CUE_PIN 2


bool stop_pin_high = false;
bool rest_pin_high = false;

bool up_pin_high = false;
bool down_pin_high = false;

bool cue_pin_high = false;

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

// TODO: Debounce these inputs somehow
void gpio_callback(uint gpio, uint32_t events)
{
    /* printf("Callback for gpio %d with events %d\n", gpio, events); */
    /* printf("Rise: %d \n", GPIO_IRQ_EDGE_RISE & events); */
    /* printf("Fall: %d \n", GPIO_IRQ_EDGE_FALL & events); */
    if (gpio == POS_REST_PIN)
    {
        handle_pin_event(events, &rest_pin_high);
    }
    else if (gpio == POS_STOP_PIN)
    {
        handle_pin_event(events, &stop_pin_high);
    }
    else if (gpio == LIFT_UP_PIN)
    {
        up_pin_high = gpio_get(LIFT_UP_PIN);
    }
    else if (gpio == LIFT_DOWN_PIN)
    {
        down_pin_high = gpio_get(LIFT_DOWN_PIN);
    }
    else if (gpio == CUE_PIN)
    {
        handle_pin_event(events, &cue_pin_high);
    }
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

void init_tonearm_transport_gpio()
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
    // TODO: Maybe set the callback in a standalone IRQ function call instead of transport setup
    gpio_set_irq_enabled_with_callback(POS_REST_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    if (gpio_get(POS_REST_PIN))
        rest_pin_high = true;

    // Set up transport stop photodiode pin
    gpio_init(POS_STOP_PIN);
    gpio_set_dir(POS_STOP_PIN, GPIO_IN);
    gpio_disable_pulls(POS_STOP_PIN);
    gpio_set_irq_enabled(POS_STOP_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    if (gpio_get(POS_STOP_PIN))
        stop_pin_high = true;

}

void init_tonearm_lift_gpio()
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
    // TODO: This line must be run after attaching a callback to the GPIO IRQ
    gpio_set_irq_enabled(LIFT_UP_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    if (gpio_get(LIFT_UP_PIN))
        up_pin_high = true;

    // Set up transport stop photodiode pin
    gpio_init(LIFT_DOWN_PIN);
    gpio_set_dir(LIFT_DOWN_PIN, GPIO_IN);
    gpio_pull_down(LIFT_DOWN_PIN);
    gpio_set_slew_rate(LIFT_DOWN_PIN, GPIO_SLEW_RATE_SLOW);
    gpio_set_irq_enabled(LIFT_DOWN_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    if (gpio_get(LIFT_DOWN_PIN))
        down_pin_high = true;
}

void init_control_gpio()
{
    // Set up tonearm transport positioning GPIO
    // Set up rest microswitch pin
    gpio_init(CUE_PIN);
    gpio_set_dir(CUE_PIN, GPIO_IN);
    gpio_pull_down(CUE_PIN);
    // TODO: This line must be run after attaching a callback to the GPIO IRQ
    gpio_set_irq_enabled(CUE_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    if (gpio_get(CUE_PIN))
        cue_pin_high = true;
}

// Lowers the tonearm
void lower_tonearm()
{
    set_tonearm_lift_dc(65, false);
    while(!down_pin_high){tight_loop_contents();}
    set_tonearm_lift_dc(0, true);
}

void raise_tonearm()
{
    set_tonearm_lift_dc(90, true);
    while(!up_pin_high){tight_loop_contents();}
    set_tonearm_lift_dc(0, true);
}

void init_tonearm_tracking_gpio()
{
    adc_init();

    adc_gpio_init(TRACK_IN_PIN);
    gpio_pull_down(TRACK_IN_PIN);

    adc_gpio_init(TRACK_OUT_PIN);
    gpio_pull_down(TRACK_OUT_PIN);
}

int32_t get_tracking_error()
{
    adc_select_input(TRACK_IN_ADC);
    int32_t track_in_val = (int32_t)adc_read();
    adc_select_input(TRACK_OUT_ADC);
    int32_t track_out_val = (int32_t)adc_read();

    int32_t tracking_error = track_in_val - track_out_val;

    return tracking_error;
}

int main() {
    stdio_init_all();

    init_tonearm_transport_gpio();
    init_tonearm_lift_gpio();
    init_tonearm_tracking_gpio();
    init_turntable_gpio();
    init_control_gpio();

    bool tonearm_up = up_pin_high;

    set_turntable_dc(0);

    int base_dc = 0;

    while (1)
    {

        if (cue_pin_high)
        {
            tonearm_up = !tonearm_up;
        }

        if (tonearm_up && down_pin_high)
        {
            raise_tonearm();
        }

        if (!tonearm_up && up_pin_high)
        {
            lower_tonearm();
        }

        if (down_pin_high)
            base_dc = 0;
        else
            base_dc = 0;

        int32_t tracking_error = get_tracking_error();
        int32_t scaled_tracking_error = tracking_error / 40;

        int dc = base_dc + scaled_tracking_error;

        if (dc > 0)
        {
            set_tonearm_transport_dc(dc, true);
        }
        else if (dc <= 0)
        {
            set_tonearm_transport_dc(-1 * dc, false);
        }


        printf("Tracking error %d\t Scaled %d\n", tracking_error, scaled_tracking_error);
        printf("Cue : %d\n", cue_pin_high);

        sleep_ms(50);
    }
}
