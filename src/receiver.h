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

#include <math.h>

#include "arming.h"
#include "core_dt.h"
#include "datatypes.h"
#include "failsafe.h"
#include "maths.h"
#include "pt3_filter.h"
#include "pwm.h"
#include "rx_rate.h"
#include "scale.h"
#include "serial.h"
#include "time.h"

class Receiver {

    private:

        static const uint8_t CHANNEL_COUNT = 18;
        static const uint8_t THROTTLE_LOOKUP_TABLE_SIZE = 12;

        static const uint32_t FAILSAFE_POWER_ON_DELAY_US = (1000 * 1000 * 5);

        // Minimum rc smoothing cutoff frequency
        static const uint16_t SMOOTHING_CUTOFF_MIN_HZ = 15;    

        // The value to use for "auto" when interpolated feedforward is enabled
        static const uint16_t SMOOTHING_FEEDFORWARD_INITIAL_HZ = 100;   

        // Guard time to wait after retraining to prevent retraining again too
        // quickly
        static const uint16_t SMOOTHING_FILTER_RETRAINING_DELAY_MS = 2000;  

        // Number of rx frame rate samples to average during frame rate changes
        static const uint8_t  SMOOTHING_FILTER_RETRAINING_SAMPLES = 20;    

        // Time to wait after power to let the PID loop stabilize before starting
        // average frame rate calculation
        static const uint16_t SMOOTHING_FILTER_STARTUP_DELAY_MS = 5000;  

        // Additional time to wait after receiving first valid rx frame before
        // initial training starts
        static const uint16_t SMOOTHING_FILTER_TRAINING_DELAY_MS = 1000;  

        // Number of rx frame rate samples to average during initial training
        static const uint8_t  SMOOTHING_FILTER_TRAINING_SAMPLES = 50;    

        // Look for samples varying this much from the current detected frame
        // rate to initiate retraining
        static const uint8_t  SMOOTHING_RATE_CHANGE_PERCENT = 20;    

        // 65.5ms or 15.26hz
        static const uint32_t SMOOTHING_RATE_MAX_US = 65500; 

        // 0.950ms to fit 1kHz without an issue
        static const uint32_t SMOOTHING_RATE_MIN_US = 950;   

        static const uint32_t DELAY_15_HZ       = 1000000 / 15;

        static const uint32_t NEED_SIGNAL_MAX_DELAY_US  = 1000000 / 10;
        static const uint16_t  MAX_INVALID__PULSE_TIME     = 300;
        static const uint16_t  RATE_LIMIT                  = 1998;
        static constexpr float THR_EXPO8                   = 0;
        static constexpr float THR_MID8                    = 50;
        static constexpr float COMMAND_DIVIDER             = 500;
        static constexpr float YAW_COMMAND_DIVIDER         = 500;

    public:

        // minimum PWM pulse width which is considered valid
        static const uint16_t PWM_PULSE_MIN   = 750;   

        // maximum PWM pulse width which is considered valid
        static const uint16_t PWM_PULSE_MAX   = 2250;  


        typedef struct {
            demands_t demands;
            float aux1;
            float aux2;
        } axes_t;

        typedef enum rc_alias {
            THROTTLE,
            ROLL,
            PITCH,
            YAW,
            AUX1,
            AUX2
        } rc_alias_e;

        typedef enum {
            FRAME_PENDING = 0,
            FRAME_COMPLETE = (1 << 0),
            FRAME_FAILSAFE = (1 << 1),
            FRAME_PROCESSING_REQUIRED = (1 << 2),
            FRAME_DROPPED = (1 << 3)
        } rxFrameState_e;

        typedef enum {
            FAILSAFE_MODE_AUTO = 0,
            FAILSAFE_MODE_HOLD,
            FAILSAFE_MODE_SET,
            FAILSAFE_MODE_INVALID
        } rxFailsafeChannelMode_e;

        typedef struct rxFailsafeChannelConfig_s {
            uint8_t mode; 
            uint8_t step;
        } rxFailsafeChannelConfig_t;

        typedef struct rxChannelRangeConfig_s {
            uint16_t min;
            uint16_t max;
        } rxChannelRangeConfig_t;


        typedef enum {
            STATE_CHECK,
            STATE_PROCESS,
            STATE_MODES,
            STATE_UPDATE,
            STATE_COUNT
        } rxState_e;

        typedef struct rxSmoothingFilter_s {

            uint8_t     autoSmoothnessFactorSetpoint;
            uint32_t    averageFrameTimeUs;
            uint8_t     autoSmoothnessFactorThrottle;
            uint16_t    feedforwardCutoffFrequency;
            uint8_t     ffCutoffSetting;

            pt3Filter_t filterThrottle;
            pt3Filter_t filterRoll;
            pt3Filter_t filterPitch;
            pt3Filter_t filterYaw;

            pt3Filter_t filterDeflectionRoll;
            pt3Filter_t filterDeflectionPitch;

            bool        filterInitialized;
            uint16_t    setpointCutoffFrequency;
            uint8_t     setpointCutoffSetting;
            uint16_t    throttleCutoffFrequency;
            uint8_t     throttleCutoffSetting;
            float       trainingSum;
            uint32_t    trainingCount;
            uint16_t    trainingMax;
            uint16_t    trainingMin;

        } rxSmoothingFilter_t;

        typedef void    (*rx_dev_init_fun
                )(serialPortIdentifier_e port);
        typedef uint8_t (*rx_dev_check_fun)
            (uint16_t * channelData, uint32_t * frameTimeUs);
        typedef float   (*rx_dev_convert_fun)
            (uint16_t * channelData, uint8_t chan);

        typedef struct {

            rx_dev_init_fun init;
            rx_dev_check_fun check;
            rx_dev_convert_fun convert;

        } device_funs_t;

        typedef struct {

            rxSmoothingFilter_t smoothingFilter;

            bool               auxiliaryProcessingRequired;
            bool               calculatedCutoffs;
            uint16_t           channelData[CHANNEL_COUNT];
            float              command[4];
            demands_t          commands;
            bool               dataProcessingRequired;
            demands_t          dataToSmooth;
            rx_dev_check_fun   devCheck;
            rx_dev_convert_fun devConvert;
            int32_t            frameTimeDeltaUs;
            bool               gotNewData;
            bool               inFailsafeMode;
            bool               initializedFilter;
            bool               initializedThrottleTable;
            uint32_t           invalidPulsePeriod[CHANNEL_COUNT];
            bool               isRateValid;
            uint32_t           lastFrameTimeUs;
            uint32_t           lastRxTimeUs;
            int16_t            lookupThrottleRc[THROTTLE_LOOKUP_TABLE_SIZE];
            uint32_t           needSignalBefore;
            uint32_t           nextUpdateAtUs;
            uint32_t           previousFrameTimeUs;
            float              raw[CHANNEL_COUNT];
            uint32_t           refreshPeriod;
            bool               signalReceived;
            rxState_e          state;
            uint32_t           validFrameTimeMs;

        } data_t;


        static float pt3FilterGain(float f_cut, float dT)
        {
            const float order = 3.0f;
            const float orderCutoffCorrection =
                1 / sqrtf(powf(2, 1.0f / order) - 1);
            float RC = 1 / (2 * orderCutoffCorrection * M_PI * f_cut);
            // float RC = 1 / (2 * 1.961459177f * M_PI * f_cut);
            // where 1.961459177 = 1 / sqrt( (2^(1 / order) - 1) ) and order is 3
            return dT / (RC + dT);
        }

        static void pt3FilterInit(pt3Filter_t *filter, float k)
        {
            filter->state = 0;
            filter->state1 = 0;
            filter->state2 = 0;
            filter->k = k;
        }

        static void pt3FilterUpdateCutoff(pt3Filter_t *filter, float k)
        {
            filter->k = k;
        }

        static inline int32_t cmp32(uint32_t a, uint32_t b)
        {
            return (int32_t)(a-b); 
        }

        static uint8_t rxfail_step_to_channel_value(uint8_t step)
        {
            return (PWM_PULSE_MIN + 25 * step);
        }

        static bool isPulseValid(uint16_t pulseDuration)
        {
            return  pulseDuration >= 885 && pulseDuration <= 2115;
        }

        static uint16_t getFailValue(float * rcData, uint8_t channel)
        {
            rxFailsafeChannelConfig_t rxFailsafeChannelConfigs[CHANNEL_COUNT];

            for (uint8_t i = 0; i < CHANNEL_COUNT; i++) {
                rxFailsafeChannelConfigs[i].step = 30;
            }
            rxFailsafeChannelConfigs[3].step = 5;
            for (uint8_t i = 0; i < 4; i++) {
                rxFailsafeChannelConfigs[i].mode = 0;
            }
            for (uint8_t i = 4; i < CHANNEL_COUNT; i++) {
                rxFailsafeChannelConfigs[i].mode = 1;
            }

            const rxFailsafeChannelConfig_t *channelFailsafeConfig =
                &rxFailsafeChannelConfigs[channel];

            switch (channelFailsafeConfig->mode) {
                case FAILSAFE_MODE_AUTO:
                    return channel == ROLL || channel == PITCH || channel == YAW ?
                        1500 :
                        885;
                case FAILSAFE_MODE_INVALID:
                case FAILSAFE_MODE_HOLD:
                    return rcData[channel];
                case FAILSAFE_MODE_SET:
                    return
                        rxfail_step_to_channel_value(channelFailsafeConfig->step);
            }

            return 0;
        }

        static float applyRxChannelRangeConfiguraton(
                float sample,
                const rxChannelRangeConfig_t *range)
        {
            // Avoid corruption of channel with a value of PPM_RCVR_TIMEOUT
            if (sample == 0) {
                return 0;
            }

            sample =
                scaleRangef(sample, range->min, range->max, PWM_MIN, PWM_MAX);
            sample = constrain_f(sample, PWM_PULSE_MIN, PWM_PULSE_MAX);

            return sample;
        }

        // Determine a cutoff frequency based on smoothness factor and calculated
        // average rx frame time
        static int calcAutoSmoothingCutoff(
                int avgRxFrameTimeUs,
                uint8_t autoSmoothnessFactor)
        {
            if (avgRxFrameTimeUs > 0) {
                const float cutoffFactor =
                    1.5f / (1.0f + (autoSmoothnessFactor / 10.0f));
                float cutoff =
                    (1 / (avgRxFrameTimeUs * 1e-6f));  // link frequency
                cutoff = cutoff * cutoffFactor;
                return lrintf(cutoff);
            } else {
                return 0;
            }
        }

        static void rcSmoothingResetAccumulation(
                rxSmoothingFilter_t *smoothingFilter)
        {
            smoothingFilter->trainingSum = 0;
            smoothingFilter->trainingCount = 0;
            smoothingFilter->trainingMin = UINT16_MAX;
            smoothingFilter->trainingMax = 0;
        }

        static void initChannelRangeConfig(rxChannelRangeConfig_t  * config)
        {
            config->min = PWM_MIN;
            config->max = PWM_MAX;
        }

        static void readChannelsApplyRanges(data_t * rx, float raw[])
        {
            rxChannelRangeConfig_t rxChannelRangeConfigThrottle = {};
            rxChannelRangeConfig_t rxChannelRangeConfigRoll = {};
            rxChannelRangeConfig_t rxChannelRangeConfigPitch = {};
            rxChannelRangeConfig_t rxChannelRangeConfigYaw = {};

            initChannelRangeConfig(&rxChannelRangeConfigThrottle);
            initChannelRangeConfig(&rxChannelRangeConfigRoll);
            initChannelRangeConfig(&rxChannelRangeConfigPitch);
            initChannelRangeConfig(&rxChannelRangeConfigYaw);

            for (uint8_t channel=0; channel<CHANNEL_COUNT; ++channel) {

                // sample the channel
                float sample = rx->devConvert(rx->channelData, channel);

                // apply the rx calibration
                switch (channel) {
                    case 0:
                        sample =applyRxChannelRangeConfiguraton(sample,
                                &rxChannelRangeConfigThrottle);
                        break;
                    case 1:
                        sample = applyRxChannelRangeConfiguraton(sample,
                                &rxChannelRangeConfigRoll);
                        break;
                    case 2:
                        sample = applyRxChannelRangeConfiguraton(sample,
                                &rxChannelRangeConfigPitch);
                        break;
                    case 3:
                        sample = applyRxChannelRangeConfiguraton(sample,
                                &rxChannelRangeConfigYaw);
                        break;
                }

                raw[channel] = sample;
            }
        }

        static void detectAndApplySignalLossBehaviour(
                data_t * rx,
                arming_t * arming,
                uint32_t currentTimeUs,
                float raw[])
        {
            uint32_t currentTimeMs = currentTimeUs/ 1000;

            bool useValueFromRx = rx->signalReceived && !rx->inFailsafeMode;

            bool flightChannelsValid = true;

            for (uint8_t channel = 0; channel < CHANNEL_COUNT; channel++) {

                float sample = raw[channel];

                bool validPulse = useValueFromRx && isPulseValid(sample);

                if (validPulse) {
                    rx->invalidPulsePeriod[channel] =
                        currentTimeMs + MAX_INVALID__PULSE_TIME;
                } else {
                    if (cmp32(currentTimeMs,
                                rx->invalidPulsePeriod[channel]) < 0) {
                        // skip to next channel to hold channel value
                        // MAX_INVALID__PULSE_TIME
                        continue;           
                    } else {

                        // after that apply rxfail value
                        sample = getFailValue(raw, channel); 
                        if (channel < 4) {
                            flightChannelsValid = false;
                        }
                    }
                }

                raw[channel] = sample;
            }

            if (flightChannelsValid) {
                failsafeOnValidDataReceived(arming);
            } else {
                rx->inFailsafeMode = true;
                failsafeOnValidDataFailed(arming);
                for (uint8_t channel = 0; channel < CHANNEL_COUNT; channel++) {
                    raw[channel] = getFailValue(raw, channel);
                }
            }
        }

        static int16_t lookupThrottle(data_t * rx, int32_t tmp)
        {
            if (!rx->initializedThrottleTable) {
                for (uint8_t i = 0; i < THROTTLE_LOOKUP_TABLE_SIZE; i++) {
                    const int16_t tmp2 = 10 * i - THR_MID8;
                    uint8_t y = tmp2 > 0 ?
                        100 - THR_MID8 :
                        tmp2 < 0 ?
                        THR_MID8 :
                        1;
                    rx->lookupThrottleRc[i] =
                        10 * THR_MID8 + tmp2 * (100 - THR_EXPO8 + (int32_t)
                                THR_EXPO8 * (tmp2 * tmp2) / (y * y)) / 10;
                    rx->lookupThrottleRc[i] = PWM_MIN + (PWM_MAX - PWM_MIN) *
                        rx->lookupThrottleRc[i] / 1000; 
                }
            }

            rx->initializedThrottleTable = true;

            const int32_t tmp3 = tmp / 100;
            // [0;1000] -> expo -> [MINTHROTTLE;MAXTHROTTLE]
            return rx->lookupThrottleRc[tmp3] + (tmp - tmp3 * 100) *
                (rx->lookupThrottleRc[tmp3 + 1] - rx->lookupThrottleRc[tmp3]) / 100;
        }

        static float updateCommand(float raw, float sgn)
        {
            float tmp = fminf(fabs(raw - 1500), 500);

            float cmd = tmp * sgn;

            return raw < 1500 ? -cmd : cmd;
        }

        static void updateCommands(data_t * rx, float raw[])
        {
            for (uint8_t axis=ROLL; axis<=YAW; axis++) {
                // non coupled PID reduction scaler used in PID controller 1
                // and PID controller 2.
                rx->command[axis] =
                    updateCommand(raw[axis], axis == YAW ? -1 : +1);
            }

            int32_t tmp = constrain_f_i32(raw[THROTTLE], 1050, PWM_MAX);
            int32_t tmp2 = (uint32_t)(tmp - 1050) * PWM_MIN / (PWM_MAX - 1050);

            rx->commands.throttle = lookupThrottle(rx, tmp2);
        }

        static bool calculateChannelsAndUpdateFailsafe(
                data_t * rx,
                arming_t * arming,
                uint32_t currentTimeUs,
                float raw[])
        {
            if (rx->auxiliaryProcessingRequired) {
                rx->auxiliaryProcessingRequired = false;
            }

            if (!rx->dataProcessingRequired) {
                return false;
            }

            rx->dataProcessingRequired = false;
            rx->nextUpdateAtUs = currentTimeUs + DELAY_15_HZ;

            readChannelsApplyRanges(rx, raw);
            detectAndApplySignalLossBehaviour(rx, arming, currentTimeUs, raw);

            return true;
        }

        static int32_t getFrameDelta(
                data_t * rx, uint32_t currentTimeUs, int32_t *frameAgeUs)
        {
            uint32_t frameTimeUs = rx->lastFrameTimeUs;

            *frameAgeUs = cmpTimeUs(currentTimeUs, frameTimeUs);

            const int32_t deltaUs =
                cmpTimeUs(frameTimeUs, rx->previousFrameTimeUs);
            if (deltaUs) {
                rx->frameTimeDeltaUs = deltaUs;
                rx->previousFrameTimeUs = frameTimeUs;
            }

            return rx->frameTimeDeltaUs;
        }

        static bool processData(
                data_t * rx,
                void * motorDevice,
                float raw[],
                uint32_t currentTimeUs,
                arming_t * arming)
        {
            int32_t frameAgeUs;

            int32_t refreshPeriodUs =
                getFrameDelta(rx, currentTimeUs, &frameAgeUs);

            if (!refreshPeriodUs ||
                    cmpTimeUs(currentTimeUs, rx->lastRxTimeUs) <= frameAgeUs) {

                // calculate a delta here if not supplied by the protocol
                refreshPeriodUs = cmpTimeUs(currentTimeUs, rx->lastRxTimeUs); 
            }

            rx->lastRxTimeUs = currentTimeUs;

            rx->isRateValid =
                ((uint32_t)refreshPeriodUs >= SMOOTHING_RATE_MIN_US &&
                 (uint32_t)refreshPeriodUs <= SMOOTHING_RATE_MAX_US);

            rx->refreshPeriod =
                constrain_i32_u32(refreshPeriodUs, SMOOTHING_RATE_MIN_US,
                        SMOOTHING_RATE_MAX_US);

            if (currentTimeUs >
                    FAILSAFE_POWER_ON_DELAY_US && !failsafeIsMonitoring()) {
                failsafeStartMonitoring();
            }

            failsafeUpdateState(raw, motorDevice, arming);

            return throttleIsDown(raw);
        }

        static bool throttleIsDown(float raw[])
        {
            return raw[THROTTLE] < 1050;
        }

        static void ratePidFeedforwardLpfInit(
                anglePid_t * pid, uint16_t filterCutoff)
        {
            if (filterCutoff > 0) {
                pid->feedforwardLpfInitialized = true;
                pt3FilterInit(&pid->feedforwardPt3[0],
                        pt3FilterGain(filterCutoff, CORE_DT()));
                pt3FilterInit(&pid->feedforwardPt3[1],
                        pt3FilterGain(filterCutoff, CORE_DT()));
                pt3FilterInit(&pid->feedforwardPt3[2],
                        pt3FilterGain(filterCutoff, CORE_DT()));
            }
        }

        static void ratePidFeedforwardLpfUpdate(
                anglePid_t * pid, uint16_t filterCutoff)
        {
            if (filterCutoff > 0) {
                for (uint8_t axis=ROLL; axis<=YAW; axis++) {
                    pt3FilterUpdateCutoff(&pid->feedforwardPt3[axis],
                            pt3FilterGain(filterCutoff, CORE_DT()));
                }
            }
        }

        static void smoothingFilterInit(
                rxSmoothingFilter_t * smoothingFilter,
                pt3Filter_t * filter,
                float setpointCutoffFrequency,
                float dT)
        {
            if (!smoothingFilter->filterInitialized) {
                pt3FilterInit(
                        filter, pt3FilterGain(setpointCutoffFrequency, dT)); 
            } else {
                pt3FilterUpdateCutoff(
                        filter, pt3FilterGain(setpointCutoffFrequency, dT)); 
            }
        }

        static void smoothingFilterInitRollPitchYaw(
                rxSmoothingFilter_t * smoothingFilter,
                pt3Filter_t * filter,
                float dT)
        {
            smoothingFilterInit(smoothingFilter, filter,
                    smoothingFilter->setpointCutoffFrequency, dT);
        }

        static void levelFilterInit(
                rxSmoothingFilter_t * smoothingFilter,
                pt3Filter_t * filter,
                float dT)
        {
            if (!smoothingFilter->filterInitialized) {
                pt3FilterInit(filter,
                        pt3FilterGain(
                            smoothingFilter->setpointCutoffFrequency, dT)); 
            } else {
                pt3FilterUpdateCutoff(filter,
                        pt3FilterGain(
                            smoothingFilter->setpointCutoffFrequency, dT)); 
            }
        }

        static void smoothingFilterApply(
                rxSmoothingFilter_t * smoothingFilter,
                pt3Filter_t * filter,
                float dataToSmooth,
                float * dst)
        {
            if (smoothingFilter->filterInitialized) {
                *dst = pt3FilterApply(filter, dataToSmooth);
            } else {
                // If filter isn't initialized yet, as in smoothing off, use the
                // actual unsmoothed rx channel data
                *dst = dataToSmooth;
            }
        }

        static void setSmoothingFilterCutoffs(anglePid_t * ratepid,
                rxSmoothingFilter_t *smoothingFilter)
        {
            const float dT = CORE_PERIOD() * 1e-6f;
            uint16_t oldCutoff = smoothingFilter->setpointCutoffFrequency;

            if (smoothingFilter->setpointCutoffSetting == 0) {
                smoothingFilter->setpointCutoffFrequency =
                    fmaxf(SMOOTHING_CUTOFF_MIN_HZ,
                            calcAutoSmoothingCutoff(
                                smoothingFilter->averageFrameTimeUs,
                                smoothingFilter->autoSmoothnessFactorSetpoint)); 
            }
            if (smoothingFilter->throttleCutoffSetting == 0) {
                smoothingFilter->throttleCutoffFrequency =
                    fmaxf(SMOOTHING_CUTOFF_MIN_HZ,
                            calcAutoSmoothingCutoff(
                                smoothingFilter->averageFrameTimeUs,
                                smoothingFilter->autoSmoothnessFactorThrottle));
            }

            // initialize or update the Setpoint filter
            if ((smoothingFilter->setpointCutoffFrequency != oldCutoff) ||
                    !smoothingFilter->filterInitialized) {

                smoothingFilterInit(
                        smoothingFilter, &smoothingFilter->filterThrottle,
                        smoothingFilter->throttleCutoffFrequency, dT);

                smoothingFilterInitRollPitchYaw(smoothingFilter,
                        &smoothingFilter->filterRoll, dT);
                smoothingFilterInitRollPitchYaw(smoothingFilter,
                        &smoothingFilter->filterPitch, dT);
                smoothingFilterInitRollPitchYaw(smoothingFilter,
                        &smoothingFilter->filterYaw, dT);

                levelFilterInit(
                        smoothingFilter,
                        &smoothingFilter->filterDeflectionRoll,
                        dT);
                levelFilterInit(
                        smoothingFilter,
                        &smoothingFilter->filterDeflectionPitch,
                        dT);
            }

            // update or initialize the FF filter
            oldCutoff = smoothingFilter->feedforwardCutoffFrequency;
            if (smoothingFilter->ffCutoffSetting == 0) {
                smoothingFilter->feedforwardCutoffFrequency =
                    fmaxf(SMOOTHING_CUTOFF_MIN_HZ,
                            calcAutoSmoothingCutoff(
                                smoothingFilter->averageFrameTimeUs,
                                smoothingFilter->autoSmoothnessFactorSetpoint)); 
            }
            if (!smoothingFilter->filterInitialized) {
                ratePidFeedforwardLpfInit(ratepid,
                        smoothingFilter->feedforwardCutoffFrequency);
            } else if (smoothingFilter->feedforwardCutoffFrequency != oldCutoff) {
                ratePidFeedforwardLpfUpdate(ratepid,
                        smoothingFilter->feedforwardCutoffFrequency);
            }
        }


        static bool rcSmoothingAccumulateSample(
                rxSmoothingFilter_t *smoothingFilter,
                int rxFrameTimeUs)
        {
            smoothingFilter->trainingSum += rxFrameTimeUs;
            smoothingFilter->trainingCount++;
            smoothingFilter->trainingMax =
                fmaxf(smoothingFilter->trainingMax, rxFrameTimeUs);
            smoothingFilter->trainingMin =
                fminf(smoothingFilter->trainingMin, rxFrameTimeUs);

            // if we've collected enough samples then calculate the average and
            // reset the accumulation
            uint32_t sampleLimit = (smoothingFilter->filterInitialized) ?
                SMOOTHING_FILTER_RETRAINING_SAMPLES :
                SMOOTHING_FILTER_TRAINING_SAMPLES;

            if (smoothingFilter->trainingCount >= sampleLimit) {
                // Throw out high and low samples
                smoothingFilter->trainingSum = smoothingFilter->trainingSum -
                    smoothingFilter->trainingMin - smoothingFilter->trainingMax; 

                smoothingFilter->averageFrameTimeUs =
                    lrintf(smoothingFilter->trainingSum /
                        (smoothingFilter->trainingCount - 2));
                rcSmoothingResetAccumulation(smoothingFilter);
                return true;
            }
            return false;
        }


        static bool rcSmoothingAutoCalculate(
                rxSmoothingFilter_t * smoothingFilter)
        {
            // if any rc smoothing cutoff is 0 (auto) then we need to calculate
            // cutoffs
            if ((smoothingFilter->setpointCutoffSetting == 0) ||
                    (smoothingFilter->ffCutoffSetting == 0) ||
                    (smoothingFilter->throttleCutoffSetting == 0)) {
                return true;
            }
            return false;
        }

        static void processSmoothingFilter(
                uint32_t currentTimeUs,
                data_t * rx,
                anglePid_t * ratepid,
                float setpointRate[4],
                float rawSetpoint[3])
        {
            // first call initialization
            if (!rx->initializedFilter) {

                rx->smoothingFilter.filterInitialized = false;
                rx->smoothingFilter.averageFrameTimeUs = 0;
                rx->smoothingFilter.autoSmoothnessFactorSetpoint = 30;
                rx->smoothingFilter.autoSmoothnessFactorThrottle = 30;
                rx->smoothingFilter.setpointCutoffSetting = 0;
                rx->smoothingFilter.throttleCutoffSetting = 0;
                rx->smoothingFilter.ffCutoffSetting = 0;
                rcSmoothingResetAccumulation(&rx->smoothingFilter);
                rx->smoothingFilter.setpointCutoffFrequency =
                    rx->smoothingFilter.setpointCutoffSetting;
                rx->smoothingFilter.throttleCutoffFrequency =
                    rx->smoothingFilter.throttleCutoffSetting;
                if (rx->smoothingFilter.ffCutoffSetting == 0) {
                    // calculate and use an initial derivative cutoff until the RC
                    // interval is known
                    const float cutoffFactor = 1.5f /
                        (1.0f +
                         (rx->smoothingFilter.autoSmoothnessFactorSetpoint /
                          10.0f));
                    float ffCutoff = 
                        SMOOTHING_FEEDFORWARD_INITIAL_HZ * cutoffFactor;
                    rx->smoothingFilter.feedforwardCutoffFrequency = 
                        lrintf(ffCutoff);
                } else {
                    rx->smoothingFilter.feedforwardCutoffFrequency =
                        rx->smoothingFilter.ffCutoffSetting;
                }

                rx->calculatedCutoffs = 
                    rcSmoothingAutoCalculate(&rx->smoothingFilter);

                // if we don't need to calculate cutoffs dynamically then the
                // filters can be initialized now
                if (!rx->calculatedCutoffs) {
                    setSmoothingFilterCutoffs(ratepid, &rx->smoothingFilter);
                    rx->smoothingFilter.filterInitialized = true;
                }
            }

            rx->initializedFilter = true;

            if (rx->gotNewData) {
                // for auto calculated filters we need to examine each rx frame
                // interval
                if (rx->calculatedCutoffs) {
                    const uint32_t currentTimeMs = currentTimeUs / 1000;

                    // If the filter cutoffs in auto mode, and we have good rx
                    // data, then determine the average rx frame rate and use
                    // that to calculate the filter cutoff frequencies

                    // skip during FC initialization
                    if ((currentTimeMs > SMOOTHING_FILTER_STARTUP_DELAY_MS))
                    {
                        if (rx->signalReceived && rx->isRateValid) {

                            // set the guard time expiration if it's not set
                            if (rx->validFrameTimeMs == 0) {
                                rx->validFrameTimeMs =
                                    currentTimeMs +
                                    (rx->smoothingFilter.filterInitialized ?
                                     SMOOTHING_FILTER_RETRAINING_DELAY_MS :
                                     SMOOTHING_FILTER_TRAINING_DELAY_MS);
                            } else {
                            }

                            // if the guard time has expired then process the
                            // rx frame time
                            if (currentTimeMs > rx->validFrameTimeMs) {
                                bool accumulateSample = true;

                                // During initial training process all samples.
                                // During retraining check samples to determine
                                // if they vary by more than the limit
                                // percentage.
                                if (rx->smoothingFilter.filterInitialized) {
                                    const float percentChange =
                                        fabs((rx->refreshPeriod -
                                                    rx->smoothingFilter.averageFrameTimeUs) /
                                                (float)rx->smoothingFilter.averageFrameTimeUs) *
                                        100;

                                    if (percentChange <
                                            SMOOTHING_RATE_CHANGE_PERCENT) {
                                        // We received a sample that wasn't
                                        // more than the limit percent so reset
                                        // the accumulation During retraining
                                        // we need a contiguous block of
                                        // samples that are all significantly
                                        // different than the current average
                                        rcSmoothingResetAccumulation(
                                                &rx->smoothingFilter);
                                        accumulateSample = false;
                                    }
                                }

                                // accumlate the sample into the average
                                if (accumulateSample) { if
                                    (rcSmoothingAccumulateSample(
                                                                 &rx->smoothingFilter,
                                                                 rx->refreshPeriod))
                                    {
                                        // the required number of samples were
                                        // collected so set the filter cutoffs, but
                                        // only if smoothing is active
                                        setSmoothingFilterCutoffs(ratepid, &rx->smoothingFilter);
                                        rx->smoothingFilter.filterInitialized = true;
                                        rx->validFrameTimeMs = 0;
                                    }
                                }

                            }
                        } else {
                            // we have either stopped receiving rx samples
                            // (failsafe?) or the sample time is unreasonable
                            // so reset the accumulation
                            rcSmoothingResetAccumulation(&rx->smoothingFilter);
                        }
                    }
                }

                rx->dataToSmooth.throttle = rx->commands.throttle;
                rx->dataToSmooth.roll = rawSetpoint[1];
                rx->dataToSmooth.pitch = rawSetpoint[2];
                rx->dataToSmooth.yaw = rawSetpoint[3];
            }

            // Each pid loop, apply the last received channel value to the
            // filter, if initialised - thanks @klutvott
            smoothingFilterApply(&rx->smoothingFilter,
                    &rx->smoothingFilter.filterThrottle,
                    rx->dataToSmooth.throttle, &rx->commands.throttle);
            smoothingFilterApply(&rx->smoothingFilter,
                    &rx->smoothingFilter.filterRoll,
                    rx->dataToSmooth.roll, &setpointRate[1]);
            smoothingFilterApply(&rx->smoothingFilter,
                    &rx->smoothingFilter.filterPitch,
                    rx->dataToSmooth.pitch, &setpointRate[2]);
            smoothingFilterApply(&rx->smoothingFilter,
                    &rx->smoothingFilter.filterYaw,
                    rx->dataToSmooth.yaw, &setpointRate[3]);
        }

        static float getRawSetpoint(float command, float divider)
        {
            float commandf = command / divider;

            float commandfAbs = fabsf(commandf);

            float angleRate = rxApplyRates(commandf, commandfAbs);

            return constrain_f(angleRate, -(float)RATE_LIMIT, +(float)RATE_LIMIT);
        }


    public:

        // Called from hackflight.c::adjustRxDynamicPriority()
        static bool check(data_t * rx, uint32_t currentTimeUs)
        {
            bool signalReceived = false;
            bool useDataDrivenProcessing = true;

            if (rx->state != STATE_CHECK) {
                return true;
            }

            const uint8_t frameStatus =
                rx->devCheck(rx->channelData, &rx->lastFrameTimeUs);

            if (frameStatus & FRAME_COMPLETE) {
                rx->inFailsafeMode = (frameStatus & FRAME_FAILSAFE) != 0;
                bool rxFrameDropped = (frameStatus & FRAME_DROPPED) != 0;
                signalReceived = !(rx->inFailsafeMode || rxFrameDropped);
                if (signalReceived) {
                    rx->needSignalBefore =
                        currentTimeUs + NEED_SIGNAL_MAX_DELAY_US;
                }
            }

            if (frameStatus & FRAME_PROCESSING_REQUIRED) {
                rx->auxiliaryProcessingRequired = true;
            }

            if (signalReceived) {
                rx->signalReceived = true;
            } else if (currentTimeUs >= rx->needSignalBefore) {
                rx->signalReceived = false;
            }

            if ((signalReceived && useDataDrivenProcessing) ||
                    cmpTimeUs(currentTimeUs, rx->nextUpdateAtUs) > 0) {
                rx->dataProcessingRequired = true;
            }

            // data driven or 50Hz
            return rx->dataProcessingRequired || rx->auxiliaryProcessingRequired; 

        } // check

        static void poll(
                data_t * rx,
                uint32_t currentTimeUs,
                bool imuIsLevel,
                bool calibrating,
                axes_t * rxax,
                void * motorDevice,
                arming_t * arming,
                bool * pidItermResetReady,
                bool * pidItermResetValue,
                bool * gotNewData)
        {
            *pidItermResetReady = false;

            rx->gotNewData = false;

            switch (rx->state) {
                default:
                case STATE_CHECK:
                    rx->state = STATE_PROCESS;
                    break;

                case STATE_PROCESS:
                    if (!calculateChannelsAndUpdateFailsafe(rx,
                                arming, currentTimeUs,
                                rx->raw)) {
                        rx->state = STATE_CHECK;
                        break;
                    }
                    *pidItermResetReady = true;
                    *pidItermResetValue = processData(rx, motorDevice, rx->raw,
                            currentTimeUs, arming);
                    rx->state = STATE_MODES;
                    break;

                case STATE_MODES:
                    armingCheck(arming, motorDevice, currentTimeUs, rx->raw,
                            imuIsLevel,
                            calibrating);
                    rx->state = STATE_UPDATE;
                    break;

                case STATE_UPDATE:
                    rx->gotNewData = true;
                    updateCommands(rx, rx->raw);
                    armingUpdateStatus(arming, rx->raw, imuIsLevel, calibrating);
                    rx->state = STATE_CHECK;
                    break;
            }

            rxax->demands.throttle = rx->raw[THROTTLE];
            rxax->demands.roll     = rx->raw[ROLL];
            rxax->demands.pitch    = rx->raw[PITCH];
            rxax->demands.yaw      = rx->raw[YAW];
            rxax->aux1             = rx->raw[AUX1];
            rxax->aux2             = rx->raw[AUX2];

            *gotNewData = rx->gotNewData;

        } // poll

        // Runs in fast (inner, core) loop
        static void getDemands(
                data_t * rx,
                uint32_t currentTimeUs,
                anglePid_t * ratepid,
                demands_t * demands)
        {
            float rawSetpoint[4] = {};
            float setpointRate[4] = {};

            if (rx->gotNewData) {

                rx->previousFrameTimeUs = 0;

                rawSetpoint[ROLL] =
                    getRawSetpoint(rx->command[ROLL], COMMAND_DIVIDER);
                rawSetpoint[PITCH] =
                    getRawSetpoint(rx->command[PITCH], COMMAND_DIVIDER);
                rawSetpoint[YAW] =
                    getRawSetpoint(rx->command[YAW], YAW_COMMAND_DIVIDER);
            }

            processSmoothingFilter(
                    currentTimeUs, rx, ratepid, setpointRate, rawSetpoint);

            // Find min and max throttle based on conditions. Throttle has to
            // be known before mixing
            demands->throttle =
                constrain_f((rx->commands.throttle - PWM_MIN) /
                        (PWM_MAX - PWM_MIN), 0.0f, 1.0f);

            demands->roll  = setpointRate[ROLL];
            demands->pitch = setpointRate[PITCH];
            demands->yaw   = setpointRate[YAW];

            rx->gotNewData = false;

        } // getDemands


}; // class Receiver

// For hardware impelmentations ------------------------------------------------

uint8_t rxDevCheck(uint16_t * channelData, uint32_t * frameTimeUs);

float rxDevConvert(uint16_t * channelData, uint8_t chan);