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

    err = bno055_set_operation_mode(BNO055_OPERATION_MODE_ACCGYRO);
    if(err) {
        printf("IMU_Controller: operation mode request failed.\n");
        return false;
    }

    printf("IMU_Controller: initialized succesfully.\n");

    return true;
}

bool IMU_Controller::readGravityVector(struct imu_gravity_vector *dst) {

    struct bno055_accel_float_t accelData;
    s8 err = bno055_convert_float_accel_xyz_msq(&accelData);

    if(err) {
        printf("IMU_Controller: bno055 data read failiure %d\n", err);
        return false;
    }

    dst->x = accelData.x;
    dst->y = accelData.y;
    dst->z = accelData.z;

    return true;
}

void IMU_Controller::debugPrint() {
    struct imu_gravity_vector vec;

    if(readGravityVector(&vec)) {
        printf("IMU_Controller: X: %.2f, Y: %.2f, Z: %.2f\n", vec.x, vec.y, vec.z);
    }
    
}