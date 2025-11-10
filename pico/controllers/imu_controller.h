#pragma once

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "BNO055_driver/bno055.h"

struct imu_gravity_vector {
    float x;
    float y;
    float z;
};

class IMU_Controller {
    public:
        IMU_Controller();

        // Initialize BNO055 device and confirm liveness
        bool init(i2c_inst_t *i2c_port);

        bool readGravityVector(struct imu_gravity_vector *dst);

        void debugPrint();
    
    private:
        // i2c_inst_t *i2c_;
        struct bno055_t bno_;
        bool initialized_;
};