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

#include <avr/io.h>

#include "hwinit.h"

void hwinit() {
 
    // Disable some unused components: USART, SPI, 16 bit TIMER1
    PRR |=  _BV(PRUSART0) | _BV(PRSPI) | _BV(PRTIM1);
    
    // Enable the ADC. See Datasheet: Chapter 28.2,  p. 305:
    // “The Power Reduction ADC bit in the Power Reduction Register (PRR.PRADC)
    // must be written to '0' in order to be enable the ADC.”
    PRR &= ~_BV(PRADC);
    
    // Set the oscillator to 12.8 MHz, which is the only available frequency
    // usable with both V-USB and the ATmega328P. Value empirically determined.
    // TODO: Read this value from the EEPROM, instead of hardcoding it here, as it
    // has to be configured individually per device.
    OSCCAL = 218;
    
    /* Datasheed: 28.9.2. ADC Control and Status Register A, page 319:
     * Set the ADC prescaler to 128. Sets the ADC frequency to F_CPU/128 = 12.8MHz/128 = 100 kHz,
     * Which is in the range for high precision (50-200Mhz).
     * 
     * Datasheet: 28.4. Prescaling and Conversion Timing, page 308:
     * “By default, the successive approximation circuitry requires an input clock frequency between 50kHz and
     * 200kHz to get maximum resolution. If a lower resolution than 10 bits is needed, the input clock frequency
     * to the ADC can be higher than 200kHz to get a higher sample rate.”
     */
    ADCSRA |= _BV(ADPS0) | _BV(ADPS1) | _BV(ADPS2);
    
}
