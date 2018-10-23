#pragma once

void i2c_init(void);
unsigned char i2c_start(unsigned char address);
unsigned char i2c_rep_start(unsigned char address);
void i2c_stop(void);
unsigned char i2c_write( unsigned char data );
unsigned char i2c_readAck(void);
unsigned char i2c_readNak(void);
// Higher level functions (by urjaman):
uint8_t i2c_read_regs(uint8_t dev, uint8_t reg, uint8_t cnt, uint8_t* buf);
uint8_t i2c_write_regs(uint8_t dev, uint8_t reg, uint8_t cnt, uint8_t* buf);
uint8_t i2c_read_reg(uint8_t dev, uint8_t reg, uint8_t *val);
uint8_t i2c_write_reg(uint8_t dev, uint8_t reg, uint8_t val);
