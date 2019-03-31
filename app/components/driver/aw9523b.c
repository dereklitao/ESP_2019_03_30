#include "aw9523b.h"

#define I2C_MASTER_SCL_IO 2         /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO 14        /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM I2C_NUM_0    /*!< I2C port number for master dev */
#define I2C_MASTER_TX_BUF_DISABLE 0 /*!< I2C master do not need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0 /*!< I2C master do not need buffer */

static void aw9523b_gpio_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = 1;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = 1;
    i2c_driver_install(i2c_master_port, conf.mode);
    i2c_param_config(i2c_master_port, &conf);

    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << 5);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);
}

static void i2c_master_aw9523b_write(uint8_t reg_address, uint8_t value)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, 0xB6, 1);
    i2c_master_write_byte(cmd, reg_address, 1);
    i2c_master_write_byte(cmd, value, 1);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
}

uint8_t i2c_master_aw9523b_read(uint8_t reg_address)
{
    uint8_t data = 0;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, 0xB6, 1);
    i2c_master_write_byte(cmd, reg_address, 1);
    i2c_master_stop(cmd);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, 0xB7, 1);
    i2c_master_read_byte(cmd, &data, 1);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return data;
}

void csro_aw9523b_init(void)
{
    aw9523b_gpio_init();
    i2c_master_aw9523b_write(0x13, 0x00);
}

void csro_aw9523b_test(void)
{
    static int count = 0;

    uint8_t data;
    data = i2c_master_aw9523b_read(0x23);
    debug("Chip 23 = %d\r\n", data);
    data = i2c_master_aw9523b_read(0x13);
    debug("Chip 13 = %d\r\n", data);

    count = (count + 1) % 6;
    if (count == 0)
    {
        i2c_master_aw9523b_write(0x20, 32);
        i2c_master_aw9523b_write(0x21, 32);
        i2c_master_aw9523b_write(0x22, 32);
        i2c_master_aw9523b_write(0x23, 32);
        i2c_master_aw9523b_write(0x2C, 32);
        i2c_master_aw9523b_write(0x2D, 32);
        i2c_master_aw9523b_write(0x2E, 32);
        i2c_master_aw9523b_write(0x2F, 32);
    }
    else if (count == 3)
    {
        i2c_master_aw9523b_write(0x20, 0);
        i2c_master_aw9523b_write(0x21, 0);
        i2c_master_aw9523b_write(0x22, 0);
        i2c_master_aw9523b_write(0x23, 0);
        i2c_master_aw9523b_write(0x2C, 0);
        i2c_master_aw9523b_write(0x2D, 0);
        i2c_master_aw9523b_write(0x2E, 0);
        i2c_master_aw9523b_write(0x2F, 0);
    }
}
