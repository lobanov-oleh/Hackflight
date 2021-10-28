/*
   Copyright (C) 2021 Simon D. Levy

   MIT License

 */

#include "serial.hpp"
#include "debugger.hpp"
#include "stream_serial.h"
#include "stream_receiver.h"
#include "copilot.h"

void setup(void)
{
    Serial1.begin(115200);

    for (uint8_t k=0; k<10; ++k) {
        Serial1.println();
    }
}

void loop(void)
{
    step();
}

void stream_run(
        uint8_t byteval,
        uint8_t msgtype,
        bool sending,
        uint8_t payload_index)
{
    if (receiving) {
        Debugger::printf(Serial1, "byte=%d msgtype=%d\n", byteval, msgtype);
    }

    static float state_phi = -0.1;
    float state_theta = 0.2;
    static float state_psi;

    if (sending) {
        handleSerialInput(msgtype, state_phi, state_theta, state_psi);
    }

    updateSerialOutput();

    delay(10);
}
