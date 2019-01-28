/* The RTC is a DS1307. This module is minimalistic, should work for lots of RTC's with little change. */
#include "main.h"
#include "time.h"
#include "i2c.h"
#include "rtc.h"

#define DS1307_I2C_ADDR 0xD0
#define RTC_I2C_ADDR DS1307_I2C_ADDR

static uint8_t readbcd(uint8_t bcd) {
	return (bcd>>4)*10+(bcd&0xF);
}
static uint8_t writebcd(uint8_t bin) {
	uint8_t bcd = bin%10;
	bcd |= (bin/10) << 4;
	return bcd;
}

static uint8_t rtc_is_ok=0;

// 0 = OK, nonzero = not ok
uint8_t rtc_read(struct mtm* tm) {
	uint8_t buf[10];
	if (i2c_read_regs(RTC_I2C_ADDR,0,10,buf)) {
		rtc_is_ok=0;
		return 1; // Not OK
	}
	if (buf[0]&0x80) {
		rtc_is_ok = 0;
		return 2; // VL, not OK
	}
	tm->sec = readbcd(buf[0]);
	tm->min = readbcd(buf[1]&0x7F);
	tm->hour = readbcd(buf[2]&0x3F);	
	tm->year = readbcd(buf[6]);
	tm->month = readbcd(buf[5]&0x1F);
	tm->day = readbcd(buf[4]&0x3F);
	uint8_t chkcentury = buf[9] ^ 0xFF;
	if ((buf[8]) && (buf[8] <= 2) && (buf[8] == chkcentury)) {
		/* Use first byte of SRAM for century -
		 * it will not work automatically, but atleast
		 * we should remember it correctly if set. */
		tm->year += (buf[8] * 100);
	}
	rtc_is_ok = 1;
	return 0; // OK
}


void rtc_write(struct mtm* tm) {
	uint8_t buf[10];
	buf[0] = writebcd(tm->sec);
	buf[1] = writebcd(tm->min);
	buf[2] = writebcd(tm->hour);
	buf[4] = writebcd(tm->day);
	buf[3] = 1; // WeekDay, unused.
	buf[5] = writebcd(tm->month);
	buf[6] = writebcd(tm->year % 100);
	buf[7] = 0; /* Controls, dont use SQWO or OUT */
	buf[8] = tm->year/100;
	buf[9] = (tm->year/100) ^ 0xFF; // a bit of a checksum so the century doesnt get used accidentally
	if (i2c_write_regs(RTC_I2C_ADDR,0,10,buf)) {
		rtc_is_ok = 0;
		return; // Not OK
	}
	rtc_is_ok = 1;
	return;
}



uint8_t rtc_valid(void) {
	return rtc_is_ok;
}
