/**
   Header defines and gpio convenience functions for the LIT-5V.
 */

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

uint32_t pwm_set_freq_duty(uint slice_num,
                           uint chan,uint32_t f, int d);

void handle_pin_event(uint32_t events, bool* pin_bool);

void set_motor_pwm(uint32_t duty_cycle, bool direction, uint fwd_pin, uint rev_pin);

void set_tonearm_transport_dc(uint32_t duty_cycle, bool direction);

void set_tonearm_lift_dc(uint32_t duty_cycle, bool direction);

void set_turntable_dc(uint32_t duty_cycle);

void init_turntable_gpio();

void init_tonearm_tracking_gpio();

void init_tonearm_lift_gpio(bool* down_pin_bool, bool* up_pin_bool);

void init_tonearm_transport_gpio(bool* rest_pin_bool, bool* stop_pin_bool);

void init_control_gpio(bool* cue_pin_bool);
