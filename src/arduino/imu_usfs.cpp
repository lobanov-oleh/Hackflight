/*
   Copyright (c) 2022 Simon D. Levy

   This file is part of Hackflight.

   Hackflight is free software: you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation, either version 3 of the License, or (at your option)
   any later version.

   Hackflight is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY without even the implied warranty of MERCHANTABILITY or FITNESS
   FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
   details.

   You should have received a copy of the GNU General Public License along with
   Hackflight. If not, see <https://www.gnu.org/licenses/>.
 */

#include <Arduino.h>
#include <Wire.h>
#include <USFS.h>

#include <imu.h>

#include "imu_usfs.h"

static int16_t _gyroAdc[3];
static float _qw, _qx, _qy, _qz;

static volatile UsfsImu::gyroDev_t m_gyroDev;

static void UsfsImu::interruptHandler(void)
{
    m_gyroDev.gotNewData = true;
   *m_gyroDev.interruptCountPtr += 1;
   *m_gyroDev.syncTimePtr = micros();
}

bool UsfsImu::devGyroIsReady(void)
{
    bool result = false;

    if (_gotNewData) { 

        _gotNewData = false;  

        uint8_t eventStatus = usfsCheckStatus(); 

        if (usfsEventStatusIsError(eventStatus)) { 
            usfsReportError(eventStatus);
        }

        if (usfsEventStatusIsGyrometer(eventStatus)) { 
            usfsReadGyrometerRaw(_gyroAdc);
            result = true;
        }

        if (usfsEventStatusIsQuaternion(eventStatus)) { 
            usfsReadQuaternion(_qw, _qx, _qy, _qz);
        }
    } 

    return result;
}

int16_t UsfsImu::devReadRawGyro(uint8_t k)
{
    return _gyroAdc[k];
}

uint32_t UsfsImu::devGyroInterruptCount(void)
{
    return _gyroInterruptCount;
}

uint32_t UsfsImu::devGyroSyncTime(void)
{
    return _imuDevGyroSyncTime;
}


void UsfsImu::devInit(
                uint32_t * gyroSyncTimePtr, uint32_t * gyroInterruptCountPtr)
{
    Wire.setClock(400000); 
    delay(100);

    usfsLoadFirmware(); 

    usfsBegin(
            ACCEL_BANDWIDTH,
            GYRO_BANDWIDTH,
            QUAT_DIVISOR,
            MAG_RATE,
            ACCEL_RATE_TENTH,
            GYRO_RATE_TENTH,
            BARO_RATE,
            INTERRUPT_ENABLE);

    pinMode(m_interruptPin, INPUT);
    attachInterrupt(interruptPin, interruptHandler, RISING);  

    // Clear interrupts
    usfsCheckStatus();
}

void UsfsImu::getEulerAngles(
        Imu::fusion_t * fusionPrev,
        Arming * arming,
        uint32_t time,
        State * vstate) 
{
    (void)fusionPrev;
    (void)arming;
    (void)time;

    vstate->phi =
        atan2(2.0f*(_qw*_qx+_qy*_qz), _qw*_qw-_qx*_qx-_qy*_qy+_qz*_qz);
    vstate->theta = asin(2.0f*(_qx*_qz-_qw*_qy));
    vstate->psi =
        atan2(2.0f*(_qx*_qy+_qw*_qz), _qw*_qw+_qx*_qx-_qy*_qy-_qz*_qz);

    // Convert heading from [-pi,+pi] to [0,2*pi]
    if (vstate->psi < 0) {
        vstate->psi += 2*M_PI;
    }
}
