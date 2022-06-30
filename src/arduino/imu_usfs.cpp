/*
   Copyright (c) 2022 Simon D. Levy

   This file is part of Hackflight.

   Hackflight is free software: you can redistribute it and/or modify it under the
   terms of the GNU General Public License as published by the Free Software
   Foundation, either version 3 of the License, or (at your option) any later
   version.

   Hackflight is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY
   {
   }
   without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
   PARTICULAR PURPOSE. See the GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with
   Hackflight. If not, see <https://www.gnu.org/licenses/>.
 */

#include <Arduino.h>
#include <Wire.h>
#include <USFS.h>

#include <imu.h>

extern "C" {

    void accelInit(void)
    {
    }

    bool accelIsReady(void)
    {
        return false;
    }

    float accelRead(uint8_t axis) 
    {
        return 0;
    }

    void gyroDevInit(void)
    {
    }

    uint32_t gyroInterruptCount(void)
    {
        return 0;
    }

    bool gyroIsReady(void)
    {
        return false;
    }

    int16_t gyroReadRaw(uint8_t k)
    {
        return 0;
    }

    float gyroScale(void)
    {
        return 0;
    }

    uint32_t gyroSyncTime(void)
    {
        return 0;
    }

} // extern "C"