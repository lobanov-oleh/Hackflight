/*
   Copyright (c) 2022 Simon D. Levy

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

#include <string.h>

#include "esc.h"
#include "task.h"
#include "../receiver.h"

class ReceiverTask : public Task {

    private:

        Receiver * m_receiver;
        Esc *      m_esc;
        Arming *   m_arming;

        bool m_gotPidReset;

    public:

        ReceiverTask()
            : Task(33) // Hz
        {
        }

        void begin(Receiver * receiver, Esc * esc, Arming * arming)
        {
            m_receiver = receiver;
            m_esc = esc;
            m_arming = arming;

            m_receiver->begin(esc);
        }

        bool gotPidReset(void)
        {
            return m_gotPidReset;
        }

        // Increase priority for RX task
        void adjustDynamicPriority(Task::data_t *data, uint32_t usec) 
        {
            (void)data;

            if (m_dynamicPriority > 0) {
                m_ageCycles = 1 + (cmpTimeUs(usec,
                            m_lastSignaledAtUs) / m_desiredPeriodUs);
                m_dynamicPriority = 1 + m_ageCycles;
            } else  {
                if (m_receiver->check(usec)) {
                    m_lastSignaledAtUs = usec;
                    m_ageCycles = 1;
                    m_dynamicPriority = 2;
                } else {
                    m_ageCycles = 0;
                }
            }
        }    
        
        void fun(Task::data_t * data, uint32_t usec)
        {
            auto pidItermResetReady = false;
            auto pidItermResetValue = false;

            Receiver::sticks_t rxsticks = {{0, 0, 0, 0}, 0, 0};

            auto gotNewData = false;

            Receiver::state_e receiverState = m_receiver->poll(usec, &rxsticks);

            switch (receiverState) {

                case Receiver::STATE_PROCESS:
                    pidItermResetReady = true;
                    pidItermResetValue = m_receiver->processData(usec);
                    break;
                case Receiver::STATE_MODES:
                    m_arming->check(m_esc, usec, rxsticks);
                    break;
                case Receiver::STATE_UPDATE:
                    gotNewData = true;
                    m_arming->updateReceiverStatus(rxsticks);
                    break;
                default:
                    break;
            }

            if (pidItermResetReady) {
                m_gotPidReset = pidItermResetValue;
            }

            if (gotNewData) {
                data->rxSticks.demands.throttle = rxsticks.demands.throttle;
                data->rxSticks.demands.roll = rxsticks.demands.roll;
                data->rxSticks.demands.pitch = rxsticks.demands.pitch;
                data->rxSticks.demands.yaw = rxsticks.demands.yaw;
                data->rxSticks.aux1 = rxsticks.aux1;
                data->rxSticks.aux2 = rxsticks.aux2;
            }
        }
};
