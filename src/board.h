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
#include <stdarg.h>

#include <vector>

#include "core.h"
#include "core/mixer.h"
#include "core/motors.h"
#include "esc.h"
#include "esc/mock.h"
#include "imu.h"
#include "task/accelerometer.h"
#include "task/attitude.h"
#include "task/visualizer.h"
#include "task/receiver.h"

class Board {

    private:

        static constexpr float MAX_ARMING_ANGLE_DEG = 25;

        static const uint8_t  STARTUP_BLINK_LED_REPS  = 10;
        static const uint32_t STARTUP_BLINK_LED_DELAY = 50;

        Core::armingStatus_e m_armingStatus;

        uint8_t m_ledPin;
        bool m_ledInverted;

        uint32_t m_imuInterruptCount;

        VehicleState m_vstate;

        AttitudeTask m_attitudeTask = AttitudeTask(m_vstate);

        ReceiverTask m_receiverTask;

        VisualizerTask m_visualizerTask =
            VisualizerTask(m_msp, m_vstate, m_receiverTask, m_skyrangerTask);

        Msp m_msp;

        Core m_core;

        // Initialzed in sketch
        Esc *   m_esc;
        Mixer * m_mixer;
        std::vector<PidController *> * m_pidControllers;

        bool m_dshotEnabled;

    protected:

        Board(
                Imu * imu,
                std::vector<PidController *> & pidControllers,
                Mixer & mixer,
                Esc & esc,
                const int8_t ledPin)
        {
            m_imu = imu;
            m_pidControllers = &pidControllers;
            m_mixer = &mixer;
            m_esc = &esc;

            // Support negative LED pin number for inversion
            m_ledPin = ledPin < 0 ? -ledPin : ledPin;
            m_ledInverted = ledPin < 0;
        }


    private:

        void runDynamicTasks(const int16_t rawAccel[3])
        {
            if (m_visualizerTask.gotRebootRequest()) {
                reboot();
            }

            Task::prioritizer_t prioritizer = {Task::NONE, 0};

            const uint32_t usec = micros(); 

            m_receiverTask.prioritize(usec, prioritizer);
            m_attitudeTask.prioritize(usec, prioritizer);
            m_visualizerTask.prioritize(usec, prioritizer);

            prioritizeExtraTasks(prioritizer, usec);

            switch (prioritizer.id) {

                case Task::ATTITUDE:
                    runTask(m_attitudeTask);
                    updateArmingStatus(usec);
                    updateLed();
                    break;

                case Task::VISUALIZER:
                    runVisualizerTask();
                    break;

                case Task::RECEIVER:
                    updateArmingStatus(usec);
                    updateLed();
                    runTask(m_receiverTask);
                    break;

                case Task::ACCELEROMETER:
                    runTask(m_accelerometerTask);
                    m_imu->updateAccelerometer(rawAccel);
                    break;

                case Task::SKYRANGER:
                    runTask(m_skyrangerTask);
                    break;

                default:
                    break;
            }
        }

        void runTask(Task & task)
        {
            const uint32_t anticipatedEndCycles = getAnticipatedEndCycles(task);

            if (anticipatedEndCycles > 0) {

                const uint32_t usec = micros();

                task.run(usec);

                postRunTask(task, usec, anticipatedEndCycles);
            } 
        }

        void postRunTask(
                Task & task,
                const uint32_t usecStart,
                const uint32_t anticipatedEndCycles)
        {
            m_core.postRunTask(
                    task, usecStart, micros(), getCycleCounter(), anticipatedEndCycles);
        }

        void updateLed(void)
        {
            switch (m_armingStatus) {

                case Core::ARMING_UNREADY:
                    ledBlink(500);
                    break;

                case Core::ARMING_READY:
                    ledSet(false);
                    break;

                case Core::ARMING_ARMED:
                    ledSet(true);
                    break;

                default: // failsafe
                    ledBlink(200);
                    break;
             }
        }

        void updateArmingStatus(const uint32_t usec)
        {
            checkFailsafe(usec);

            switch (m_armingStatus) {

                case Core::ARMING_UNREADY:
                    if (safeToArm(usec)) {
                        m_armingStatus = Core::ARMING_READY;
                    }
                    break;

                case Core::ARMING_READY:
                    if (safeToArm(usec)) {
                        checkArmingSwitch();
                    }
                    else {
                        m_armingStatus = Core::ARMING_UNREADY;
                    }
                    break;

                case Core::ARMING_ARMED:
                    checkArmingSwitch();
                    break;

                default: // failsafe
                    break;
            }
        }

        void checkFailsafe(const uint32_t usec)
        {
            static bool hadSignal;

            const auto haveSignal = m_receiverTask.haveSignal(usec);

            if (haveSignal) {
                hadSignal = true;
            }

            if (hadSignal && !haveSignal) {
                m_armingStatus = Core::ARMING_FAILSAFE;
            }
        }

        bool safeToArm(const uint32_t usec)
        {
            const auto maxArmingAngle = Imu::deg2rad(MAX_ARMING_ANGLE_DEG);

            const auto imuIsLevel =
                fabsf(m_vstate.phi) < maxArmingAngle &&
                fabsf(m_vstate.theta) < maxArmingAngle;

            const auto gyroDoneCalibrating = !m_imu->gyroIsCalibrating();

            const auto haveReceiverSignal = m_receiverTask.haveSignal(usec);

            return
                gyroDoneCalibrating &&
                imuIsLevel &&
                m_receiverTask.throttleIsDown() &&
                haveReceiverSignal;
        }

        void checkArmingSwitch(void)
        {
            static bool aux1WasSet;

            if (m_receiverTask.getRawAux1() > 1500) {
                if (!aux1WasSet) {
                    m_armingStatus = Core::ARMING_ARMED;
                }
                aux1WasSet = true;
            }
            else {
                if (aux1WasSet) {
                    m_armingStatus = Core::ARMING_READY;
                }
                aux1WasSet = false;
            }
        }

        void ledBlink(const uint32_t msecDelay)
        {
            static bool ledPrev;
            static uint32_t msecPrev;
            const uint32_t msecCurr = millis();

            if (msecCurr - msecPrev > msecDelay) {
                ledPrev = !ledPrev;
                ledSet(ledPrev);
                msecPrev = msecCurr;
            }
        }


        void ledSet(bool on)
        {
            digitalWrite(m_ledPin, m_ledInverted ? on : !on);
        }

        // STM32F boards have no auto-reset bootloader support, so we reboot on
        // an external input
        virtual void reboot(void)
        {
        }

        static void outbuf(char * buf)
        {
            Serial.print(buf); 
            Serial.flush();
        }

        void runVisualizerTask(void)
        {
            const uint32_t anticipatedEndCycles =
                getAnticipatedEndCycles(m_visualizerTask);

            if (anticipatedEndCycles > 0) {

                const auto usec = micros();

                while (Serial.available()) {

                    if (m_visualizerTask.parse(Serial.read())) {
                        Serial.write(m_msp.payload, m_msp.payloadSize);
                    }
                }

                postRunTask(m_visualizerTask, usec, anticipatedEndCycles);
            }
        }

        uint32_t getAnticipatedEndCycles(Task & task)
        {
            return m_core.getAnticipatedEndCycles(task, getCycleCounter());
        }

        void escBegin(void)
        {
            switch (m_esc->type) {

                case Esc::BRUSHED:
                    for (auto pin : *m_esc->motorPins) {
                        // analogWriteFrequency(pin, 10000);
                        analogWrite(pin, 0);
                    }
                    break;

                case Esc::DSHOT:
                    dmaInit(m_esc->motorPins, 1000 * m_esc->dshotOutputFreq);
                    m_dshotEnabled = true;
                    break;

                default:
                    break;
            }
        }

        void dshotWrite(const float motorValues[])
        {
            dmaUpdateStart();

            for (uint8_t k=0; k<m_esc->motorPins->size(); k++) {

                auto packet = m_esc->prepareDshotPacket(k, motorValues[k]);

                dmaWriteMotor(k, packet);
            }

            m_esc->dshotComplete();

            dmaUpdateComplete();
        }

        void escWrite(const float motorValues[])
        {
            switch (m_esc->type) {

                case Esc::BRUSHED:
                    for (uint8_t k=0; k<m_esc->motorPins->size(); ++k) {
                        analogWrite((*m_esc->motorPins)[k], (uint8_t)(motorValues[k] * 255));
                    }
                    break;

                case Esc::DSHOT:
                    if (m_dshotEnabled) {
                        dshotWrite(motorValues);
                    }
                    break;

                default:
                    break;
            }
        }

    protected:

        // Initialized in sketch
        Imu * m_imu;

        AccelerometerTask m_accelerometerTask; 

        SkyrangerTask m_skyrangerTask = SkyrangerTask(m_vstate);

        virtual void prioritizeExtraTasks(
                Task::prioritizer_t & prioritizer, const uint32_t usec)
        {
            (void)prioritizer;
            (void)usec;
        }

    public:

        void setSbusValues(uint16_t chanvals[], const uint32_t usec)
        {
            m_receiverTask.setValues(chanvals, usec, 172, 1811);
        }

        void setDsmxValues(uint16_t chanvals[], const uint32_t usec)
        {
            m_receiverTask.setValues(chanvals, usec, 988, 2011);
        }

        void handleImuInterrupt(void)
        {
            m_imuInterruptCount++;
            m_imu->handleInterrupt(getCycleCounter());
        }

        uint32_t microsToCycles(uint32_t micros)
        {
            return getClockSpeed() / 1000000 * micros;
        }

        virtual uint32_t getClockSpeed(void)  = 0;

        virtual uint32_t getCycleCounter(void) = 0;

        virtual void startCycleCounter(void) = 0;

        virtual void dmaInit(
                const std::vector<uint8_t> * motorPins, const uint32_t dshotOutputFreq)
        {
            (void)motorPins;
            (void)dshotOutputFreq;
        }

        virtual void dmaUpdateComplete(void)
        {
        }

        virtual void dmaUpdateStart(void)
        {
        }

        virtual void dmaWriteMotor(uint8_t index, uint16_t packet)
        {
            (void)index;
            (void)packet;
        }

        void begin(void)
        {
            startCycleCounter();

            m_attitudeTask.begin(m_imu);

            m_visualizerTask.begin(m_esc, &m_receiverTask);

            m_imu->begin(getClockSpeed());

            escBegin();

            pinMode(m_ledPin, OUTPUT);

            ledSet(false);
            for (auto i=0; i<10; i++) {
                static bool ledOn;
                ledOn = !ledOn;
                ledSet(ledOn);
                delay(50);
            }
            ledSet(false);
        }

        void step(int16_t rawGyro[3], int16_t rawAccel[3])
        {
            auto nowCycles = getCycleCounter();

            if (m_core.isCoreTaskReady(nowCycles)) {

                const uint32_t usec = micros();

                int32_t loopRemainingCycles = 0;

                const uint32_t nextTargetCycles =
                    m_core.coreTaskPreUpdate(loopRemainingCycles);

                while (loopRemainingCycles > 0) {
                    nowCycles = getCycleCounter();
                    loopRemainingCycles = intcmp(nextTargetCycles, nowCycles);
                }

                float mixmotors[Motors::MAX_SUPPORTED] = {};

                m_core.getMotorValues(
                        rawGyro,
                        m_imu,
                        m_vstate,
                        m_receiverTask,
                        m_pidControllers,
                        m_mixer,
                        m_esc,
                        usec,
                        mixmotors);

                escWrite(
                        m_armingStatus == Core::ARMING_ARMED ? 
                        mixmotors :
                        m_visualizerTask.motors);

                m_core.updateScheduler(
                        m_imu, m_imuInterruptCount, nowCycles, nextTargetCycles);
            }

            if (m_core.isDynamicTaskReady(getCycleCounter())) {
                runDynamicTasks(rawAccel);
            }
        }

        void step(int16_t rawGyro[3], int16_t rawAccel[3], HardwareSerial & serial)
        {
            step(rawGyro, rawAccel);

            while (m_skyrangerTask.imuDataAvailable()) {
                serial.write(m_skyrangerTask.readImuData());
            }
        }

        static void setInterrupt(
                const uint8_t pin, void (*irq)(void), const uint32_t mode)
        {
            pinMode(pin, INPUT);
            attachInterrupt(pin, irq, mode);  
        }

        static void printf(const char * fmt, ...)
        {
            va_list ap;
            va_start(ap, fmt);
            char buf[200];
            vsnprintf(buf, 200, fmt, ap); 
            outbuf(buf);
            va_end(ap);
        }

        static void reportForever(const char * fmt, ...)
        {
            va_list ap;
            va_start(ap, fmt);
            char buf[200];
            vsnprintf(buf, 200, fmt, ap); 
            va_end(ap);

            strcat(buf, "\n");

            while (true) {
                outbuf(buf);
                delay(500);
            }
        }

}; // class Board
