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

#include <stdlib.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "usbdrv.h"

#include "hwinit.h"
#include "joystick.h"

extern struct joystick_read_t joystick_read_result;

uint8_t idleRate;   /* repeat rate for keyboards, never used for mice/joysticks */

usbMsgLen_t usbFunctionSetup(uint8_t data[8])
{
    usbRequest_t *rq = (void *)data;

    /* The following requests are never used. But since they are required by
     * the specification, these are implemented regardless.
     */
    if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS){    /* class request type */
        if(rq->bRequest == USBRQ_HID_GET_REPORT){  /* wValue: ReportType (highbyte), ReportID (lowbyte) */
            /* we only have one report type, so don't look at wValue */
            usbMsgPtr = (unsigned short) &joystick_read_result;
            return sizeof(joystick_read_result);
        }else if(rq->bRequest == USBRQ_HID_GET_IDLE){
            usbMsgPtr = (unsigned short) &idleRate;
            return 1;
        }else if(rq->bRequest == USBRQ_HID_SET_IDLE){
            idleRate = rq->wValue.bytes[1];
        }
    } else {
        /* no vendor specific requests implemented */
    }
    return 0;   /* default for not implemented requests: return no data back to host */
}


int main() {
    hwinit();
    usbInit();
    //usbDeviceDisconnect();
    _delay_ms(500);
    usbDeviceConnect();
    sei();
    for(;;) {
        usbPoll();
        if(usbInterruptIsReady()) {
            read_joystick();
            usbSetInterrupt((void *) &joystick_read_result, sizeof(joystick_read_result));
        }
    }
}




