#include "eeprom.h"
#include "configs.h"

#include <stdio.h>
#include <stdlib.h>

#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "pico/binary_info.h"
#include "pico/stdlib.h"

#define RW 3


static int i2c_writeByte(uint8_t word_address, uint8_t *data){
    int address = gConfig.i2cAddress;
    int ret;
	if (gConfig.writeProtect){
        gpio_put(RW,1);
        gpio_put(RW,0);
    }
    ret = i2c_write_blocking(i2c_default, address, data, 2, false);
    return ret;

}

static int i2c_readByte(uint8_t word_address, uint8_t *data){
    int address = gConfig.i2cAddress;
    int ret;
    gpio_put(RW,1);
    ret = i2c_write_blocking(i2c_default, address, &word_address, 1, true);
    gpio_put(RW, 0);
    ret = i2c_read_blocking(i2c_default, address, data, 1, false);

    return ret;
}


void i2c_eeprom_writeImage(const uint8_t* buffer, size_t size){
	if (gConfig.writeProtect){
        gpio_put(RW,1);
        gpio_put(RW,0);
    }

    int pageSize = gConfig.pageSize;
    int address = gConfig.i2cAddress;

    for(int i = 0; i < size/16; i++){
        uint8_t* page = (uint8_t*)malloc(pageSize + 1);

        for(int j = 0; j <= pageSize;j++){
            if(j == 0){
                page[0] = i*pageSize;
            }else{
                page[j] = buffer[i+(j-1)];
            }

        }
        if(i != 0 && page[0] == 0x00){
            address = address + 0x01;
        }
        i2c_write_blocking(i2c_default, address ,page, 16+1, false);
        free(page);
        sleep_ms(gConfig.pageDelayMs);
        
    }
}

int i2c_eeprom_readImage(uint8_t* buffer, size_t size){
    int ret;
    ret = i2c_read_blocking(i2c_default, gConfig.i2cAddress, buffer, size, false);
    return ret;
}

bool i2c_eeprom_init(){
    int ret;
    uint8_t rxdata;
    bool EEPROMPresent = false;

    i2c_init(i2c_default, 200 * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
    gpio_init(RW);
    gpio_set_dir(RW, GPIO_OUT);
    gpio_put(RW, 0);
    
    ret = i2c_read_blocking(i2c_default, gConfig.i2cAddress, &rxdata, 1, false); //Dummy read to check if the EEPROM is responding
    if(ret >0){
        EEPROMPresent = true;
    }
    return EEPROMPresent;

}