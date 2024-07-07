/**
 * Main for LIT-5V
 */

#include <stdio.h>
#include "pico/stdlib.h"

#include "lit5v_gpio.h"

// TODO: Handle these state variables in a better way, probably some encapsulating class
bool stop_pin_high = false;
bool rest_pin_high = false;

bool up_pin_high = false;
bool down_pin_high = false;

bool cue_pin_high = false;

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
