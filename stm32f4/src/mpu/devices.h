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

#include <stdbool.h>
#include <stdint.h>

#include "bus.h"
#include "exti.h"
#include "io.h"

typedef enum {
    MPU_NONE,
    MPU_3050,
    MPU_60x0,
    MPU_60x0_SPI,
    MPU_65xx_I2C,
    MPU_65xx_SPI,
    MPU_9250_SPI,
    ICM_20601_SPI,
    ICM_20602_SPI,
    ICM_20608_SPI,
    ICM_20649_SPI,
    ICM_20689_SPI,
    ICM_42605_SPI,
    ICM_42688P_SPI,
    BMI_160_SPI,
    BMI_270_SPI,
    LSM6DSO_SPI,
    L3GD20_SPI,
} mpuSensor_e;

typedef struct mpuDetectionResult_s {
    mpuSensor_e sensor;
} mpuDetectionResult_t;


struct accDev_s;
typedef void (*sensorAccInitFuncPtr)(struct accDev_s *acc);
typedef bool (*sensorAccReadFuncPtr)(struct accDev_s *acc);

struct gyroDev_s;
typedef void (*sensorGyroInitFuncPtr)(struct gyroDev_s *gyro);
typedef bool (*sensorGyroReadFuncPtr)(struct gyroDev_s *gyro);
typedef bool (*sensorGyroReadDataFuncPtr)(struct gyroDev_s *gyro, int16_t *data);

typedef enum {
    GYRO_NONE = 0
} gyroHardware_e;

typedef enum {
    GYRO_HARDWARE_LPF_NORMAL,
    GYRO_HARDWARE_LPF_EXPERIMENTAL
} gyroHardwareLpf_e;

typedef struct fp_rotationMatrix_s {
    float m[3][3];              // matrix
} fp_rotationMatrix_t;

typedef struct gyroDev_s {
    sensorGyroInitFuncPtr initFn;                             // initialize function
    sensorGyroReadFuncPtr readFn;                             // read 3 axis data function
    sensorGyroReadDataFuncPtr temperatureFn;                  // read temperature if available
    extiCallbackRec_t exti;
    extDevice_t dev;
    uint16_t scaleDps;
    int16_t adcRaw[3];                                       // raw data from sensor
    int16_t temperature;
    mpuDetectionResult_t mpuDetectionResult;
    uint32_t detectedEXTI;
    uint32_t gyroLastEXTI;
    uint32_t gyroSyncEXTI;
    int32_t gyroShortPeriod;
    int32_t gyroDmaMaxDuration;
    busSegment_t segments[2];
    volatile bool dataReady;
    uint8_t hardware_lpf;
    uint8_t hardware_32khz_lpf;
    ioTag_t mpuIntExtiTag;
    uint8_t gyroHasOverflowProtection;
    gyroHardware_e gyroHardware;
    fp_rotationMatrix_t rotationMatrix;
    uint16_t gyroSampleRateHz;
    uint16_t accSampleRateHz;
} gyroDev_t;

typedef struct accDev_s {
    sensorAccInitFuncPtr initFn;                              // initialize function
    sensorAccReadFuncPtr readFn;                              // read 3 axis data function
    uint16_t acc_1G;
    int16_t ADCRaw[3];
    mpuDetectionResult_t mpuDetectionResult;
    //sensor_align_e accAlign;
    bool dataReady;
    gyroDev_t *gyro;
    bool acc_high_fsr;
    char revisionCode;                                      // a revision code for the sensor, if known
    uint8_t filler[2];
    fp_rotationMatrix_t rotationMatrix;
} accDev_t;

#if defined(__cplusplus)
extern "C" {
#endif

gyroDev_t * gyroContainerOf(extiCallbackRec_t * cb);

#if defined(__cplusplus)
}
#endif


