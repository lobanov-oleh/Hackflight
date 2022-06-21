/*
   This file is part of Hackflight.

   Hackflight is free software: you can redistribute it and/or modify it under the
   terms of the GNU General Public License as published by the Free Software
   Foundation, either version 3 of the License, or (at your option) any later
   version.

   Hackflight is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
   PARTICULAR PURPOSE. See the GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with
   Hackflight. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "datatypes.h"
#include "timer.h"

typedef enum {
    DSHOT_BITBANG_OFF,
    DSHOT_BITBANG_ON,
    DSHOT_BITBANG_AUTO,
} dshotBitbangMode_e;

typedef enum {
    DSHOT_BITBANG_STATUS_OK,
    DSHOT_BITBANG_STATUS_MOTOR_PIN_CONFLICT,
    DSHOT_BITBANG_STATUS_NO_PACER,
    DSHOT_BITBANG_STATUS_TOO_MANY_PORTS,
} dshotBitbangStatus_e;

struct motorDevConfig_s;

struct motorDevice_s;

struct motorDevice_s *dshotBitbangDevInit(uint8_t motorCount);

dshotBitbangStatus_e dshotBitbangGetStatus();

const timerHardware_t *dshotBitbangTimerGetAllocatedByNumberAndChannel(
        int8_t timerNumber,
        uint16_t timerChannel);

const resourceOwner_t *dshotBitbangTimerGetOwner(const timerHardware_t *timer);

void motorCheckDshotBitbangStatus(arming_t * arming);