#include "main.h"
#include "timer.h"

extern uint8_t timer_waiting;
volatile uint8_t timer_run_todo=0;
#define S(s) str(s)
#define str(s) #s
#define VL (((F_CPU+128)/256)&0xFF)
#define VH (((F_CPU+128)/256)/256)

ISR(TIMER0_OVF_vect, ISR_NAKED) {
	asm volatile (
	".comm subsectimer,2\n\t"
	"push r1\n\t"
	"in r1, __SREG__\n\t"
	"push r24\n\t"
	"push r25\n\t"
	"lds r24, subsectimer\n\t"
	"lds r25, subsectimer+1\n\t"
	"adiw r24, 1\n\t"
	"sts subsectimer, r24\n\t"
	"sts subsectimer+1, r25\n\t"
	"cpi r24, " S(VL) "\n\t"
	"ldi r24, " S(VH) "\n\t"
	"cpc r25, r24\n\t"
	"brcs 1f\n\t"
	"lds r24, timer_run_todo\n\t"
	"inc r24\n\t"
	"sts timer_run_todo, r24\n\t"
	"clr r24\n\t"
	"sts subsectimer, r24\n\t"
	"sts subsectimer+1, r24\n\t"
	"1:\n\t"
	"pop r25\n\t"
	"pop r24\n\t"
	"out __SREG__, r1\n\t"
	"pop r1\n\t"
	"reti\n\t"
	::);
}

uint16_t timer_get_subsectimer(void) {
	uint16_t rv;
	asm volatile (
	"cli\n\t"
	"lds %A0,subsectimer\n\t"
	"sei\n\t"
	"lds %B0,subsectimer+1\n\t"
	: "=r" (rv)
	: );
	return rv;
}

void timer_init(void) { // Timer0 is being run in PWM mode by the backlight init already, so only enable OVF interrupt
	timer_run_todo=0;
	timer_waiting=1;
	TCCR0A = _BV(WGM01) | _BV(WGM00);
	TCCR0B = _BV(CS00);
	TIMSK0 |= _BV(TOIE0);
}

uint8_t timer_getdec_todo(void) {
	uint8_t rv;
	cli();
	rv = timer_run_todo;
	if (rv) timer_run_todo = rv-1;
	sei();
	return rv;
}

uint24_t timer_get_linear_ss_time(void) {
	uint8_t todo;
	uint16_t sstimer;
	cli();
	todo = timer_run_todo;
	sstimer = timer_get_subsectimer(); // sei() happens here
	return (uint24_t)todo*SSTC + sstimer;
}

uint16_t timer_get_lin_ss_u16(void) {
	/* This is the above, but inline optimized and for u16 return. */
	uint8_t todo;
	uint16_t sstimer;
	cli();
	todo = timer_run_todo;
	asm (
	"lds %A0,subsectimer\n\t"
	"sei\n\t"
	"lds %B0,subsectimer+1\n\t"
	: "=r" (sstimer)
	: );
	if (likely(!todo)) return sstimer;
	if (likely(todo==1)) {
		return SSTC+sstimer;
	} else {
		return 0xFFFF;
	}
}

uint8_t timer_get_todo(void) {
	uint8_t rv;
	// cli is not needed here since timer_run_todo is uint8_t
	// and it is not modified here
	rv = timer_run_todo;
	return rv;
}
