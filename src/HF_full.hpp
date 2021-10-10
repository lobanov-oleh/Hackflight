/*
   Hackflight class supporting safety checks and serial communication

   Copyright (c) 2021 Simon D. Levy

   MIT License
 */

#pragma once

#include "HF_pure.hpp"
#include "HF_serial.hpp"

#include "stream_receiver.h"

namespace hf {

    class HackflightFull : public HackflightPure {

        private:

            void checkSafety(State * state, float * motorvals)
            {
                // Safety
                static bool _safeToArm;

                // Sync failsafe to open-loop-controller
                if (stream_receiverLostSignal && state->armed) {
                    cutMotors(motorvals);
                    state->armed = false;
                    state->failsafe = true;
                    return;
                }

                // Disarm
                if (state->armed && !_receiver->inArmedState()) {
                    state->armed = false;
                } 

                // Avoid arming when controller is in armed state
                if (!_safeToArm) {
                    _safeToArm = !_receiver->inArmedState();
                }

                // Arm after lots of safety checks
                if (_safeToArm
                    && !state->armed
                    && !state->failsafe 
                    && state->safeToArm()
                    && _receiver->inactive()
                    && _receiver->inArmedState()
                    ) {
                    state->armed = true;
                }

                // Cut motors on inactivity
                if (state->armed && _receiver->inactive()) {
                    cutMotors(motorvals);
                }

            } // checkSafety

            void cutMotors(float * motorvals)
            {
                memset(motorvals, 0, 4*sizeof(float));  // XXX Support other than 4
            }

        public:

            HackflightFull(Receiver * receiver, Mixer * mixer)
                : HackflightPure(receiver, mixer)
            {
            }

            void begin(void)
            {  
                _state.armed = false;

            } // begin

            void update(
                    uint32_t time_usec,
                    float * motorvals,
                    bool * led,
                    SerialTask * serialTask)
            {
                HackflightPure::update(time_usec, motorvals);

                // Update serial task
                serialTask->update(time_usec, &_state, _mixer, motorvals);

                checkSafety(&_state, motorvals);

                *led = time_usec < 2000000 ? (time_usec / 50000) % 2 == 0 : _state.armed;
            }

    }; // class HackflightFull

} // namespace
