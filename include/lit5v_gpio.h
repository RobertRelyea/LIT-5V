/**
   Header defines and gpio convenience functions for the LIT-5V.
 */

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"

// TODO Enums might make these much more readable

//// Inputs
// Tonearm Positioning
#define POS_REST_PIN 16
#define POS_LEADIN_PIN 4
#define POS_STOP_PIN 6
// Tonearm Tracking
#define TRACK_IN_PIN 27
#define TRACK_OUT_PIN 28
// TODO: Is there some library function to look these up?
#define TRACK_IN_ADC 1
#define TRACK_OUT_ADC 2
// Tonearm lift
#define LIFT_UP_PIN 17
#define LIFT_DOWN_PIN 18

//// Outputs
// Tonearm Transport
#define TRANS_FWD_PIN 19
#define TRANS_REV_PIN 20

// Tonearm Lift
#define LIFT_FWD_PIN 21
#define LIFT_REV_PIN 22

//// Buttons
// Lift/cue
#define CTRL_SPEED_PIN 7
#define CTRL_REPEAT_PIN 8
#define CTRL_START_PIN 9
#define CTRL_CUE_PIN 10
#define CTRL_STOP_PIN 11

//// LEDs
#define LED_LEADIN_PIN 2
#define LED_SINGLE_PIN 3
#define LED_END_PIN 5
#define LED_TT_PIN 26

uint32_t pwm_set_freq_duty(uint slice_num,
                           uint chan,uint32_t f, int d);

void handle_pin_event(uint32_t events, bool* pin_bool);

void set_motor_pwm(uint32_t duty_cycle, bool direction, uint fwd_pin, uint rev_pin);

void set_tonearm_transport_dc(uint32_t duty_cycle, bool direction);

void set_tonearm_lift_dc(uint32_t duty_cycle, bool direction);

void init_tonearm_tracking_gpio();

void init_tonearm_lift_gpio(bool* down_pin_bool, bool* up_pin_bool);

void init_tonearm_transport_gpio(bool* rest_pin_bool,
                                 bool* stop_pin_bool,
                                 bool* lp_pin_bool);

void init_control_gpio(bool* cue_pin_bool,
                       bool* start_pin_bool,
                       bool* stop_pin_bool,
                       bool* repeat_pin_bool);

void init_led_gpio();
