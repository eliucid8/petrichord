#include "io_extender.h"

bool io_write_reg(i2c_inst_t* i2c_channel, uint8_t dev_addr, uint8_t reg, const uint8_t *data, size_t len) {
    uint8_t buf[1 + len];
    buf[0] = reg;
    for (unsigned int i = 0; i < len; ++i) buf[1 + i] = data[i];
    int ret = i2c_write_blocking(i2c_channel, dev_addr, buf, 1 + len, false);
    return (ret >= 0);
}

bool io_read_reg(i2c_inst_t* i2c_channel, uint8_t dev_addr, uint8_t reg, uint8_t *out, size_t len) {
    int ret = i2c_write_blocking(i2c_channel, dev_addr, &reg, 1, true); // send reg, keep master
    if (ret < 0) return false;
    ret = i2c_read_blocking(i2c_channel, dev_addr, out, len, false);
    return (ret >= 0);
}

bool io_extender_init(i2c_inst_t* i2c_channel) {
    printf("Initializing IO Extender...\n");
    uint8_t chip_id = 0;   
    if (io_read_reg(i2c_channel, AW9523_ADDR, GET_REG_CHIPID, &chip_id, 1)) {
    } else {
        return false;
    }

    // Configure all pins as inputs
    uint8_t config_data[2] = {0xFF, 0xFF}; // Set all pins as inputs
    if(io_write_reg(i2c_channel, AW9523_ADDR, CONFIG_PORT0, config_data, 1) &&
       io_write_reg(i2c_channel, AW9523_ADDR, CONFIG_PORT1, config_data + 1, 1)) {

    } else {
        return false;
    }

    printf("IO Extender initialized with Chip ID: 0x%02X on channel %p\n", chip_id, i2c_channel);
    return true;
}

uint16_t io_extender_read_ports(i2c_inst_t* i2c_channel) {
    printf("Reading IO Extender ports on channel %p...\n", i2c_channel);
    uint8_t port_data[2] = {0, 0};
    if (io_read_reg(i2c_channel, AW9523_ADDR, GET_PORT0, &port_data[0], 1) &&
        io_read_reg(i2c_channel, AW9523_ADDR, GET_PORT1, &port_data[1], 1)) {
        printf("AW9523 GPIO States - Port0: 0x%02X, Port1: 0x%02X\n", port_data[0], port_data[1]);
    } else {
        printf("Failed to read AW9523 GPIO states\n");
        return 0;
    }

    return (static_cast<uint16_t>(port_data[1]) << 8) | port_data[0];
}