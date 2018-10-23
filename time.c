 #include "main.h"
 #include "time.h"
 #include "lib.h"
 // lib.c
static uint8_t is_leap(uint8_t y_in) {
	uint16_t year = (uint16_t)y_in+TIME_EPOCH_YEAR;
	if ((year%4)==0) {
		if ((year%100)==0) {
			if ((year%400)==0) {
				return 1;
			} else {
				return 0;
			}
		} else {
			return 1;
		}
	} else {
		return 0;
	}
}

static uint16_t year_days(uint8_t year) {
	if (is_leap(year)) return 366;
	return 365;
}

uint8_t month_days(uint8_t year, uint8_t month) {
	switch (month) {
		case 0: return 31;
		case 1:
			if (is_leap(year)) return 29;
			return 28;
		case 2: return 31;
		case 3: return 30;
		case 4: return 31;
		case 5: return 30;
		case 6: return 31;
		case 7: return 31;
		case 8: return 30;
		case 9: return 31;
		case 10: return 30;
		case 11: return 31;
	}
	return 30; // Error, but we will just roll with the average.
}

uint32_t mtm2linear(const struct mtm * tm) {
	uint32_t counter = mtm2lindate(tm);
	counter *= 24; // to hours
	counter += tm->hour;
	counter *= 60; // to minutes
	counter += tm->min;
	counter *= 60; // to seconds
	counter += tm->sec;
	return counter;
}

uint24_t mtm2lindate(const struct mtm *tm) {
	uint24_t counter;
	int8_t leaps=0;
	/* Based on the fact that our epoc is at a conjuction of the leap cycles... */
	/* If you need to adjust for EPOCH != 2000, maybe put tm->year + difference_to_2000 */
	/* into an uint16_t. */
	if (tm->year) {
		uint8_t m = tm->year - 1; /* tm->year >= 1, thus m>=0. */
		leaps = m / 4;
		leaps -= m / 100; /* m / 4 > m/ 100 ... */
		leaps++; /* because >=2001 */
	}
	counter = (365UL * tm->year) + leaps;

	uint8_t mon = tm->month-1;
	for (uint8_t am=0;am<mon;am++) {
		counter += month_days(tm->year, am);
	}
	counter += (tm->day-1);
	return counter;
}

void lindate2mtm(struct mtm*tm, uint24_t days) {
	uint8_t year;
	uint24_t days_compare=0;
	for(year=0;year<255;year++) {
		uint16_t yd = year_days(year);
		if ((days_compare+yd)>days) break;
		days_compare += yd;
	}
	tm->year = year;
	days = days - days_compare;
	uint8_t mon;
	days_compare=0;
	for(mon=0;mon<12;mon++) {
		uint8_t md = month_days(year,mon);
		if ((days_compare+md)>days) break;
		days_compare += md;
	}
	tm->month = mon+1;
	days = days - days_compare;
	tm->day = days+1;
}

void linear2mtm(struct mtm*tm, uint32_t lintime) {
	ldiv_t day_s = ldiv(lintime, 86400);
	ldiv_t min_s = ldiv(day_s.rem, 60);
	tm->sec = min_s.rem;
	div_t hour_s = div(min_s.quot, 60);
	tm->min = hour_s.rem;
	tm->hour = hour_s.quot;
	lindate2mtm(tm, day_s.quot);
}
