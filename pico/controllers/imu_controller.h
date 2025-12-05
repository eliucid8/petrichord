#pragma once

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "BNO055_driver/bno055.h"

struct imu_xyz_data {
    float x;
    float y;
    float z;
};

struct imu_hrp_data {
    float h;
    float r;
    float p;
};

class IMU_Controller {
    public:
        IMU_Controller();

        // Initialize BNO055 device and confirm liveness
        bool init(i2c_inst_t *i2c_port);

        bool readGravityVector(struct imu_xyz_data *dst);

        bool readLinearAccelerationVector(struct imu_xyz_data *dst);

        bool readEulerCoordinates(struct imu_hrp_data *dst);

        void debugPrint();

        bool initialized();
    
    private:
        // i2c_inst_t *i2c_;
        struct bno055_t bno_;
        bool initialized_;
};