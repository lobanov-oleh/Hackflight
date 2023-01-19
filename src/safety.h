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

#include <stdint.h>
#include <stdbool.h>

#include "core/vstate.h"
#include "esc.h"
#include "imu.h"
#include "receiver.h"

class Safety {

    private:

        static constexpr float MAX_ARMING_ANGLE = 25;

        // Avoid repeated degrees-to-radians conversion
        const float maxAngle = Imu::deg2rad(MAX_ARMING_ANGLE);

        bool m_accDoneCalibrating;
        bool m_angleOkay;
        bool m_gotFailsafe;
        bool m_gyroDoneCalibrating;
        bool m_isArmed;
        bool m_ledOn;
        bool m_haveSignal;
        bool m_switchOkay;
        bool m_throttleIsDown;

        uint32_t m_timer;

        void disarm(Esc * esc)
        {
            if (m_isArmed) {
                esc->stop();
            }
            m_isArmed = false;
        }

        bool isReady(void)
        {
            return 
                m_accDoneCalibrating &&
                m_angleOkay &&
                !m_gotFailsafe &&
                m_haveSignal &&
                m_gyroDoneCalibrating &&
                m_switchOkay &&
                m_throttleIsDown;
        }

    public:

        static const uint8_t  STARTUP_BLINK_LED_REPS  = 10;
        static const uint32_t STARTUP_BLINK_LED_DELAY = 50;

        typedef enum {

            LED_UNCHANGED,
            LED_TURN_ON,
            LED_TURN_OFF

        } ledChange_e;

        void attemptToArm(Receiver & receiver, Esc * esc, const uint32_t usec)
        {
            static bool _doNotRepeat;

            if (receiver.aux1IsSet()) {

                if (isReady()) {

                    if (m_isArmed) {
                        return;
                    }

                    if (!esc->isReady(usec)) {
                        return;
                    }

                    m_isArmed = true;
                }

            } else {

                disarm(esc);
            }

            if (!(m_isArmed || _doNotRepeat || !isReady())) {
                _doNotRepeat = true;
            }
        }

        bool isArmed(void)
        {
            return m_isArmed;
        }

        void updateFromImu(Imu & imu, VehicleState & vstate)
        {
            const auto imuIsLevel =
                fabsf(vstate.phi) < maxAngle && fabsf(vstate.theta) < maxAngle;

            m_angleOkay = imuIsLevel;

            m_gyroDoneCalibrating = !imu.gyroIsCalibrating();

            m_accDoneCalibrating = true; // XXX
        }

        auto updateFromReceiver(
                Receiver * receiver,
                Esc * esc,
                const uint32_t usec) -> ledChange_e
        {
            ledChange_e ledChange = LED_UNCHANGED;

            if (receiver->getState() == Receiver::STATE_UPDATE) {
                attemptToArm(*receiver, esc, usec);
            }

            else  if (receiver->getState() == Receiver::STATE_CHECK) {

                if (isArmed()) {

                    if (!receiver->hasSignal() && m_haveSignal) {
                        m_gotFailsafe = true;
                        disarm(esc);
                    }
                    else {
                        ledChange = LED_TURN_ON;
                    }
                } 
                else {

                    m_throttleIsDown = receiver->throttleIsDown();

                    // If arming is disabled and the ARM switch is on
                    if (!isReady() && receiver->aux1IsSet()) {
                        m_switchOkay = false;
                    } else if (!receiver->aux1IsSet()) {
                        m_switchOkay = true;
                    }

                    if ((int32_t)(usec - m_timer) < 0) {
                        return ledChange;
                    }

                    if (isReady()) {
                        ledChange = LED_TURN_OFF;
                    }
                    else {
                        m_ledOn = !m_ledOn;
                        ledChange = m_ledOn ? LED_TURN_ON : LED_TURN_OFF;
                    }

                    m_timer = usec + 500000;
                }

                m_haveSignal = receiver->hasSignal();
            }

            return ledChange;
        }

}; // class Safety
