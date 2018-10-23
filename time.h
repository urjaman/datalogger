#pragma once

 // time.c: Linear format is seconds since 00:00 1.1.TIME_EPOCH_YEAR, mtm format as below:
struct mtm {
	uint8_t year; // EPOCH==0
	uint8_t month; // 1-12
	uint8_t day; // 1-31
	uint8_t hour; // 0-23
	uint8_t min; // 0-59
	uint8_t sec; // 0-59
};
// Lindate is linear / 86400, granularity of day.
uint8_t month_days(uint8_t year, uint8_t month);
uint32_t mtm2linear(const struct mtm * tm);
uint24_t mtm2lindate(const struct mtm* tm);
void lindate2mtm(struct mtm*tm, uint24_t lindate);
void linear2mtm(struct mtm*tm, uint32_t lintime);
void lindate_string(unsigned char* buf, uint24_t lindate);

#define TIME_EPOCH_YEAR 2000
// Change that once per century or so :P
// 1.1.TIME_EPOCH_YEAR day as 0-6, mon-sun.
#define EPOCH_DAY_OF_WEEK 5
// That would be a saturday.
