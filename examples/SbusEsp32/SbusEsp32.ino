/*
   Copyright (c) 2022 Simon D. Levy

   This file is part of Hackflight.

   Hackflight is free software: you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation, either version 3 of the License, or (at your option)
   any later version.

   Hackflight is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
   more details.

   You should have received a copy of the GNU General Public License along with
   Hackflight. If not, see <https://www.gnu.org/licenses/>.
 */

#include <hackflight.h>
#include <tasks/receivers/sbus.h>

static const uint8_t RX_PIN = 4;
static const uint8_t TX_PIN = 14; // unused

static SbusReceiver _rx;

static uint16_t convert(uint16_t chanval)
{
    return (uint16_t)SbusReceiver::convert(chanval);
}

static void report (
        const uint16_t value, const char * label, const char * delim="   ")
{
    Serial.print(label);
    Serial.print(value);
    Serial.print(delim);
}

void setup(void)
{
    Serial.begin(115200);

    Serial1.begin(100000, SERIAL_8E2, RX_PIN, TX_PIN, true);
}

void loop(void)
{
    _rx.read(Serial1);

    if (_rx.ready()) {

        const uint16_t c1 = convert(_rx.readChannel1());
        const uint16_t c2 = convert(_rx.readChannel2());
        const uint16_t c3 = convert(_rx.readChannel3());
        const uint16_t c4 = convert(_rx.readChannel4());
        
        report(c1, "C1=");
        report(c2, "C2=");
        report(c3, "C3=");
        report(c4, "C4=", "\n");
    }

    delay(5);
}
