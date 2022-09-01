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

#include <escs/dshot_protocol.h>
#include <time.h>

#include "bitbang.h"
#include "dshot_dev.h"
#include "timer.h"

// Max direct dshot port groups, limited by number of usable timer (TIM1 and
// TIM8) x number of channels per timer (4), 3 is enough to cover motor pins on
// GPIOA, GPIOB and GPIOC.
#define MAX_SUPPORTED_MOTOR_PORTS 4 

#define DSHOT_BITBANG_TELEMETRY_OVER_SAMPLE 3

#define MOTOR_DSHOT_SYMBOL_TIME_NS(rate)  (1000000000 / (rate))

#define MOTOR_DSHOT_BIT_PER_SYMBOL         1

#define MOTOR_DSHOT_STATE_PER_SYMBOL       3  // Initial high, 0/1, low

#define MOTOR_DSHOT_FRAME_BITS             16

#define MOTOR_DSHOT_FRAME_TIME_NS(rate)    ((MOTOR_DSHOT_FRAME_BITS / MOTOR_DSHOT_BIT_PER_SYMBOL) * MOTOR_DSHOT_SYMBOL_TIME_NS(rate))

#define MOTOR_DSHOT_TELEMETRY_WINDOW_US    (30000 + MOTOR_DSHOT_FRAME_TIME_NS(rate) * (1.1)) / 1000

#define MOTOR_DSHOT_CHANGE_INTERVAL_NS(rate) (MOTOR_DSHOT_SYMBOL_TIME_NS(rate) / MOTOR_DSHOT_STATE_PER_SYMBOL)

#define MOTOR_DSHOT_GCR_CHANGE_INTERVAL_NS(rate) (MOTOR_DSHOT_CHANGE_INTERVAL_NS(rate) * 5 / 4)

#define MOTOR_DSHOT_BUF_LENGTH            ((MOTOR_DSHOT_FRAME_BITS / MOTOR_DSHOT_BIT_PER_SYMBOL) * MOTOR_DSHOT_STATE_PER_SYMBOL)

#define MOTOR_DSHOT_BUF_CACHE_ALIGN_LENGTH MOTOR_DSHOT_BUF_LENGTH

#define BB_GPIO_PULLDOWN GPIO_PuPd_DOWN
#define BB_GPIO_PULLUP   GPIO_PuPd_UP


// DMA input buffer
// (30us + <frame time> + <slack>) / <input sampling clock period>
// <frame time> = <DShot symbol time> * 16
// Temporary size for DS600
// <frame time> = 26us
// <sampling period> = 0.44us
// <slack> = 10%
// (30 + 26 + 3) / 0.44 = 134
// In some cases this was not enough, so we add 6 extra samples
#define DSHOT_BB_PORT_IP_BUF_LENGTH 140
#define DSHOT_BB_PORT_IP_BUF_CACHE_ALIGN_LENGTH DSHOT_BB_PORT_IP_BUF_LENGTH

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

typedef struct bbMotor_s {
    dshotProtocolControl_t protocolControl;
    int pinIndex;    // pinIndex of this motor output within a group that bbPort points to
    int portIndex;
    IO_t io;         // IO_t for this output
    uint8_t output;
    uint32_t iocfg;
    bbPort_t *bbPort;
    bool configured;
    bool enabled;
} bbMotor_t;

#if defined (__cplusplus)
extern "C" {
#endif

bool bbEnableMotors(void);
void bbPostInit(dshotProtocol_t protocol);
void bbUpdateComplete(uint8_t motorCount);
bool bbUpdateStart(void);
void bbWrite(uint8_t motorIndex, float value);

void dshotBitbangDevInit(const uint8_t pins[], const uint8_t count);

dshotBitbangStatus_e dshotBitbangGetStatus();

const timerHardware_t *dshotBitbangTimerGetAllocatedByNumberAndChannel(
        int8_t timerNumber,
        uint16_t timerChannel);

const resourceOwner_t *dshotBitbangTimerGetOwner(const timerHardware_t *timer);

#if defined (__cplusplus)
}
#endif


