#include "main.h"
#include "timer.h"
#include "buttons.h"
#include "uart.h"

void buttons_init(void) {
	PORTD |= _BV(2);
	PORTD |= _BV(3); // enable pull-ups
}

uint8_t buttons_get_v(void) {
	uint8_t rv = (uint8_t)~PIND;
	rv = (uint8_t)rv & (uint8_t)(_BV(2)|_BV(3));
	rv = (uint8_t)rv >> (uint8_t)2;
	return rv;
}

uint8_t buttons_get(void) {
	uint8_t v = buttons_get_v();
	if (!v) return 0;
	timer_activity();
	timer_delay_ms(125);
	for(;;) {
		uint8_t sv;
		timer_delay_ms(30);
		sv = buttons_get_v();
		if (sv == v) {
			return v;
		}
		v = sv;
	}
}
