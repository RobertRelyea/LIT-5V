/**
 * Main for LIT-5V
 */

#include <stdio.h>
#include "pico/stdlib.h"

#include "lit5v_gpio.h"

// TODO: Handle these state variables in a better way, probably some encapsulating class
// Tonearm positioning
bool pos_rest_pin_high = false;
bool pos_leadin_pin_high = false;
bool pos_stop_pin_high = false;

// Cue/lift
bool up_pin_high = false;
bool down_pin_high = false;

// Control buttons
bool ctrl_cue_pin_high = false;
bool ctrl_start_pin_high = false;
bool ctrl_stop_pin_high = false;
bool ctrl_repeat_pin_high = false;

bool ctrl_repeat_bool = false;

typedef enum
{
    rest_down=0, // At the resting position, tonearm down
    rest_up,     // At the resting position, tonearm up
    seek_leadin, // Moving lifted tonearm to leadin position
    over_leadin, // Tonearm over leadin position
    playing,     // Tonearm down at/after leadin position
    manual_seek, // Tonearm up between leadin and end positions
    end,         // Tonearm up at end position
    seek_repeat, // Tonearm moving back to leadin position
    seek_rest,   // Tonearm moving back to rest position
    unknown
} tt_states;


// Todo: Debounce these inputs somehow, also make less gross
void gpio_callback(uint gpio, uint32_t events)
{
    switch(gpio)
    {
    case POS_REST_PIN:
        pos_rest_pin_high = !gpio_get(POS_REST_PIN);
        break;
    case POS_LEADIN_PIN:
        pos_leadin_pin_high = gpio_get(POS_LEADIN_PIN);
        break;
    case POS_STOP_PIN:
        pos_stop_pin_high = gpio_get(POS_STOP_PIN);
        break;
    case LIFT_UP_PIN:
        up_pin_high = !gpio_get(LIFT_UP_PIN);
        break;
    case LIFT_DOWN_PIN:
        down_pin_high = !gpio_get(LIFT_DOWN_PIN);
        break;
    case CTRL_CUE_PIN:
        ctrl_cue_pin_high = gpio_get(CTRL_CUE_PIN);
        break;
    case CTRL_START_PIN:
        ctrl_start_pin_high = gpio_get(CTRL_START_PIN);
        break;
    case CTRL_STOP_PIN:
        ctrl_stop_pin_high = gpio_get(CTRL_STOP_PIN);
        break;
    case CTRL_REPEAT_PIN:
        ctrl_repeat_pin_high = gpio_get(CTRL_REPEAT_PIN);
        if (ctrl_repeat_pin_high)
        {
            ctrl_repeat_bool = !ctrl_repeat_bool;
            printf("Repeat: %d\n", ctrl_repeat_bool);
        }
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

// Raises the tonearm
void raise_tonearm()
{
    set_tonearm_lift_dc(80, true);
    while(!up_pin_high){tight_loop_contents();}
    set_tonearm_lift_dc(0, true);
}

tt_states get_initial_state()
{
    // Possible starting states:
    //    rest_down
    //    rest_up
    //    over_leadin
    //    playing
    //    manual_seek
    //    end
}

// Moves the tonearm from any position safely back to the rest
tt_states rest_tonearm(tt_states current_state)
{
    switch(current_state)
    {
        // Handle cases where tonearm is over the record
    default:
    case unknown:
    case over_leadin:
    case playing:
    case manual_seek:
    case end:
        // Raise tonearm if down
        raise_tonearm();
        // Transport tonearm to rest position
        while(!pos_rest_pin_high)
        {
            set_tonearm_transport_dc(80, false);
            tight_loop_contents();
        }
        // Stop tonearm transport
        set_tonearm_transport_dc(0, true);
    case rest_up:
        // Lower tonearm onto rest
        lower_tonearm();
    }
    return rest_down;
}

tt_states tonearm_to_leadin(tt_states current_state)
{
    raise_tonearm();

    // If we are already in the leadin section, move towards rest until we are out of it
    while(pos_leadin_pin_high)
    {
        set_tonearm_transport_dc(80, false);
        tight_loop_contents();
    }

    // Move to the start of the leadin section
    while(!pos_leadin_pin_high)
    {
        set_tonearm_transport_dc(80, true);
        tight_loop_contents();
    }
    set_tonearm_transport_dc(0, true);
    // Yeah state stuff needs to be handled in a much more reactive way
    // but I just wanna listen to music right now.
    return over_leadin;
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
    printf("Initializing GPIO\n");

    init_led_gpio();

    // Initialize GPIO interrupt callback
    gpio_set_irq_callback(&gpio_callback);
    irq_set_enabled(IO_IRQ_BANK0, true);

    printf("IRQ Initialized\n");

    // Initialize all LIT-5V gpio
    init_tonearm_transport_gpio(&pos_rest_pin_high,
                                &pos_stop_pin_high,
                                &pos_leadin_pin_high);

    printf("Tonearm Transport Initialized\n");
    init_tonearm_lift_gpio(&down_pin_high, &up_pin_high);
    printf("Tonearm Lift Initialized\n");
    init_tonearm_tracking_gpio();
    printf("Tonearm Tracking Initialized\n");
    init_control_gpio(&ctrl_cue_pin_high,
                      &ctrl_start_pin_high,
                      &ctrl_stop_pin_high,
                      &ctrl_repeat_pin_high);
    printf("Control Initialized\n");

    //// Initialize turntable state machine
    // TODO: Determine initial state from tonearm switches and photodiodes
    // Set to an unknown state, bring the tonearm to resting position
    tt_states current_state = unknown;

    current_state = rest_tonearm(current_state);
    printf("Current state: %d\n", current_state);

    // TODO: This is a clumsy way to react to control input.
    // Ideally inputs should generate interrupts with immediate mechanical reaction
    // Tonearm movements should be handled without any blocking operations
    while(1)
    {
        printf("Rest: %d Leadin: %d Stop: %d\n", pos_rest_pin_high, pos_leadin_pin_high, pos_stop_pin_high);

        switch(current_state)
        {
        case rest_down:
            if (ctrl_cue_pin_high)
            {
                raise_tonearm();
                current_state = rest_up;
            }
            else if (ctrl_start_pin_high)
            {
                current_state = tonearm_to_leadin(current_state);
                lower_tonearm();
                current_state = playing;
            }
            break;

        case rest_up:
            if (ctrl_cue_pin_high || ctrl_stop_pin_high)
            {
                lower_tonearm();
                current_state = rest_down;
            }

            if (ctrl_start_pin_high)
            {
                current_state = tonearm_to_leadin(current_state);
                lower_tonearm();
                current_state = playing;
            }

            break;

        case manual_seek:
            if (ctrl_cue_pin_high)
            {
                lower_tonearm();
                current_state = playing;
            }
            if (ctrl_stop_pin_high)
            {
                if (!pos_leadin_pin_high)
                {
                    current_state = rest_tonearm(current_state);
                }
                else
                {
                    set_tonearm_transport_dc(60, false);
                }
            }
            else if(ctrl_start_pin_high && !pos_stop_pin_high)
            {
                set_tonearm_transport_dc(60, true);
            }
            else
            {
                set_tonearm_transport_dc(0, true);
            }
            break;

        case playing:
            if (ctrl_cue_pin_high)
            {
                set_tonearm_transport_dc(0, true);
                raise_tonearm();
                current_state = manual_seek;
            }
            else if (pos_stop_pin_high)
            {
                set_tonearm_transport_dc(0, true);

                // Go back to the leadin and lower the tonearm if repeat is on
                if (ctrl_repeat_bool)
                {
                    current_state = tonearm_to_leadin(current_state);
                    lower_tonearm();
                    current_state = playing;
                }
                // Rest otherwise
                else
                {
                    current_state = rest_tonearm(current_state);
                }
            }
            else if (ctrl_stop_pin_high)
            {
                current_state = rest_tonearm(current_state);
            }
            else
            {
                // Tonearm tracking update
                int32_t tracking_error = get_tracking_error();
                int32_t scaled_tracking_error = tracking_error / 40;

                int dc = scaled_tracking_error;

                if (dc > 0)
                {
                    set_tonearm_transport_dc((dc + 30), true);
                }
                else if (dc <= 0)
                {
                    set_tonearm_transport_dc(-1 * (dc + 30), false);
                }

                /* // TODO: Better debug logging, this kills the main loop */
                /* printf("Tracking error %d\t Scaled %d\n", tracking_error, scaled_tracking_error); */
                /* printf("Cue : %d\n", ctrl_cue_pin_high); */
                /* sleep_ms(50); */
            }

            break;
        }
    }
}
