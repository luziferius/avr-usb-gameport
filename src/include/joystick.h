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

#ifndef ANALOG_READ_H_INCLUDED
#define ANALOG_READ_H_INCLUDED

#include <stdint.h>

/**
 * Structure that stores joystick reads. 4 axis and (up to) 8 digital buttons
 */
struct joystick_read_t {
    uint16_t axis[4];
    uint8_t buttons;
};

/**
 * Reads the joystick values and stores them in the global joystick_read_result variable.
 */
void read_joystick();


/**
 * Calibrates the given axis and do an averaged analog read with 4 reads.
 */
uint16_t calibrate_and_read_axis(const uint8_t axis);

/**
 * Set the ADC input pin to read from.
 * Should be used to set the channel before reading a value with analog_read().
 * 
 * Allowed values are 0-7, of which 6 and 7 are inaccessible
 * on the hardware side for chips in PDIP-28 form factor.
 * (These two are only routed on 32 pin TQFP and 32 pin MLF chips.)
 */
void joystick_set_analog_input_pin(const uint8_t channel);

/**
 * Reads the analog value on the pin selected by joystick_set_analog_input_pin().
 * Converts the analog value to a value with 10 bit precision using the ADC hardware
 * and returns it in the lowest 10 bits of the returned uint16_t value.
 */
uint16_t analog_read();

/**
 * Reads the analog value on the pin selected by joystick_set_analog_input_pin() 3 additional times
 * and averages the result by computing the arithmetic mean of all reads.
 * Converts the analog value to a value with 10 bit precision using the ADC hardware
 * and returns it in the lowest 10 bits of the returned uint16_t value.
 */
uint16_t analog_read4(uint16_t result);

#endif // ANALOG_READ_H_INCLUDED
