/*
 * Copyright (C) 2009,2016 Urja Rannikko <urjaman@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "main.h"
#include "uart.h"

// UART MODULE START
typedef uint8_t urxbufoff_t;
typedef uint8_t utxbufoff_t;
uint8_t volatile uart_rcvbuf[UART_BUFLEN];
urxbufoff_t volatile uart_rcvwptr;
urxbufoff_t uart_rcvrptr;

unsigned char volatile uart_sndbuf[UARTTX_BUFLEN];
utxbufoff_t uart_sndwptr;
utxbufoff_t volatile uart_sndrptr;

#if ((UART_BUFLEN-1) & UART_BUFLEN) == 0
#define RLIM_RL(r) do { r &= (UART_BUFLEN-1); } while(0)
#else
#define RLIM_RL(r) do { if (r>=UART_BUFLEN) r=0; } while(0)
#endif

#if ((UARTTX_BUFLEN-1) & UARTTX_BUFLEN) == 0
#define RLIM_TL(r) do { r &= (UARTTX_BUFLEN-1); } while(0)
#else
#define RLIM_TL(r) do { if (r>=UARTTX_BUFLEN) r=0; } while(0)
#endif

#if (defined __AVR_ATmega328P__)||(defined __AVR_ATmega328__)||(defined __AVR_ATmega168P__)||(defined __AVR_ATmega168__)||(__AVR_ATmega88P__)||(__AVR_ATmega88__)||(defined __AVR_ATmega48P__)||(defined __AVR_ATmega48__)
#define RX_ISR USART_RX_vect
#define TX_ISR USART_UDRE_vect
#else
/* We assume this is a newer avr with fancier USART ISR names, fix if it warns :) */
#define RX_ISR USART0_RX_vect
#define TX_ISR USART0_UDRE_vect
#endif

extern uint8_t timer_waiting;

ISR(RX_ISR) {
	urxbufoff_t reg = uart_rcvwptr;
	uart_rcvbuf[reg++] = UDR0;
	RLIM_RL(reg);
	uart_rcvwptr = reg;
	timer_waiting = 1;
}


ISR(TX_ISR) {
	utxbufoff_t reg = uart_sndrptr;
	if (uart_sndwptr != reg) {
		UDR0 = uart_sndbuf[reg++];
		RLIM_TL(reg);
		uart_sndrptr = reg;
		return;
	} else {
		UCSR0B &= ~_BV(5);
		return;
	}
}


unsigned char uart_isdata(void) {
	if (uart_rcvwptr != uart_rcvrptr) {  return 1; }
	else { return 0; }
}

static inline void uart_sleep_wait(void) __attribute__((always_inline));
static inline void uart_sleep_wait(void) {
	do {
		cli();
		if (uart_rcvwptr == uart_rcvrptr) {
			sleep_enable();
			sei();
			sleep_cpu();
			sleep_disable();
		} else {
			sei();
			return;
		}
	} while (1);
}



unsigned char uart_recv(void) {
	urxbufoff_t reg = uart_rcvrptr;
	volatile uint8_t *dp = uart_rcvbuf + reg;
	reg++;
	RLIM_RL(reg);
	uart_sleep_wait();
	uart_rcvrptr = reg;
	return *dp;
}

unsigned char uart_peek(void) {
	uart_sleep_wait();
	return uart_rcvbuf[uart_rcvrptr];
}

void uart_send(unsigned char val) {
	uint8_t r0b = UCSR0B;
	if (!(r0b & _BV(5))) {
		if (UCSR0A & _BV(5)) { // ISR-less fast path (ISR=off and UDRE => direct send.)
			UDR0 = val;
			return;
		}
	}
	utxbufoff_t reg = uart_sndwptr;
	uart_sndbuf[reg++] = val; // add byte to the transmit queue
	RLIM_TL(reg);
	cli();
	uart_sndwptr = reg;
	utxbufoff_t rptr = uart_sndrptr;
	if (reg==rptr) {
		/* Takes a lot of time (relatively, for us waiting here) to enter & exit an ISR, thus
		 * just turn off ISR and handle it here ASAP. */
		r0b &= ~_BV(5);
		UCSR0B = r0b;
		sei();
		uint8_t d = uart_sndbuf[rptr++];
		RLIM_TL(rptr);
		while (!(UCSR0A & _BV(5)));
		UDR0 = d;
		uart_sndrptr = rptr;
	}
	UCSR0B = r0b | _BV(5); // make sure the transmit int is on
	sei();
}

void uart_init(void) {
	cli();

#include <util/setbaud.h>
// Assuming uart.h defines BAUD
	uart_rcvwptr = 0;
	uart_rcvrptr = 0;
	uart_sndwptr = 0;
	uart_sndrptr = 0;
	UBRR0H = UBRRH_VALUE;
	UBRR0L = UBRRL_VALUE;
	UCSR0C = 0x06; // Async USART,No Parity,1 stop bit, 8 bit chars
	UCSR0A &= 0xFC;
#if USE_2X
	UCSR0A |= (1 << U2X0);
#endif
	UCSR0B = 0x98; // RX complete interrupt enable, Receiver & Transmitter enable
	sei();
}

void uart_wait_txdone(void) {
	while (uart_sndwptr != uart_sndrptr);
}
