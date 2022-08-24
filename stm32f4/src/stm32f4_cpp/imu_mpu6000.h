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

#include "imu_mpu.h"

class ImuMpu6000 : public ImuMpu {

    private:

        // 16.384 dps/lsb scalefactor for 2000dps sensors
        static constexpr float GYRO_SCALE_2000DPS = 2000.0f / (1 << 15);

        // 8.192 dps/lsb scalefactor for 4000dps sensors
        static constexpr float GYRO_SCALE_4000DPS = 4000.0f / (1 << 15);

        // RF = Register Flag
        static const uint8_t MPU_RF_DATA_RDY_EN = 1 << 0;

        static const uint8_t MPU6000_CONFIG =  0x1A;

        // 1 MHz max SPI frequency for initialisation
        static const uint32_t MAX_SPI_INIT_CLK_HZ = 1000000;

        // 20 MHz max SPI frequency
        static const uint32_t MAX_SPI_CLK_HZ = 20000000;

        // Any interrupt interval less than this will be recognised as the short
        // interval of ~79us
        static const uint32_t SHORT_THRESHOLD = 82 ;

        // Bits
        static const uint8_t BIT_SLEEP                   = 0x40;
        static const uint8_t BIT_H_RESET                 = 0x80;
        static const uint8_t BITS_CLKSEL                 = 0x07;
        static const uint8_t MPU_CLK_SEL_PLLGYROX        = 0x01;
        static const uint8_t MPU_CLK_SEL_PLLGYROZ        = 0x03;
        static const uint8_t MPU_EXT_SYNC_GYROX          = 0x02;
        static const uint8_t BITS_FS_250DPS              = 0x00;
        static const uint8_t BITS_FS_500DPS              = 0x08;
        static const uint8_t BITS_FS_1000DPS             = 0x10;
        static const uint8_t BITS_FS_2000DPS             = 0x18;
        static const uint8_t BITS_FS_2G                  = 0x00;
        static const uint8_t BITS_FS_4G                  = 0x08;
        static const uint8_t BITS_FS_8G                  = 0x10;
        static const uint8_t BITS_FS_16G                 = 0x18;
        static const uint8_t BITS_FS_MASK                = 0x18;
        static const uint8_t BITS_DLPF_CFG_256HZ         = 0x00;
        static const uint8_t BITS_DLPF_CFG_188HZ         = 0x01;
        static const uint8_t BITS_DLPF_CFG_98HZ          = 0x02;
        static const uint8_t BITS_DLPF_CFG_42HZ          = 0x03;
        static const uint8_t BITS_DLPF_CFG_20HZ          = 0x04;
        static const uint8_t BITS_DLPF_CFG_10HZ          = 0x05;
        static const uint8_t BITS_DLPF_CFG_5HZ           = 0x06;
        static const uint8_t BITS_DLPF_CFG_2100HZ_NOLPF  = 0x07;
        static const uint8_t BITS_DLPF_CFG_MASK          = 0x07;
        static const uint8_t BIT_INT_ANYRD_2CLEAR        = 0x10;
        static const uint8_t BIT_RAW_RDY_EN              = 0x01;
        static const uint8_t BIT_I2C_IF_DIS              = 0x10;
        static const uint8_t BIT_INT_STATUS_DATA         = 0x01;
        static const uint8_t BIT_GYRO                    = 0x04;
        static const uint8_t BIT_ACC                     = 0x02;
        static const uint8_t BIT_TEMP                    = 0x01;
        
        // Product ID Description for MPU6000;
        // high 4 bits low 4 bits;
        // Product Name Product Revision;
        static const uint8_t ES_REV_C4 = 0x14;
        static const uint8_t ES_REV_C5 = 0x15;
        static const uint8_t ES_REV_D6 = 0x16;
        static const uint8_t ES_REV_D7 = 0x17;
        static const uint8_t ES_REV_D8 = 0x18;
        static const uint8_t REV_C4 = 0x54;
        static const uint8_t REV_C5 = 0x55;
        static const uint8_t REV_D6 = 0x56;
        static const uint8_t REV_D7 = 0x57;
        static const uint8_t REV_D8 = 0x58;
        static const uint8_t REV_D9 = 0x59;
        static const uint8_t REV_D10 = 0x5A;

    public:

        ImuMpu6000(uint8_t interruptPin) 
            : ImuMpu(interruptPin)
        {
        }

}; // class ImuMpu6000


#if 0

static void mpu6000AccAndGyroInit(gyroDev_t *gyro)
{
    spiSetClkDivisor(&gyro->dev, spiCalculateDivider(MPU6000_MAX_SPI_INIT_CLK_HZ));

    // Device was already reset during detection so proceed with configuration

    // Clock Source PPL with Z axis gyro reference
    spiWriteReg(&gyro->dev, MPU_RA_PWR_MGMT_1, MPU_CLK_SEL_PLLGYROZ);
    delayMicroseconds(15);

    // Disable Primary I2C Interface
    spiWriteReg(&gyro->dev, MPU_RA_USER_CTRL, BIT_I2C_IF_DIS);
    delayMicroseconds(15);

    spiWriteReg(&gyro->dev, MPU_RA_PWR_MGMT_2, 0x00);
    delayMicroseconds(15);

    // Accel Sample Rate 1kHz
    // Gyroscope Output Rate =  1kHz when the DLPF is enabled
    spiWriteReg(&gyro->dev, MPU_RA_SMPLRT_DIV, 0);
    delayMicroseconds(15);

    // Gyro +/- 2000 DPS Full Scale
    spiWriteReg(&gyro->dev, MPU_RA_GYRO_CONFIG, INV_FSR_2000DPS << 3);
    delayMicroseconds(15);

    // Accel +/- 16 G Full Scale
    spiWriteReg(&gyro->dev, MPU_RA_ACCEL_CONFIG, INV_FSR_16G << 3);
    delayMicroseconds(15);

    spiWriteReg(&gyro->dev, MPU_RA_INT_PIN_CFG, 0 << 7 | 0 << 6 | 0 << 5 | 1 << 4 | 0 << 3 | 0 << 2 | 0 << 1 | 0 << 0);  // INT_ANYRD_2CLEAR
    delayMicroseconds(15);

    spiWriteReg(&gyro->dev, MPU_RA_INT_ENABLE, MPU_RF_DATA_RDY_EN);
    delayMicroseconds(15);

    spiSetClkDivisor(&gyro->dev, spiCalculateDivider(MPU6000_MAX_SPI_CLK_HZ));
    delayMicroseconds(1);
}


void mpu6000SpiGyroInit(gyroDev_t *gyro)
{
    mpuGyroInit(gyro);

    mpu6000AccAndGyroInit(gyro);

    spiSetClkDivisor(&gyro->dev, spiCalculateDivider(MPU6000_MAX_SPI_INIT_CLK_HZ));

    // Accel and Gyro DLPF Setting
    spiWriteReg(&gyro->dev, MPU6000_CONFIG, mpuGyroDLPF(gyro));
    delayMicroseconds(1);

    spiSetClkDivisor(&gyro->dev, spiCalculateDivider(MPU6000_MAX_SPI_CLK_HZ));

    mpuGyroRead(gyro);

    if (((int8_t)gyro->adcRaw[1]) == -1 && ((int8_t)gyro->adcRaw[0]) == -1) {
        systemFailureMode(FAILURE_GYRO_INIT_FAILED);
    }
}

// ----------------------------------------------------------------------------

uint8_t mpuBusDetect(const extDevice_t *dev)
{

    spiSetClkDivisor(dev, spiCalculateDivider(MPU6000_MAX_SPI_INIT_CLK_HZ));

    // reset the device configuration
    spiWriteReg(dev, MPU_RA_PWR_MGMT_1, BIT_H_RESET);
    delay(100);  // datasheet specifies a 100ms delay after reset

    // reset the device signal paths
    spiWriteReg(dev, MPU_RA_SIGNAL_PATH_RESET, BIT_GYRO | BIT_ACC | BIT_TEMP);
    delay(100);  // datasheet specifies a 100ms delay after signal path reset


    const uint8_t whoAmI = spiReadRegMsk(dev, MPU_RA_WHO_AM_I);
    delayMicroseconds(1); // Ensure CS high time is met which is violated on H7 without this delay
    uint8_t detectedSensor = MPU_NONE;

    if (whoAmI == MPU6000_WHO_AM_I_CONST) {
        const uint8_t productID = spiReadRegMsk(dev, MPU_RA_PRODUCT_ID);

        /* look for a product ID we recognise */

        // verify product revision
        switch (productID) {
        case MPU6000ES_REV_C4:
        case MPU6000ES_REV_C5:
        case MPU6000_REV_C4:
        case MPU6000_REV_C5:
        case MPU6000ES_REV_D6:
        case MPU6000ES_REV_D7:
        case MPU6000ES_REV_D8:
        case MPU6000_REV_D6:
        case MPU6000_REV_D7:
        case MPU6000_REV_D8:
        case MPU6000_REV_D9:
        case MPU6000_REV_D10:
            detectedSensor = MPU_60x0_SPI;
        }
    }

    spiSetClkDivisor(dev, spiCalculateDivider(MPU6000_MAX_SPI_CLK_HZ));
    return detectedSensor;
}

bool mpuBusAccDetect(accDev_t *acc)
{
    if (acc->mpuDetectionResult.sensor != MPU_60x0_SPI) {
        return false;
    }

    acc->readFn = mpuAccReadSPI;

    return true;
}

bool mpuBusGyroDetect(gyroDev_t *gyro)
{
    if (gyro->mpuDetectionResult.sensor != MPU_60x0_SPI) {
        return false;
    }

    gyro->initFn = mpu6000SpiGyroInit;
    gyro->readFn = mpuGyroReadSPI;
    gyro->scaleDps = 2000;
    gyro->gyroShortPeriod = systemClockMicrosToCycles(MPU6000_SHORT_THRESHOLD);
    return true;
}
#endif
