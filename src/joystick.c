/* Copyright (C) 2020 Thomas Hess <thomas.hess@udo.edu>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#include "joystick.h"


/**
 * Stores the most recent joystick read. Used to send a data packet over USB.
 */
struct joystick_read_t joystick_read_result;

/* Analog axis use a 100kΩ potentiometer connected to Vcc. To read such an axis,
 * the current resistance has to be determined, which can be done by building
 * a voltage divider with another known resistor connected to ground and measuring
 * the voltage between those two resistors using the internal ADC.
 * 
 * But the total range of the potentiometer is too large to measure using the internal 10 bit ADC and a single resistor.
 * Thus it is neccessary to use multiple resistors and switch between them using an analog multiplexer to increase the
 * measurement range available.
 * 
 * The axis_state variable stores the multiplexer state for each axis indivitually. The physical
 * axis changes rather slowly (unless it’s a digital hat),
 * so the optimum measurement range is likely the same between measurements of the same axis.
 * 
 * If the last measurement is above or below the optimal range, the previous or next range is tried.
 */
#define AXIS_RANGE_BITS 2

struct axis_state_t {
    uint8_t axis_1 : AXIS_RANGE_BITS;
    uint8_t axis_2 : AXIS_RANGE_BITS;
    uint8_t axis_3 : AXIS_RANGE_BITS;
    uint8_t axis_4 : AXIS_RANGE_BITS;
} current_axis_range;


inline void set_selected_resistor(const uint8_t axis, const uint8_t new_multiplexer_channel) {
    switch(axis) {
        case(0):
            current_axis_range.axis_1 = new_multiplexer_channel & 0x03;
            break;
        case(1):
            current_axis_range.axis_2 = new_multiplexer_channel & 0x03;
            break;
        case(2):
            current_axis_range.axis_3 = new_multiplexer_channel & 0x03;
            break;
        case(3):
            current_axis_range.axis_4 = new_multiplexer_channel & 0x03;
            break;
        default:
            break;
    }
}


const inline uint8_t get_selected_resistor(const uint8_t axis) {
    switch(axis) {
        case(0):
            return current_axis_range.axis_1;
        case(1):
            return current_axis_range.axis_2;
        case(2):
            return current_axis_range.axis_3;
        case(3):
            return current_axis_range.axis_4;
        default:
            return 0;
    }
}

/**
 * If the ADC measures a value above ADC_UPPER_THRESHOLD, the axis has a too low resistance,
 * so the voltage divider should switch to the next-lower resistor, for better accuracy.
 */
#define ADC_UPPER_THRESHOLD 0x300


/**
 * If the ADC measures a value below ADC_LOWER_THRESHOLD, the axis has a too high resistance,
 * so the voltage divider should switch to the next-higher resistor, for better accuracy.
 */
#define ADC_LOWER_THRESHOLD 0x00F


ISR(ADC_vect) {
    /* Called when the ADC interrupt wakes the device. Nothing to do here.
     * The only purpose is to implicitly clear the interrupt flags in SREG and ADCSRA,
     * and to wake up the CPU that sleeps during the conversion.
     */
}


void read_joystick() {
    
    /* Reads the four digital buttons from Port C 0-3
     */
    joystick_read_result.buttons = PINC & 0x0F;

//#pragma unroll(4)
    for (uint8_t axis = 0; axis < 4; ++axis) {
        joystick_read_result.axis[axis] = calibrate_and_read_axis(axis);
    }
}

/**
 * An ADC conversion result.
 */
union adc_result_t {
    uint16_t result;
    uint8_t bytes[2];
};

void joystick_set_analog_input_pin(const uint8_t channel) {
    /* Datasheet: 28.9.1. ADC Multiplexer Selection Register, page 317:
     * - Only allow the plain 8 ADC channels.
     * - Make sure that the channel selection can not write the upper 3 bits (bits 5, 6 & 7).
     * - Do not reset the upper 3 bits REFS1, REFS0, ADLAR.
     */
    ADMUX = (channel & 0x7) | (ADMUX & 0xE0);
}


uint16_t calibrate_and_read_axis(const uint8_t axis) {
    uint8_t should_step_down, should_step_up;
    uint16_t axis_value;
    do {
        const uint8_t selected_resistor = get_selected_resistor(axis);
        /* Reads the analog axis.
        * 
        * First ORed term selects the measurement range by selecting a
        * resistor in the resistor battery multiplexer.
        * 
        * The second term selects the axis in the axis multiplexer.
        */
        PINB = selected_resistor | axis << 3;
        axis_value = analog_read();
        
        should_step_down = selected_resistor && axis_value > ADC_UPPER_THRESHOLD;
        should_step_up = (selected_resistor + 1) & ~_BV(AXIS_RANGE_BITS)  && axis_value < ADC_LOWER_THRESHOLD;
        
        if (should_step_down) {
            set_selected_resistor(axis, selected_resistor + 1);
        } else if (should_step_up) {
            set_selected_resistor(axis, selected_resistor - 1);
        }
    } while(should_step_down || should_step_up);
    
    return analog_read4(axis_value);
}

uint16_t analog_read() {
    /* Enter ADC Noise Reduction Mode
     * Datasheet: 14.5. ADC Noise Reduction Mode, page 63:
     * “When the SM[2:0] bits are written to '001', the SLEEP instruction makes the MCU enter ADC Noise
     * Reduction mode, stopping the CPU but allowing the ADC, the external interrupts, […] to continue
     * operating (if enabled).”
     * 
     * “If the ADC is enabled, a conversion starts automatically when this mode is entered.”
     */
    set_sleep_mode(SLEEP_MODE_ADC);
    sleep_enable();
    sleep_cpu();

    /* The CPU might have been woken up by an interrupt caused by the USB interface. In this case,
     * go to sleep again, until the conversion is completed.
     * Datasheet: 28.3. Starting a Conversion, page 307:
     * “ADCS will stay high as long as the conversion is in progress, and will be
     * cleared by hardware when the conversion is completed.”
     */
    while (ADCSRA & _BV(ADSC)) {
        sleep_cpu();
    }
    sleep_disable();

    // Conversion finished, the result can be read.
    union adc_result_t result;
    /* Datasheet 28.9.3. ADC Data Register Low (ADLAR=0), page 321:
     * “ADCL must be read first, then ADCH.”
     */
    result.bytes[0] = ADCL;
    result.bytes[1] = ADCH & 0x3;

    return result.result;
}

uint16_t analog_read4(uint16_t result) {
    // Compute the arithmetic mean of 4 results.
    // Each individual component has 10 bit accuracy,
    // the sum therefore uses at most 12 bits, which fits into a single uint16_t.
    result += analog_read();
    result += analog_read();
    result += analog_read();
    result >>= 2;
    return result;
}
