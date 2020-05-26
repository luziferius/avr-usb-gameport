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
 #include <avr/wdt.h>
 
#include "hwinit.h"


void hwinit() {
    // Disable the hardware watchdog, which is not used.
    wdt_disable();

    // Disable some unused components: USART, SPI, 16 bit TIMER1
    PRR |=  _BV(PRUSART0) | _BV(PRSPI) | _BV(PRTIM1);
    
    /* Set the oscillator to 12.8 MHz, which is the only available frequency
     * usable with both V-USB and the ATmega328P’s internal oscillator.
     * V-USB can only use a device clock of 12.8MHz or 16.5 MHz when using the device-internal oscillator,
     * but the ATmega328P can only be tuned to 12.8 MHz, as the internal oscillator maxes out at about 15MHz.
     * OSCCAL value empirically determined.
     * TODO: Read this value from the EEPROM, instead of hardcoding it here, as it
     * has to be configured individually per device.
     */
    OSCCAL = 218;
    
    /* Enable the ADC. See Datasheet: Chapter 28.2,  page 305:
     * “The Power Reduction ADC bit in the Power Reduction Register (PRR.PRADC)
     * must be written to '0' in order to be enable the ADC.”
     */
    PRR &= ~_BV(PRADC);
    
    /* Datasheed: 28.9.2. ADC Control and Status Register A, page 319:
     * Enable the ADC circuitry:
     * - Enable the circuitry (ADEN)
     * - Enable the ADC Conversion Complete Interrupt (ADIE)
     * - Set the ADC prescaler to 128. Sets the ADC frequency to F_CPU/128 = 12.8MHz/128 = 100 kHz,
     *   which is in the range for high precision (50-200Mhz).
     * 
     * TODO: If this is too slow, use a divider value of 64 to reach 200 kHz,
     *       which is denoted as the upper limit for 10 bit ADC output.
     * 
     * Datasheet: 28.4. Prescaling and Conversion Timing, page 308:
     * “By default, the successive approximation circuitry requires an input clock frequency between 50kHz and
     * 200kHz to get maximum resolution. If a lower resolution than 10 bits is needed, the input clock frequency
     * to the ADC can be higher than 200kHz to get a higher sample rate.”
     */
    ADCSRA |= _BV(ADPS0) | _BV(ADPS1) | _BV(ADPS2)
            | _BV(ADEN)
            | _BV(ADIE);
            
    /* Datasheet: 28.9.1. ADC Multiplexer Selection Register, page 317:
     * - Use channel PORTC4 (0x04) to read the analog axis data.
     * - Do not modify the upper 3 bits REFS1, REFS0, ADLAR.
     */
    ADMUX = (ADMUX & 0xE0) | 0x04;
    
    
    /* Port C is used to read the digital buttons (pins 0-3) and the analog axis (pin 4).
     * Configure these pins as inputs. Pressing joystick buttons pulls the pin low, so enable the pull-up resistors
     * for the relevant bits.
     */
    DDRC &= ~ (_BV(DDC0) | _BV(DDC1) | _BV(DDC2) | _BV(DDC3) | _BV(DDC4));
    PORTC |= _BV(PORTC0) | _BV(PORTC1) | _BV(PORTC2) | _BV(PORTC3);
    
    /* Datasheet: 28.9.8. Digital Input Disable Register 0, page 326:
     * “When the respective bits are written to logic one, the digital input buffer on the corresponding ADC pin is
     *  disabled. The corresponding PIN Register bit will always read as zero when this bit is set. When an
     *  analog signal is applied to the ADC7...0 pin and the digital input from this pin is not needed, this bit should
     *  be written logic one to reduce power consumption in the digital input buffer.”
     * 
     * Use Port C 4 to read the analog axis signals.
     */
    DIDR0 |= _BV(ADC4D);
    
    /* Configure Port B to control the resistor battery multiplexer and the axis selection multiplexer.
     * Each is driven by three bits of Port B. So configure these pins as outputs.
     */
    PORTB &= ~ (_BV(PORTB0) | _BV(PORTB1) |  _BV(PORTB2) |  _BV(PORTB3) |  _BV(PORTB4) |  _BV(PORTB5));
    DDRB |= _BV(DDB0) | _BV(DDB1) |  _BV(DDB2) |  _BV(DDB3) |  _BV(DDB4) |  _BV(DDB5);    
    
    /* Datasheet:18.2.6. Unconnected Pins:
     * “If some pins are unused, it is recommended to ensure that these pins have a defined level. […]
     * The simplest method to ensure a defined level of an unused pin, is to enable the internal pull-up.”
     * 
     * Enable the pull-up for all unused pins.
     * 
     * TODO: Fill this section when the circuitry is fully designed and
     * configure all unused pins as input with enabled pull-up resistors.
     */
     // DDRD &= ~ (_BV(DDC1) |  _BV(DDC2) |  _BV(DDC3) |  _BV(DDC4) |  _BV(DDC5));
     // PORTD |= _BV(PORTC1) |  _BV(PORTC2) |  _BV(PORTC3) |  _BV(PORTC4) |  _BV(PORTC5);
     // The topmost two bits of port B are not used, as these pins may be used in the future to connect an external clock source,
     // if the internal 12.8MHz clock has proven to be inappropriate.
     DDRB &= ~ (_BV(DDB6) | _BV(DDB7));
     PORTB |= _BV(PORTB6) | _BV(PORTB7);
     PINB |= _BV(PINB6) | _BV(PINB7);
}
