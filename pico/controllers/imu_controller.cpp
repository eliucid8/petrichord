#include "imu_controller.h"
#include <cstdio>

IMU_Controller::IMU_Controller() 
{
}

bool IMU_Controller::init(i2c_inst_t *i2c_port) {
    s8 err = bno055_pico_init(&bno_, i2c_port, BNO055_I2C_ADDR1);

    if(err) {
        err = bno055_pico_init(&bno_, i2c_port, BNO055_I2C_ADDR2);
    }

    if(err) {
        printf("IMU_Controller: BNO055 initialization failiure.\n");
        return false;
    }

    err = bno055_set_power_mode(BNO055_POWER_MODE_NORMAL);

    if(err) {
        printf("IMU_Controller: power mode request failed.\n");
        return false;
    }

    err = bno055_set_operation_mode(BNO055_OPERATION_MODE_NDOF);
    if(err) {
        printf("IMU_Controller: operation mode request failed.\n");
        return false;
    }

    printf("IMU_Controller: initialized succesfully.\n");

    return true;
}

bool IMU_Controller::readGravityVector(struct imu_xyz_data *dst) {

    bno055_gravity_float_t gravData;
    s8 err = bno055_convert_float_gravity_xyz_msq(&gravData);

    if(err) {
        // printf("IMU_Controller: bno055 gravity read failiure\n");
        return false;
    }

    dst->x = gravData.x;
    dst->y = gravData.y;
    dst->z = gravData.z;

    return true;
}

bool IMU_Controller::readLinearAccelerationVector(struct imu_xyz_data *dst) {
    bno055_accel_float_t vec;
    s8 err = bno055_convert_float_accel_xyz_msq(&vec);

    if(err) {
        // printf("IMUController: bno055 linear acceleration read failiure\n");
        return false;
    }

    dst->x = vec.x;
    dst->y = vec.y;
    dst->z = vec.z;

    return true;
}

bool IMU_Controller::readEulerCoordinates(struct imu_hrp_data *dst) {
    bno055_euler_float_t vec;
    s8 err = bno055_convert_float_euler_hpr_deg(&vec);

    if(err) {
        // printf("IMU Controller: bno055 euler coordinate read failiure\n");
        return false;
    }

    dst->h = vec.h;
    dst->r = vec.r;
    dst->p = vec.p;

    return true;
}

void IMU_Controller::debugPrint() {
    struct imu_xyz_data grav;
    struct imu_xyz_data acc;
    struct imu_hrp_data euler;

    if(readGravityVector(&grav)) {
        printf("IMU_Controller.readGravityVector: x: %f, y: %f, z: %f\n", grav.x, grav.y, grav.z);
    } else {
        printf("IMU_Controller.readGravityVector: read fail\n");
    }

    if(readEulerCoordinates(&euler)) {
        printf("IMU_Controller.readEulerCoordinates: h: %f, r: %f, p: %f\n", euler.h, euler.r, euler.p);
    } else {
        printf("IMU_Controller.readEulerCoordinates: read fail\n");
    }

    if(readLinearAccelerationVector(&acc)) {
        
        printf("IMU_Controller.readGravityVector: x: %f, y: %f, z: %f\n", acc.x, acc.y, acc.z);
    } else {
        printf("IMU_Controller.readGravityVector: read fail\n");
    }
    
}