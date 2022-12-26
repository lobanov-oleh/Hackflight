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

#pragma once

#include <stdint.h>

#include "msp.h"

class UsbMsp : public Msp {

    protected:

        virtual void write(const uint8_t buf[], const uint8_t size) override
        {
            Serial.write(buf, size);
        }

    public:

        virtual uint32_t available(void) override
        {
            return Serial.available();
        }

        virtual void begin(void) override
        {
            Serial.begin(115200);
        }

        virtual uint8_t read(void) override
        {
            return Serial.read();
        }

};
