#include "main.h"
#include "timer.h"
#include "ams2302.h"

#define DHTPIN PINB
#define DHTPRT PORTB
#define DHTDDR DDRB
#define DHTBIT 0

static uint8_t ams_data[5];
static uint32_t ams_timestamp;
static PGM_P ams_err;

static void dhtpulse(void)
{
	DHTPRT |=  _BV(DHTBIT);
	_delay_us(1);
	DHTDDR |=  _BV(DHTBIT);
	DHTPRT &= ~_BV(DHTBIT);
	_delay_ms(1.1);
	DHTPRT |=  _BV(DHTBIT);
	DHTDDR &= ~_BV(DHTBIT);
	_delay_us(10);
}

static int8_t dhtbit(void)
{
	uint8_t n, t, v;
	uint8_t b = timer_get_lin_us();
	n = b; /* In case of already zero */
	/* Wait for zero */
	while (DHTPIN & _BV(DHTBIT)) {
		n = timer_get_lin_us();
		t = n - b;
		if (t >= 100) return -1;
	};
	b = n;
	/* Wait for one */
	do {
		n = timer_get_lin_us();
		t = n - b;
		if (t >= 127) return -2;
	} while (!(DHTPIN & _BV(DHTBIT)));
	/* Wait for zero */
	b = n;
	do {
		n = timer_get_lin_us();
		v = n - b;
		if (v >= 200) return -3;
	} while (DHTPIN & _BV(DHTBIT));
	if (v > 50) return 1;
	return 0;
}

static PGM_P ams_read(void) {
	uint8_t dat[5];
	dhtpulse();
	if (dhtbit() < 0) {
		return PSTR("No response");
	}
	for (uint8_t n=0; n < 5; n++) {
		uint8_t d = 0;
		for (uint8_t i=0; i < 8; i++) {
			int8_t b = dhtbit();
			if (b < 0) {
				return PSTR("RX Failed");
			}
			d = d << 1;
			d |= b;
		}
		dat[n] = d;
	}
	uint8_t sum = dat[0] + dat[1] + dat[2] + dat[3];
	if (sum != dat[4]) {
		return PSTR("Checksum failure");
	}
	memcpy(ams_data, dat, 5);
	ams_timestamp = timer_get();
	return NULL;
/*	
	uint16_t temp = (dat[2] << 8) | dat[3];
	uint16_t rh = (dat[0] << 8) | dat[1];
	sendstr_P(PSTR("Temp: "));
	luint2outdual(temp);
	sendstr_P(PSTR("\r\nRH: "));
	luint2outdual(rh); */
}
	

void ams_run(void) {
	if ( (timer_get_1hzp()) && ( (timer_get() & 1) == 1) ) {
		ams_err = ams_read();
	}
}

void ams_init(void) {
	ams_err = PSTR("Too early");
	DHTPRT |= _BV(DHTBIT);
}

PGM_P ams_get(int16_t *tempC10, uint16_t *rh10, uint8_t max_age)
{
	uint32_t age = timer_get() - ams_timestamp;
	if ((!ams_timestamp)||(age > max_age)) return ams_err ? ams_err : PSTR("Too old");
	
	if (tempC10) {
		uint16_t wtemp = (ams_data[2] << 8) | ams_data[3];
		int16_t stemp = wtemp;
		if (wtemp & 0x8000) stemp = -(wtemp & 0x7FFF);
		*tempC10 = stemp;
	}
	
	if (rh10) {
		uint16_t rhv = (ams_data[0] << 8) | ams_data[1];
		*rh10 = rhv;
	}

	return NULL;
}

void make_v10_str(unsigned char*buf, int16_t t10) 
{
	uint16_t v10;
	if (t10 < 0) {
		*buf++ = '-';
		v10 = -t10;
	} else {
		v10 = t10;
	}
	uint8_t ldig = v10 % 10;
	uint16_t pv = v10 / 10;
	if (v10 >= 1000) {
		*buf++ = 0x30 | (pv / 100);
		pv = pv % 100;
	}
	if (v10 >= 100) {
		*buf++ = 0x30 | (pv / 10);
		pv = pv % 10;
	}
	*buf++ = 0x30 | pv;
	*buf++ = '.';
	*buf++ = 0x30 | ldig;
	*buf = 0;
}

