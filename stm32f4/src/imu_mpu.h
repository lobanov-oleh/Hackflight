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

#include <accel.h>
//#include <gyro.h>
#include <maths.h>
#include <time.h>

#include "bus.h"
#include "exti.h"

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

typedef struct gyroDev_s {
    sensorGyroInitFuncPtr initFn;                             // initialize function
    sensorGyroReadFuncPtr readFn;                             // read 3 axis data function
    sensorGyroReadDataFuncPtr temperatureFn;                  // read temperature if available
    extiCallbackRec_t exti;
    extDevice_t dev;
    float scale;                                             // scalefactor
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
#define GYRO_USES_SPI

#define MPU_RA_WHO_AM_I         0x75
#define MPU_RA_WHO_AM_I_LEGACY  0x00

#define MPUx0x0_WHO_AM_I_CONST              (0x68) 
#define MPU6000_WHO_AM_I_CONST              (0x68)
#define MPU6500_WHO_AM_I_CONST              (0x70)
#define MPU9250_WHO_AM_I_CONST              (0x71)
#define MPU9255_WHO_AM_I_CONST              (0x73)
#define ICM20601_WHO_AM_I_CONST             (0xAC)
#define ICM20602_WHO_AM_I_CONST             (0x12)
#define ICM20608G_WHO_AM_I_CONST            (0xAF)
#define ICM20649_WHO_AM_I_CONST             (0xE1)
#define ICM20689_WHO_AM_I_CONST             (0x98)
#define ICM42605_WHO_AM_I_CONST             (0x42)
#define ICM42688P_WHO_AM_I_CONST            (0x47)

// RA = Register Address

#define MPU_RA_XG_OFFS_TC       0x00    //[7] PWR_MODE, [6:1] XG_OFFS_TC, [0] OTP_BNK_VLD
#define MPU_RA_YG_OFFS_TC       0x01    //[7] PWR_MODE, [6:1] YG_OFFS_TC, [0] OTP_BNK_VLD
#define MPU_RA_ZG_OFFS_TC       0x02    //[7] PWR_MODE, [6:1] ZG_OFFS_TC, [0] OTP_BNK_VLD
#define MPU_RA_X_FINE_GAIN      0x03    //[7:0] X_FINE_GAIN
#define MPU_RA_Y_FINE_GAIN      0x04    //[7:0] Y_FINE_GAIN
#define MPU_RA_Z_FINE_GAIN      0x05    //[7:0] Z_FINE_GAIN
#define MPU_RA_XA_OFFS_H        0x06    //[15:0] XA_OFFS
#define MPU_RA_XA_OFFS_L_TC     0x07
#define MPU_RA_YA_OFFS_H        0x08    //[15:0] YA_OFFS
#define MPU_RA_YA_OFFS_L_TC     0x09
#define MPU_RA_ZA_OFFS_H        0x0A    //[15:0] ZA_OFFS
#define MPU_RA_ZA_OFFS_L_TC     0x0B
#define MPU_RA_PRODUCT_ID       0x0C    // Product ID Register
#define MPU_RA_XG_OFFS_USRH     0x13    //[15:0] XG_OFFS_USR
#define MPU_RA_XG_OFFS_USRL     0x14
#define MPU_RA_YG_OFFS_USRH     0x15    //[15:0] YG_OFFS_USR
#define MPU_RA_YG_OFFS_USRL     0x16
#define MPU_RA_ZG_OFFS_USRH     0x17    //[15:0] ZG_OFFS_USR
#define MPU_RA_ZG_OFFS_USRL     0x18
#define MPU_RA_SMPLRT_DIV       0x19
#define MPU_RA_CONFIG           0x1A
#define MPU_RA_GYRO_CONFIG      0x1B
#define MPU_RA_ACCEL_CONFIG     0x1C
#define MPU_RA_FF_THR           0x1D
#define MPU_RA_FF_DUR           0x1E
#define MPU_RA_MOT_THR          0x1F
#define MPU_RA_MOT_DUR          0x20
#define MPU_RA_ZRMOT_THR        0x21
#define MPU_RA_ZRMOT_DUR        0x22
#define MPU_RA_FIFO_EN          0x23
#define MPU_RA_I2C_MST_CTRL     0x24
#define MPU_RA_I2C_SLV0_ADDR    0x25
#define MPU_RA_I2C_SLV0_REG     0x26
#define MPU_RA_I2C_SLV0_CTRL    0x27
#define MPU_RA_I2C_SLV1_ADDR    0x28
#define MPU_RA_I2C_SLV1_REG     0x29
#define MPU_RA_I2C_SLV1_CTRL    0x2A
#define MPU_RA_I2C_SLV2_ADDR    0x2B
#define MPU_RA_I2C_SLV2_REG     0x2C
#define MPU_RA_I2C_SLV2_CTRL    0x2D
#define MPU_RA_I2C_SLV3_ADDR    0x2E
#define MPU_RA_I2C_SLV3_REG     0x2F
#define MPU_RA_I2C_SLV3_CTRL    0x30
#define MPU_RA_I2C_SLV4_ADDR    0x31
#define MPU_RA_I2C_SLV4_REG     0x32
#define MPU_RA_I2C_SLV4_DO      0x33
#define MPU_RA_I2C_SLV4_CTRL    0x34
#define MPU_RA_I2C_SLV4_DI      0x35
#define MPU_RA_I2C_MST_STATUS   0x36
#define MPU_RA_INT_PIN_CFG      0x37
#define MPU_RA_INT_ENABLE       0x38
#define MPU_RA_DMP_INT_STATUS   0x39
#define MPU_RA_INT_STATUS       0x3A
#define MPU_RA_ACCEL_XOUT_H     0x3B
#define MPU_RA_ACCEL_XOUT_L     0x3C
#define MPU_RA_ACCEL_YOUT_H     0x3D
#define MPU_RA_ACCEL_YOUT_L     0x3E
#define MPU_RA_ACCEL_ZOUT_H     0x3F
#define MPU_RA_ACCEL_ZOUT_L     0x40
#define MPU_RA_TEMP_OUT_H       0x41
#define MPU_RA_TEMP_OUT_L       0x42
#define MPU_RA_GYRO_XOUT_H      0x43
#define MPU_RA_GYRO_XOUT_L      0x44
#define MPU_RA_GYRO_YOUT_H      0x45
#define MPU_RA_GYRO_YOUT_L      0x46
#define MPU_RA_GYRO_ZOUT_H      0x47
#define MPU_RA_GYRO_ZOUT_L      0x48
#define MPU_RA_EXT_SENS_DATA_00 0x49
#define MPU_RA_MOT_DETECT_STATUS    0x61
#define MPU_RA_I2C_SLV0_DO      0x63
#define MPU_RA_I2C_SLV1_DO      0x64
#define MPU_RA_I2C_SLV2_DO      0x65
#define MPU_RA_I2C_SLV3_DO      0x66
#define MPU_RA_I2C_MST_DELAY_CTRL   0x67
#define MPU_RA_SIGNAL_PATH_RESET    0x68
#define MPU_RA_MOT_DETECT_CTRL      0x69
#define MPU_RA_USER_CTRL        0x6A
#define MPU_RA_PWR_MGMT_1       0x6B
#define MPU_RA_PWR_MGMT_2       0x6C
#define MPU_RA_BANK_SEL         0x6D
#define MPU_RA_MEM_START_ADDR   0x6E
#define MPU_RA_MEM_R_W          0x6F
#define MPU_RA_DMP_CFG_1        0x70
#define MPU_RA_DMP_CFG_2        0x71
#define MPU_RA_FIFO_COUNTH      0x72
#define MPU_RA_FIFO_COUNTL      0x73
#define MPU_RA_FIFO_R_W         0x74
#define MPU_RA_WHO_AM_I         0x75

enum gyro_fsr_e {
    INV_FSR_250DPS = 0,
    INV_FSR_500DPS,
    INV_FSR_1000DPS,
    INV_FSR_2000DPS,
    NUM_GYRO_FSR
};

enum icm_high_range_gyro_fsr_e {
    ICM_HIGH_RANGE_FSR_500DPS = 0,
    ICM_HIGH_RANGE_FSR_1000DPS,
    ICM_HIGH_RANGE_FSR_2000DPS,
    ICM_HIGH_RANGE_FSR_4000DPS,
    NUM_ICM_HIGH_RANGE_GYRO_FSR
};

enum clock_sel_e {
    INV_CLK_INTERNAL = 0,
    INV_CLK_PLL,
    NUM_CLK
};

enum accel_fsr_e {
    INV_FSR_2G = 0,
    INV_FSR_4G,
    INV_FSR_8G,
    INV_FSR_16G,
    NUM_ACCEL_FSR
};

enum icm_high_range_accel_fsr_e {
    ICM_HIGH_RANGE_FSR_4G = 0,
    ICM_HIGH_RANGE_FSR_8G,
    ICM_HIGH_RANGE_FSR_16G,
    ICM_HIGH_RANGE_FSR_32G,
    NUM_ICM_HIGH_RANGE_ACCEL_FSR
};

typedef enum {
    GYRO_OVERFLOW_NONE = 0x00,
    GYRO_OVERFLOW_X = 0x01,
    GYRO_OVERFLOW_Y = 0x02,
    GYRO_OVERFLOW_Z = 0x04
} gyroOverflow_e;

typedef struct gyroDeviceConfig_s {
    int8_t index;
    uint8_t busType;
    uint8_t spiBus;
    ioTag_t csnTag;
    uint8_t i2cBus;
    uint8_t i2cAddress;
    ioTag_t extiTag;
    uint8_t alignment;        // sensor_align_e
    //sensorAlignment_t customAlignment;
} gyroDeviceConfig_t;

struct gyroDev_s;
struct gyroDeviceConfig_s;

bool    mpuDetect(struct gyroDev_s *gyro, const struct gyroDeviceConfig_s *config);
uint8_t mpuGyroDLPF(struct gyroDev_s *gyro);
void    mpuGyroInit(struct gyroDev_s *gyro);
void    mpuPreInit(gyroDeviceConfig_t * config);
bool    mpuGyroRead(struct gyroDev_s *gyro);
bool    mpuGyroReadSPI(struct gyroDev_s *gyro);
uint8_t mpuGyroReadRegister(const extDevice_t *dev, uint8_t reg);

struct accDev_s;

bool mpuAccRead(struct accDev_s *acc);
bool mpuAccReadSPI(struct accDev_s *acc);

bool gyroSyncCheckUpdate(gyroDev_t *gyro);
uint16_t gyroSetSampleRate(gyroDev_t *gyro);

// API for specific MPU sensor
bool    mpuBusAccDetect(accDev_t *acc);
uint8_t mpuBusDetect(const extDevice_t *dev);
bool    mpuBusGyroDetect(gyroDev_t *gyro);