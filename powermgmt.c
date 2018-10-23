#include "main.h"
#include "timer.h"
#include "buttons.h"
#include "lcd.h"
#include "tui.h"
#include "i2c.h"
#include "rtc.h"
#include "powermgmt.h"
#include "tui-lib.h"
#include <avr/wdt.h>

/* This is sort of a TUI-related but not just UI module, so decided to call it just poweroff. */
/* What we are doing would be called suspend-to-ram in the PC world, but for UI simplicity I'll call it poweroff... */

EMPTY_INTERRUPT(INT0_vect);
EMPTY_INTERRUPT(INT1_vect);

static void do_poweroff(void) {
	extern uint8_t timer_time_valid;
	// We will save settings so that a power failure in the long sleep wont hurt that much...
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	while (buttons_get()); // We will wake up immediately if we sleep while user is holding button...
	EIMSK = 3;
	sleep_mode(); // Bye bye cruel world... atleast for a while.
	// and we're alive again (it might have been months or years, think of that :P)
	EIMSK = 0;
	timer_time_valid = 0;
	set_sleep_mode(SLEEP_MODE_IDLE);
}

void tui_poweroff(void) {
	uint8_t rtc_fail=0;
	struct mtm rtc_before;
	struct mtm rtc_after;
	if (!tui_are_you_sure()) return;
	lcd_clear();
	lcd_puts_P(PSTR("POWER OFF IN 1s!"));
	for(uint8_t i=0;i<10;i++) timer_delay_ms(100);
	if (rtc_read(&rtc_before)) rtc_fail = 1;
	do_poweroff();
	if (rtc_read(&rtc_after)) rtc_fail = 1;
	if (rtc_fail) {
		tui_gen_message(PSTR("SLEEP DURATION"), PSTR("UNKNOWN :("));
	} else {
		uint32_t passed = mtm2linear(&rtc_after) - mtm2linear(&rtc_before);
		lcd_clear();
		lcd_puts_P(PSTR("SLEEP DURATION:"));
		tui_time_print(passed);
		tui_waitforkey();
	}
}

void pm_init(void) {
	/* This sequence both turns off WDT reset mode and sets the period to 125ms */
	wdt_reset();
	MCUSR &= ~_BV(WDRF);
	WDTCSR = _BV(WDCE) | _BV(WDE);
	WDTCSR = _BV(WDIF) | _BV(WDP1) | _BV(WDP0);
}

extern volatile uint16_t subsectimer;
extern volatile uint8_t timer_run_todo;
ISR(WDT_vect) {
	/* Add 125ms to the system timers... You're not supposed to do this ;p */
	WDTCSR |= _BV(WDIF);
	const uint16_t addcnt = SSTC/8;
	uint24_t ss = subsectimer;
	ss += addcnt;
	if (ss >= SSTC) {
		ss -= SSTC;
		timer_run_todo++;
	}
	subsectimer = ss;
}

extern volatile uint16_t adc_isr_out_cnt;

void low_power_mode(void) {
	cli();
	/* Uhh, if we are not idle, use the "idle" sleep mode instead of power-down. */
	if (!timer_get_idle()) goto idle;
	SMCR = _BV(SM1) | _BV(SE); /* sleep enable and set mode */
	EIMSK = 3;
	WDTCSR |= _BV(WDIE); /* Enable WDT for timing. */
	sei();
	sleep_cpu();
	EIMSK = 0;
	SMCR = 0; /* sleep disable */
	WDTCSR &= ~_BV(WDIE); /* Stop the WDT timing. */
	return;

idle:
	SMCR = _BV(SE);
	sei();
	sleep_cpu();
	SMCR = 0; /* sleep_disable */
}
