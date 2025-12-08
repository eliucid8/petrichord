#pragma once

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define AW9523_ADDR 0x58 
#define GET_REG_CHIPID 0x10
#define GET_PORT0 0x00
#define GET_PORT1 0x01
#define CONFIG_PORT0 0x04
#define CONFIG_PORT1 0x05
#define INT_PORT0 0x06
#define INT_PORT1 0x07



// bool io_extender_init(i2c_inst_t* i2c_channel);

// uint16_t io_extender_read_ports(i2c_inst_t* i2c_channel);