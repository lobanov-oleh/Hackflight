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

#include <imus/fusion.h>
#include <imus/fusion/mpudev.h>

class Mpu6000 : public FusionImu {

    private:

        // RF = Register Flag
        static const uint8_t RF_DATA_RDY_EN = 1 << 0;

        static const uint8_t CONFIG = 0x1A;

        // 1 MHz max SPI frequency for initialisation
        static const uint32_t MAX_SPI_INIT_CLK_HZ = 1000000;

        // 20 MHz max SPI frequency
        static const uint32_t MAX_SPI_CLK_HZ = 20000000;

        // Any interrupt interval less than this will be recognised as the
        // short interval of ~79us
        static const uint8_t SHORT_THRESHOLD = 82 ;

        static const uint8_t WHO_AM_I_CONST   = 0x68;

        // RA = Register Address;

        static const uint8_t RA_PRODUCT_ID         = 0x0C;
        static const uint8_t RA_SMPLRT_DIV         = 0x19;
        static const uint8_t RA_GYRO_CONFIG        = 0x1B;
        static const uint8_t RA_ACCEL_CONFIG       = 0x1C;
        static const uint8_t RA_INT_PIN_CFG        = 0x37;
        static const uint8_t RA_INT_ENABLE         = 0x38;
        static const uint8_t RA_GYRO_XOUT_H        = 0x43;
        static const uint8_t RA_USER_CTRL          = 0x6A;
        static const uint8_t RA_PWR_MGMT_1         = 0x6B;
        static const uint8_t RA_PWR_MGMT_2         = 0x6C;
        static const uint8_t RA_SIGNAL_PATH_RESET  = 0x68;
        static const uint8_t RA_WHO_AM_I           = 0x75;

        // The gyro buffer is split 50/50, the first half for the transmit buffer, the
        // second half for the receive buffer This buffer is large enough for the gyros
        // currently supported in imu_mpu.c but should be reviewed id other gyro
        // types are supported with SPI DMA.
        static const uint8_t GYRO_BUF_SIZE = 32;

        // Bits
        static const uint8_t BIT_SLEEP                   = 0x40;
        static const uint8_t BIT_H_RESET                 = 0x80;
        static const uint8_t BITS_CLKSEL                 = 0x07;
        static const uint8_t CLK_SEL_PLLGYROX            = 0x01;
        static const uint8_t CLK_SEL_PLLGYROZ            = 0x03;
        static const uint8_t EXT_SYNC_GYROX              = 0x02;
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

        // Product ID Description for MPU6000 high 4 bits low 4 bits Product
        // Name Product Revision
        static const uint8_t ES_REV_C4 = 0x14;
        static const uint8_t ES_REV_C5 = 0x15;
        static const uint8_t ES_REV_D6 = 0x16;
        static const uint8_t ES_REV_D7 = 0x17;
        static const uint8_t ES_REV_D8 = 0x18;
        static const uint8_t REV_C4    = 0x54;
        static const uint8_t REV_C5    = 0x55;
        static const uint8_t REV_D6    = 0x56;
        static const uint8_t REV_D7    = 0x57;
        static const uint8_t REV_D8    = 0x58;
        static const uint8_t REV_D9    = 0x59;
        static const uint8_t REV_D10   = 0x5A;

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

        typedef struct {
            int8_t index;
            busType_e busType;
            uint8_t spiBus;
            ioTag_t csnTag;
            uint8_t i2cBus;
            uint8_t i2cAddress;
            ioTag_t extiTag;
            uint8_t alignment;
        } gyroDeviceConfig_t;

        mpuSensor_e busDetect(const extDevice_t *dev);

        bool detectSPISensorsAndUpdateDetectionResult(const gyroDeviceConfig_t *config);

        bool mpuDetect(const Mpu6000::gyroDeviceConfig_t *config);

        static bool gyroRead(void);

        static bool gyroReadSPI(void);

        virtual void devInit(uint8_t interruptPin) override;

        virtual bool devGyroIsReady(void) override;

        virtual int16_t devReadRawGyro(uint8_t k) override;

    public:

         Mpu6000(uint8_t interruptPin, uint16_t gyroScale)
            : FusionImu(interruptPin, gyroScale)
        {
        }

}; // class Mpu6000

mpuSensor_e mpuBusDetect(const extDevice_t *dev);
bool        mpuBusGyroDetect(gyroDev_t *gyro);
