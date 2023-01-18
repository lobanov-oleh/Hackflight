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
using namespace std;

#include "core/mixer.h"
#include "core/motors.h"
#include "esc.h"
#include "imu.h"
#include "receiver.h"
#include "scheduler.h"
#include "task/attitude.h"
#include "task/visualizer.h"
#include "task/receiver.h"

class Board {

    private:

        static constexpr float MAX_ARMING_ANGLE = 25;

        // Avoid repeated degrees-to-radians conversion
        float m_maxArmingAngle = Imu::deg2rad(MAX_ARMING_ANGLE);

        // Gyro interrupt counts over which to measure loop time and skew
        static const uint32_t CORE_RATE_COUNT = 25000;
        static const uint32_t GYRO_LOCK_COUNT = 400;

        // Motor safety
        bool m_failsafeIsActive;

        // Arming guards
        bool m_accDoneCalibrating;
        bool m_angleOkay;
        bool m_gotFailsafe;
        bool m_gyroDoneCalibrating;
        bool m_haveSignal;
        bool m_isArmed;
        bool m_switchOkay;
        bool m_throttleIsDown;

        // LED
        uint8_t m_ledPin;
        bool m_ledInverted;

        VehicleState m_vstate;

        AttitudeTask m_attitudeTask = AttitudeTask(m_vstate);

        ReceiverTask m_receiverTask;

        Msp m_msp;

        // Initialzed in sketch()
        Esc *   m_esc;
        Mixer * m_mixer;
        vector<PidController *> * m_pidControllers;

        void checkCoreTasks(uint32_t nowCycles)
        {
            int32_t loopRemainingCycles = m_scheduler.getLoopRemainingCycles();
            uint32_t nextTargetCycles = m_scheduler.getNextTargetCycles();

            m_scheduler.corePreUpdate();

            while (loopRemainingCycles > 0) {
                nowCycles = getCycleCounter();
                loopRemainingCycles =
                    intcmp(nextTargetCycles, nowCycles);
            }

            if (m_imu->gyroIsReady()) {

                auto angvels = m_imu->readGyroDps();

                m_vstate.dphi   = angvels.x;
                m_vstate.dtheta = angvels.y;
                m_vstate.dpsi   = angvels.z;
            }

            Demands demands = m_receiverTask.receiver->getDemands();

            delayMicroseconds(10);

            auto motors = m_mixer->step(
                    demands,
                    m_vstate,
                    m_pidControllers,
                    m_receiverTask.receiver->gotPidReset(),
                    micros());

            float mixmotors[Motors::MAX_SUPPORTED] = {};

            for (auto i=0; i<m_mixer->getMotorCount(); i++) {

                mixmotors[i] = m_esc->getMotorValue(motors.values[i], m_failsafeIsActive);
            }

            m_esc->write(isArmed() ?  mixmotors : m_visualizerTask.motors);

            m_scheduler.corePostUpdate(nowCycles);

            // Bring the scheduler into lock with the gyro Track the actual
            // gyro rate over given number of cycle times and set the expected
            // timebase
            static uint32_t _terminalGyroRateCount;
            static int32_t _sampleRateStartCycles;

            if ((_terminalGyroRateCount == 0)) {
                _terminalGyroRateCount = m_imu->getGyroInterruptCount() + CORE_RATE_COUNT;
                _sampleRateStartCycles = nowCycles;
            }

            if (m_imu->getGyroInterruptCount() >= _terminalGyroRateCount) {
                // Calculate number of clock cycles on average between gyro
                // interrupts
                uint32_t sampleCycles = nowCycles - _sampleRateStartCycles;
                m_scheduler.desiredPeriodCycles = sampleCycles / CORE_RATE_COUNT;
                _sampleRateStartCycles = nowCycles;
                _terminalGyroRateCount += CORE_RATE_COUNT;
            }

            // Track actual gyro rate over given number of cycle times and
            // remove skew
            static uint32_t _terminalGyroLockCount;
            static int32_t _gyroSkewAccum;

            auto gyroSkew =
                m_imu->getGyroSkew(nextTargetCycles, m_scheduler.desiredPeriodCycles);

            _gyroSkewAccum += gyroSkew;

            if ((_terminalGyroLockCount == 0)) {
                _terminalGyroLockCount = m_imu->getGyroInterruptCount() + GYRO_LOCK_COUNT;
            }

            if (m_imu->getGyroInterruptCount() >= _terminalGyroLockCount) {
                _terminalGyroLockCount += GYRO_LOCK_COUNT;

                // Move the desired start time of the gyroSampleTask
                m_scheduler.lastTargetCycles -= (_gyroSkewAccum/GYRO_LOCK_COUNT);

                _gyroSkewAccum = 0;
            }

        } // checkCoreTasks

        static void outbuf(char * buf)
        {
            Serial.print(buf);
            Serial.flush();
        }

        bool readyToArm(void)
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

        void disarm(void)
        {
            if (m_isArmed) {
                m_esc->stop();
            }

            m_isArmed = false;
        }

        void updateArmingFromImu(const bool imuIsLevel, const bool gyroIsCalibrating)
        {
            m_angleOkay = imuIsLevel;

            m_gyroDoneCalibrating = !gyroIsCalibrating;

            m_accDoneCalibrating = true; // XXX
        }

        bool isArmed(void)
        {
            return m_isArmed;
        }

        void attemptToArm(const uint32_t usec, const bool aux1IsSet)
        {
            static bool _doNotRepeat;

            if (aux1IsSet) {

                if (readyToArm()) {

                    if (m_isArmed) {
                        return;
                    }

                    if (!m_esc->isReady(usec)) {
                        return;
                    }

                    m_isArmed = true;
                }

            } else {

                if (m_isArmed) {
                    disarm();
                    m_isArmed = false;
                }
            }

            if (!(m_isArmed || _doNotRepeat || !readyToArm())) {
                _doNotRepeat = true;
            }
        }

        void updateFromReceiver(
                const bool throttleIsDown, const bool aux1IsSet, const bool haveSignal)
        {
            if (m_isArmed) {

                if (!haveSignal && m_haveSignal) {
                    m_gotFailsafe = true;
                    disarm();
            }
            else {
                ledSet(true);
            }
        } else {

            m_throttleIsDown = throttleIsDown;

            // If arming is disabled and the ARM switch is on
            if (!readyToArm() && aux1IsSet) {
                m_switchOkay = false;
            } else if (!aux1IsSet) {
                m_switchOkay = true;
            }

            if (!readyToArm()) {
                ledWarningFlash();
            } else {
                ledWarningDisable();
            }

            ledWarningUpdate();
        }

        m_haveSignal = haveSignal;
    }

        typedef enum {
            LED_WARNING_OFF = 0,
            LED_WARNING_ON,
            LED_WARNING_FLASH
        } ledWarningVehicleState_e;

        bool m_ledOn;

        ledWarningVehicleState_e m_ledWarningVehicleState = LED_WARNING_OFF;

        uint32_t m_ledWarningTimer = 0;

        void ledToggle(void)
        {
            m_ledOn = !m_ledOn;
            ledSet(m_ledOn);
        }

        void ledWarningRefresh(void)
        {
            switch (m_ledWarningVehicleState) {
                case LED_WARNING_OFF:
                    ledSet(false);
                    break;
                case LED_WARNING_ON:
                    ledSet(true);
                    break;
                case LED_WARNING_FLASH:
                    ledToggle();
                    break;
            }

            m_ledWarningTimer = micros() + 500000;
        }

        void ledSet(bool on)
        {
            if (m_ledPin > 0) {
                digitalWrite(m_ledPin, m_ledInverted ? on : !on);
            }

            m_ledOn = on;
        }

        void ledBegin(void)
        {
            if (m_ledPin > 0) {
                pinMode(m_ledPin, OUTPUT);
            }
        }

        void ledFlash(uint8_t reps, uint16_t delayMs)
        {
            ledSet(false);
            for (auto i=0; i<reps; i++) {
                ledToggle();
                delay(delayMs);
            }
            ledSet(false);
        }

        void ledWarningFlash(void)
        {
            m_ledWarningVehicleState = LED_WARNING_FLASH;
        }

        void ledWarningDisable(void)
        {
            m_ledWarningVehicleState = LED_WARNING_OFF;
        }

        void ledWarningUpdate(void)
        {
            if ((int32_t)(micros() - m_ledWarningTimer) < 0) {
                return;
            }

            ledWarningRefresh();
        }

        bool isCoreReady(uint32_t nowCycles)
        {
            m_scheduler.m_nextTargetCycles = m_scheduler.lastTargetCycles +
                m_scheduler.desiredPeriodCycles;

            m_scheduler.m_loopRemainingCycles = intcmp(m_scheduler.m_nextTargetCycles, nowCycles);

            if (m_scheduler.m_loopRemainingCycles < -m_scheduler.desiredPeriodCycles) {
                // A task has so grossly overrun that at entire gyro cycle has
                // been skipped This is most likely to occur when connected to
                // the configurator via USB as the serial task is
                // non-deterministic Recover as best we can, advancing
                // scheduling by a whole number of cycles
                m_scheduler.m_nextTargetCycles += m_scheduler.desiredPeriodCycles * (1 +
                        (m_scheduler.m_loopRemainingCycles / -m_scheduler.desiredPeriodCycles));
                m_scheduler.m_loopRemainingCycles = intcmp(
                        m_scheduler.getNextTargetCycles(), nowCycles);
            }

            // Tune out the time lost between completing the last task
            // execution and re-entering the scheduler
            if ((m_scheduler.m_loopRemainingCycles < m_scheduler.m_loopStartMinCycles) &&
                    (m_scheduler.m_loopStartCycles < m_scheduler.m_loopStartMaxCycles)) {
                m_scheduler.m_loopStartCycles += m_scheduler.m_loopStartDeltaUpCycles;
            }

            // Once close to the timing boundary, poll for its arrival
            return m_scheduler.m_loopRemainingCycles < m_scheduler.m_loopStartCycles;
        }

        bool isDynamicReady(uint32_t nowCycles) 
        {
            auto newLoopRemainingCyles =
                intcmp(m_scheduler.getNextTargetCycles(), nowCycles);

            return newLoopRemainingCyles > m_scheduler.m_guardMargin;
        }


    protected:

        Scheduler m_scheduler;

        // Initialized in sketch
        Imu * m_imu;

        SkyrangerTask m_skyrangerTask = SkyrangerTask(m_vstate);

        VisualizerTask m_visualizerTask = VisualizerTask(m_msp, m_vstate, m_skyrangerTask);

        Board(
                Receiver & receiver,
                Imu & imu,
                vector<PidController *> & pidControllers,
                Mixer & mixer,
                Esc & esc,
                const int8_t ledPin)
        {
            m_receiverTask.receiver = &receiver;

            m_imu = &imu;
            m_pidControllers = &pidControllers;
            m_mixer = &mixer;
            m_esc = &esc;

            m_ledPin = ledPin < 0 ? -ledPin : ledPin;
            m_ledInverted = ledPin < 0;

            esc.m_board = this;
            receiver.m_board = this;
        }

        auto prioritizeDynamicTasks(const uint32_t usec) -> Task::prioritizer_t
        {
            Task::prioritizer_t prioritizer = {Task::NONE, 0};

            m_receiverTask.prioritize(usec, prioritizer);
            m_attitudeTask.prioritize(usec, prioritizer);
            m_visualizerTask.prioritize(usec, prioritizer);

            return prioritizer;
        }

        int32_t getTaskGuardCycles(void)
        {
            return m_scheduler.m_taskGuardCycles;
        }

        void updateDynamic(uint32_t nowCycles, uint32_t anticipatedEndCycles)
        {
            auto cyclesOverdue = intcmp(nowCycles, anticipatedEndCycles);

            if ((cyclesOverdue > 0) || (-cyclesOverdue < m_scheduler.m_taskGuardMinCycles)) {

                if (m_scheduler.m_taskGuardCycles < m_scheduler.m_taskGuardMaxCycles) {
                    m_scheduler.m_taskGuardCycles += m_scheduler.m_taskGuardDeltaUpCycles;
                }
            } else if (m_scheduler.m_taskGuardCycles > m_scheduler.m_taskGuardMinCycles) {
                m_scheduler.m_taskGuardCycles -= m_scheduler.m_taskGuardDeltaDownCycles;
            }        
        }

        void runPrioritizedTask(Task::prioritizer_t prioritizer)
        {
            switch (prioritizer.id) {

                case Task::ATTITUDE:
                    runAttitudeTask();
                    break;

                case Task::VISUALIZER:
                    runVisualizerTask();
                    break;

                case Task::RECEIVER:
                    runReceiverTask();
                    break;

                default:
                    break;
            }
        }

        virtual void checkDynamicTasks(void)
        {
            Task::prioritizer_t prioritizer = prioritizeDynamicTasks(micros());

            runPrioritizedTask(prioritizer);
        }

        void runAttitudeTask(void)
        {
            const auto nowCycles = getCycleCounter();

            const uint32_t taskRequiredCycles = 
                m_attitudeTask.checkReady(
                        m_scheduler.getNextTargetCycles(),
                        nowCycles,
                        getTaskGuardCycles());

            if (taskRequiredCycles > 0) {

                const auto anticipatedEndCycles = nowCycles + taskRequiredCycles;

                const auto usec = micros();

                m_attitudeTask.run(usec);

                m_attitudeTask.update(usec, micros()-usec);

                updateDynamic(getCycleCounter(), anticipatedEndCycles);
            } 

            updateArmingFromImu();
        }

        void runReceiverTask(void)
        {
            const auto nowCycles = getCycleCounter();

            const uint32_t taskRequiredCycles = 
                m_receiverTask.checkReady(
                        m_scheduler.getNextTargetCycles(),
                        nowCycles,
                        getTaskGuardCycles());

            if (taskRequiredCycles > 0) {

                const auto anticipatedEndCycles = nowCycles + taskRequiredCycles;

                const auto usec = micros();

                m_receiverTask.run(usec);

                m_receiverTask.update(usec, micros()-usec);

                updateDynamic(getCycleCounter(), anticipatedEndCycles);
            } 

            updateArmingFromReceiver();
        }

        void runVisualizerTask(void)
        {
            const auto nowCycles = getCycleCounter();

            const uint32_t taskRequiredCycles = 
                m_visualizerTask.checkReady(
                        m_scheduler.getNextTargetCycles(),
                        nowCycles,
                        getTaskGuardCycles());

            if (taskRequiredCycles > 0) {

                const auto anticipatedEndCycles = nowCycles + taskRequiredCycles;

                const auto usec = micros();

                while (Serial.available()) {

                    if (m_visualizerTask.parse(Serial.read())) {
                        Serial.write(m_msp.payload, m_msp.payloadSize);
                    }
                }

                m_visualizerTask.update(usec, micros()-usec);

                updateDynamic(getCycleCounter(), anticipatedEndCycles);
            }
        }

        void updateArmingFromReceiver(void)
        {
            Receiver * receiver = m_receiverTask.receiver;

            switch (receiver->getState()) {

                case Receiver::STATE_UPDATE:
                    attemptToArm(micros(), receiver->aux1IsSet());
                    break;

                case Receiver::STATE_CHECK:
                    updateFromReceiver(
                            receiver->throttleIsDown(),
                            receiver->aux1IsSet(),
                            receiver->hasSignal());
                    break;

                default:
                    break;
            }
        }

        void updateArmingFromImu(void)
        {
            const auto imuIsLevel =
                fabsf(m_vstate.phi) < m_maxArmingAngle &&
                fabsf(m_vstate.theta) < m_maxArmingAngle;

            updateArmingFromImu(imuIsLevel, m_imu->gyroIsCalibrating()); 
        }


    public:

        uint32_t microsToCycles(uint32_t micros)
        {
            return getClockSpeed() / 1000000 * micros;
        }

        virtual uint32_t getClockSpeed(void)  = 0;

        virtual uint32_t getCycleCounter(void) = 0;

        virtual void startCycleCounter(void) = 0;

        virtual void dmaInit(const vector<uint8_t> * motorPins, const uint32_t outputFreq)
        {
            (void)motorPins;
            (void)outputFreq;
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

            m_visualizerTask.begin(m_esc, m_receiverTask.receiver);

            m_imu->begin(getClockSpeed());

            m_esc->begin();

            ledBegin();
            ledFlash(10, 50);
        }

        void step(void)
        {
            // Realtime gyro/filtering/PID task get complete priority
            auto nowCycles = getCycleCounter();

            if (isCoreReady(nowCycles)) {
                checkCoreTasks(nowCycles);
            }

            if (isDynamicReady(getCycleCounter())) {
                checkDynamicTasks();
            }
        }

        void step(HardwareSerial & serial)
        {
            step();
            
            while (m_skyrangerTask.imuDataAvailable()) {
                serial.write(m_skyrangerTask.readImuData());
            }
        }

        static void setInterrupt(const uint8_t pin, void (*irq)(void), const uint32_t mode)
        {
            pinMode(pin, INPUT);
            attachInterrupt(pin, irq, mode);  
        }

        static void handleReceiverSerialEvent(Receiver & rx, HardwareSerial & serial) {

            while (serial.available()) {

                rx.parse(serial.read(), micros());
            }
        }

        static void serialWrite(const uint8_t buffer[], const uint8_t size)
        {
            Serial.write(buffer, size);
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
