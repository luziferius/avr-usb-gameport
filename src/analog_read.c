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

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>

#include "analog_read.h"

union adc_result_t {
    uint16_t result;
    uint8_t bytes[2];
};


ISR(ADC_vect) {
    /*
     * Nothing to do here.
     * The only purpose is to implicitly clear the interrupt flags in SREG and ADCSRA,
     * and to wake up the CPU that sleeps during the conversion.
     */
}

void set_channel(const uint8_t channel) {
    /* Datasheet: 28.9.1. ADC Multiplexer Selection Register, page 317:
     * - Only allow the plain 8 ADC channels.
     * - Make sure that the channel selection can not write the upper 3 bits (bits 5, 6 & 7).
     * - Do not reset the upper 3 bits REFS1, REFS0, ADLAR.
     */
    ADMUX = (channel & 0x7) | (ADMUX & 0xE0);
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

uint16_t analog_read4() {
    // Compute the arithmetic mean of 4 results.
    // Each individual component has 10 bit accuracy,
    // the sum therefore uses at most 12 bits, which fits into a single uint16_t.
    uint16_t result = analog_read();
    result += analog_read();
    result += analog_read();
    result += analog_read();
    result >>= 2;
    return result;
}
