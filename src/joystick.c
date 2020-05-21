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

#include "analog_read.h"

struct joystick_read_t {
    uint16_t axis[4];
    uint8_t buttons;
};

static struct joystick_read_t joystick_read_result;

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


struct axis_state_t {
    uint8_t axis_1 : 2;
    uint8_t axis_2 : 2;
    uint8_t axis_3 : 2;
    uint8_t axis_4 : 2;
} current_axis_range;

/**
 * If the ADC measures a value above UPPER_THRESHOLD, the axis has a too low resistance,
 * so the divider should switch to the next-lower resistor, for better accuracy.
 */
#define UPPER_THRESHOLD 0x300
/**
 * If the ADC measures a value below LOWER_THRESHOLD, the axis has a too high resistance,
 * so the divider should switch to the next-higher resistor, for better accuracy.
 */
#define LOWER_THRESHOLD 0x00F


void read_joystick() {
    /*
     * Reads the four digital buttons from Port D 0 - Port D 3.
     */
    joystick_read_result.buttons = PIND & 0x0F;
    
    /*
     * Reads the four analog axis.
     * 
     * First ORed term selects the measurement range by selecting the
     * last chosen resistor in the resistor battery multiplexer.
     * 
     * The second term selects the axis in the axis multiplexer.
     * TODO: If the upmost two bits are never needed, (PORTB & 0xD0) may be dropped to 
     * save some CPU cycles.
     */
    PORTB = (PORTB & 0xD0) | current_axis_range.axis_1;
    joystick_read_result.axis[0] = analog_read4();
    
    PORTB = (PORTB & 0xD0) | current_axis_range.axis_2 | 0x01 << 3;
    joystick_read_result.axis[1] = analog_read4();
    
    PORTB = (PORTB & 0xD0) | current_axis_range.axis_3 | 0x02 << 3;
    joystick_read_result.axis[2] = analog_read4();
    
    PORTB = (PORTB & 0xD0) | current_axis_range.axis_4 | 0x03 << 3;
    joystick_read_result.axis[3] = analog_read4();
  
}
