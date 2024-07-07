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

// TODO: Debounce these inputs somehow, also make less gross
void gpio_callback(uint gpio, uint32_t events)
{
    switch(gpio)
    {
    case POS_REST_PIN:
        handle_pin_event(events, &rest_pin_high);
        break;
    case POS_STOP_PIN:
        handle_pin_event(events, &stop_pin_high);
        break;
    case LIFT_UP_PIN:
        up_pin_high = gpio_get(LIFT_UP_PIN);
        break;
    case LIFT_DOWN_PIN:
        down_pin_high = gpio_get(LIFT_DOWN_PIN);
        break;
    case CUE_PIN:
        handle_pin_event(events, &cue_pin_high);
        break;
    }

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

    // Initialize all LIT-5V gpio
    gpio_set_irq_callback(&gpio_callback);
    init_tonearm_transport_gpio(&rest_pin_high, &stop_pin_high);
    init_tonearm_lift_gpio(&down_pin_high, &up_pin_high);
    init_tonearm_tracking_gpio();
    init_turntable_gpio();
    init_control_gpio(&cue_pin_high);

    // Set up a basic tracking and tonearm control loop
    bool tonearm_up = up_pin_high;

    set_turntable_dc(0);

    int base_dc = 0;

    while (1)
    {
        // TODO: This gets wrecked by bouncing
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

        // TODO: Better debug logging, this kills the main loop
        printf("Tracking error %d\t Scaled %d\n", tracking_error, scaled_tracking_error);
        printf("Cue : %d\n", cue_pin_high);
        sleep_ms(50);
    }
}
