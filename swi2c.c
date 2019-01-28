
#include "main.h"
#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>
#include "i2c.h"
#include "console.h"
#include "lib.h"

#if 0
#define EDLY _delay_ms(15);
#else
#define EDLY asm volatile("nop");
#endif

// PC3 = SCL
// PC2 = SDA

// Hardware-specific support functions that MUST be customized:

static void I2C_delay() {
	_delay_us(3); // this should give <=50kHz ... or so.
}

static uint8_t read_SCL(void) {
	// Set SCL as input and return current level of line, 0 or 1
	DDRC &= ~_BV(3);
	return PINC&_BV(3);
}

static uint8_t read_SDA(void) {
	// Set SDA as input and return current level of line, 0 or 1
	DDRC &= ~_BV(2);
	return PINC&_BV(2);
}

static void clear_SCL(void) {
	 // Actively drive SCL signal low
	 DDRC |= _BV(3);
}

static void clear_SDA(void) {
	// Actively drive SDA signal low
	DDRC |= _BV(2);
}

#define STARTED 1
#define TIMEOUT 2

static uint8_t status = 0;

void swi2c_init(void) {
	DDRC &= ~_BV(2);
	DDRC &= ~_BV(3);
	PORTC &= ~_BV(2);
	PORTC &= ~_BV(3);
}

static void Wait_SCL1(uint8_t id) {
	uint16_t i=0;
	do {
		if (read_SCL()) return;
		_delay_us(1);
	} while (++i);
	status |= TIMEOUT;
	(void)id;
//	sendstr_P(PSTR(" SCL_TO "));
//	luint2outdual(id);
}

static void i2c_start_cond(void) {
  if (status&STARTED) { // if started, do a restart cond
    // set SDA to 1
    read_SDA();
    I2C_delay();
    Wait_SCL1(0);
    // Repeated start setup time, minimum 4.7us
    I2C_delay();
  }
  // SCL is high, set SDA from 1 to 0.
  clear_SDA();
  I2C_delay();
  clear_SCL();
  status |= STARTED;
  return;
}

void swi2c_stop(void){
  // set SDA to 0
  clear_SDA();
  I2C_delay();
  Wait_SCL1(1);
  // Stop bit setup time, minimum 4us
  I2C_delay();
  // SCL is high, set SDA from 0 to 1
  read_SDA();
  I2C_delay();
  status &= ~STARTED;
  EDLY;
  return;
}

// Write a bit to I2C bus
static void i2c_write_bit(uint8_t bit) {
  if (bit) {
    read_SDA();
  } else {
    clear_SDA();
  }
  I2C_delay();
  Wait_SCL1(2);
  // SCL is high, now data is valid
  I2C_delay();
  clear_SCL();
  I2C_delay();
}

// Read a bit from I2C bus
static uint8_t i2c_read_bit(void) {
  uint8_t bit;
  // Let the slave drive data
  read_SDA();
  I2C_delay();
  Wait_SCL1(3);
  // SCL is high, now data is valid
  bit = !!read_SDA();
  I2C_delay();
  clear_SCL();
  return bit;
}

static uint8_t i2c_is_error(void) {
	if (status&TIMEOUT) {
		status &= ~TIMEOUT;
		return 1;
	}
	return 0;
}

// Write a byte to I2C bus. Return 0 if ack by the slave.
uint8_t swi2c_write(uint8_t byte) {
  uint8_t bit;
  uint8_t nack;
  for (bit = 0; bit < 8; bit++) {
    i2c_write_bit(byte & 0x80);
    byte <<= 1;
  }
  nack = i2c_read_bit();
  EDLY;
  uint8_t err = i2c_is_error() || nack;
  if (err) swi2c_stop();
  return err;
}


uint8_t swi2c_start(uint8_t addr) {
	i2c_start_cond();
	if ((i2c_is_error())||((status&STARTED)==0)) return 1;
	EDLY;
	uint8_t nack = swi2c_write(addr);
	if (nack) {
		swi2c_stop();
		return 2;
	}
	return 0;
}

uint8_t swi2c_rep_start(uint8_t addr) {
	return swi2c_start(addr);
}

// Read a byte from I2C bus
static uint8_t i2c_read_(uint8_t nack) {
  uint8_t byte = 0;
  uint8_t bit;
  for (bit = 0; bit < 8; bit++) {
    byte = (byte << 1) | i2c_read_bit();
  }
  i2c_write_bit(nack);
  EDLY;
  return byte;
}

uint8_t swi2c_readAck(void) {
	return i2c_read_(0);
}

uint8_t swi2c_readNak(void) {
	return i2c_read_(1);
}


/************************************************************************
 Perform register-based I2C device operations. This reads cnt regs
 starting with reg into buf from dev. Return values per i2c_start().
************************************************************************/
uint8_t swi2c_read_regs(uint8_t dev, uint8_t reg, uint8_t cnt, uint8_t* buf) {
	uint8_t x;
	if ((x = swi2c_start(dev&0xFE))) return x;
	if (swi2c_write(reg)) return 1;
	if ((x = swi2c_rep_start(dev|0x01))) return x;
	uint8_t offset=0;
	while (cnt>1) {
		buf[offset++] = swi2c_readAck();
		cnt--;
	}
	if (cnt) buf[offset] = swi2c_readNak();
	swi2c_stop();
	return i2c_is_error();
}


/************************************************************************
 Perform register-based I2C device operations. This writes cnt regs
 starting with reg into dev from buf. Return values per i2c_start().
************************************************************************/
uint8_t swi2c_write_regs(uint8_t dev, uint8_t reg, uint8_t cnt, uint8_t* buf) {
	uint8_t x;
	if ((x = swi2c_start(dev&0xFE))) return x;
	if (swi2c_write(reg)) {
	    sendstr_P(PSTR(" write reg fail "));
	    return 1;
	}
	for (uint8_t i=0;i<cnt;i++) {
		if (swi2c_write(buf[i])) {
		    sendstr_P(PSTR(" write data fail i="));
		    luint2outdual(i);
		    sendstr_P(PSTR(" d[i]="));
		    luint2outdual(buf[i]);
		    sendstr_P(PSTR(" d[i-1]="));
		    luint2outdual(buf[i-1]);
		    return 1;
		}
	}
	swi2c_stop();
	return i2c_is_error();
}



uint8_t swi2c_writem(uint8_t dev, uint8_t cnt, uint8_t* buf) {
	uint8_t x;
	if ((x = swi2c_start(dev&0xFE))) return x;
	for (uint8_t i=0;i<cnt;i++) {
		if (swi2c_write(buf[i])) {
		    sendstr_P(PSTR(" write data fail i="));
		    luint2outdual(i);
		    sendstr_P(PSTR(" d[i]="));
		    luint2outdual(buf[i]);
		    sendstr_P(PSTR(" d[i-1]="));
		    luint2outdual(buf[i-1]);
		    return 1;
		}
	}
	swi2c_stop();
	return i2c_is_error();
}


/* Simple 1-byte transfer versions of the above functions. */
uint8_t swi2c_read_reg(uint8_t dev, uint8_t reg, uint8_t *val) {
	return swi2c_read_regs(dev,reg,1,val);
}


uint8_t swi2c_write_reg(uint8_t dev, uint8_t reg, uint8_t val) {
	return swi2c_write_regs(dev,reg,1,&val);
}

/************************************************************************
 Just read from I2C dev (no register set). Return values per i2c_start().
************************************************************************/
uint8_t swi2c_read(uint8_t dev, uint8_t cnt, uint8_t* buf) {
	uint8_t x;
	if ((x = swi2c_start(dev|0x01))) return x;
	uint8_t offset=0;
	while (cnt>1) {
		buf[offset++] = swi2c_readAck();
		cnt--;
	}
	if (cnt) buf[offset] = swi2c_readNak();
	swi2c_stop();
	return 0;
}
