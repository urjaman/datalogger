/* This is the RCSwitch thing, but heavily modified
 * (from C++ to C, stripped down and removed the arduino API usage)
 * by Urja Rannikko 2018 - my modifications are under GPLv2,
 * as included in the file COPYING in this repository. */

/*
  RCSwitch - Arduino libary for remote control outlet switches
  Copyright (c) 2011 Suat Özgür.  All right reserved.

  Contributors:
  - Andre Koehler / info(at)tomate-online(dot)de
  - Gordeev Andrey Vladimirovich / gordeev(at)openpyro(dot)com
  - Skineffect / http://forum.ardumote.com/viewtopic.php?f=2&t=46
  - Dominik Fischer / dom_fischer(at)web(dot)de
  - Frank Oltmanns / <first name>.<last name>(at)gmail(dot)com
  - Andreas Steinel / A.<lastname>(at)gmail(dot)com
  - Max Horn / max(at)quendi(dot)de
  - Robert ter Vehn / <first name>.<last name>(at)gmail(dot)com
  - Johann Richard / <first name>.<last name>(at)gmail(dot)com
  - Vlad Gheorghe / <first name>.<last name>(at)gmail(dot)com https://github.com/vgheo

  (Original Project home: https://github.com/sui77/rc-switch/ )

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

*/
#include "main.h"
#include "timer.h"
#include "RCSwitch.h"
#include "uart.h"
#include "console.h"

/* Format for protocol definitions:
 * {pulselength, Sync bit, "0" bit, "1" bit}
 *
 * pulselength: pulse length in microseconds, e.g. 350
 * Sync bit: {1, 31} means 1 high pulse and 31 low pulses
 *     (perceived as a 31*pulselength long pulse, total length of sync bit is
 *     32*pulselength microseconds), i.e:
 *      _
 *     | |_______________________________ (don't count the vertical bars)
 * "0" bit: waveform for a data bit of value "0", {1, 3} means 1 high pulse
 *     and 3 low pulses, total length (1+3)*pulselength, i.e:
 *      _
 *     | |___
 * "1" bit: waveform for a data bit of value "1", e.g. {3,1}:
 *      ___
 *     |   |_
 *
 */


/**
 * Description of a single pule, which consists of a high signal
 * whose duration is "high" times the base pulse length, followed
 * by a low signal lasting "low" times the base pulse length.
 * Thus, the pulse overall lasts (high+low)*pulseLength
 */
struct HighLow {
	uint8_t high;
	uint8_t low;
};

/**
 * A "protocol" describes how zero and one bits are encoded into high/low
 * pulses.
 */
struct Protocol {
	/** base pulse length in microseconds */
	uint16_t pulseLength;

	struct HighLow syncFactor;
	struct HighLow zero;
	struct HighLow one;

	/**
	 * If true, interchange high and low logic levels in all transmissions.
	 */
	Bool invertedSignal;
};

static const struct Protocol PROGMEM proto[] = {
	{ 104, {  32, 33 }, {  4,  5 }, {  4,  13 }, false },    // protocol 1
	{ 350, {  1, 31 }, {  1,  3 }, {  3,  1 }, false },    // protocol 1
	{ 650, {  1, 10 }, {  1,  2 }, {  2,  1 }, false },    // protocol 2
	{ 100, { 30, 71 }, {  4, 11 }, {  9,  6 }, false },    // protocol 3
	{ 380, {  1,  6 }, {  1,  3 }, {  3,  1 }, false },    // protocol 4
	{ 500, {  6, 14 }, {  1,  2 }, {  2,  1 }, false },    // protocol 5
	{ 450, { 23,  1 }, {  1,  2 }, {  2,  1 }, true },      // protocol 6 (HT6P20B)
	{ 150, {  2, 62 }, {  1,  6 }, {  6,  1 }, false }     // protocol 7 (HS2303-PT, i. e. used in AUKEY Remote)
};

enum {
	numProto = sizeof(proto) / sizeof(proto[0])
};

// Number of maximum high/Low changes per packet.
// We can handle up to (unsigned long) => 32 bit * 2 H/L changes per bit + 2 for sync
#define RCSWITCH_MAX_CHANGES 90

/* TX statics */
static int nRepeatTransmit;
static struct Protocol protocol;

/* RX is hooked to PORTD a bit hard, but atleast the bit is quick to change */

#define RX_BIT 4

/* Transmitter */

#define TX_PORT PORTD
#define TX_DDR DDRD
#define TX_BIT 6

void rcsw_set_tx_proto(int nProtocol)
{
	memcpy_P(&protocol, &proto[nProtocol-1], sizeof(struct Protocol));
}

void rcsw_set_tx_pulselen(int nPulseLength)
{
	protocol.pulseLength = nPulseLength;
}

void rcsw_set_tx_repeats(int repeat)
{
	nRepeatTransmit = repeat;
}

void rcsw_enable_tx(void)
{
	TX_PORT &= ~_BV(TX_BIT);
	TX_DDR  |= _BV(TX_BIT);
}

void rcsw_disable_tx(void)
{
	/// ehhhhh, just dont do it, dude.
}

static void transmit(struct HighLow pulses)
{
	if (protocol.invertedSignal) {
		TX_PORT &= ~_BV(TX_BIT);
		timer_delay_us( protocol.pulseLength * pulses.high);
		TX_PORT |= _BV(TX_BIT);
		timer_delay_us( protocol.pulseLength * pulses.low);
	} else {
		TX_PORT |= _BV(TX_BIT);
		timer_delay_us( protocol.pulseLength * pulses.high);
		TX_PORT &= ~_BV(TX_BIT);
		timer_delay_us( protocol.pulseLength * pulses.low);
	}
}

/**
 * Transmit the first 'length' bits of the integer 'code'. The
 * bits are sent from MSB to LSB, i.e., first the bit at position length-1,
 * then the bit at position length-2, and so on, till finally the bit at position 0.
 */
void rcsw_tx(uint64_t code, unsigned int length)
{
#ifdef RCSW_RX
	// make sure the receiver is disabled while we transmit
	int receiver_enabled = PCMSK2 & _BV(RX_BIT);
	if (receiver_enabled) rcsw_disable_rx();
#endif
	
	uint64_t mask = 1ULL << (length-1);
	uint64_t xmit;
	for (int nRepeat = 0; nRepeat < nRepeatTransmit; nRepeat++) {
		xmit = code;
		for (int i = length-1; i >= 0; i--) {
			if (xmit & mask)
				transmit(protocol.one);
			else
				transmit(protocol.zero);
			xmit = xmit << 1;
		}
		transmit(protocol.syncFactor);
	}

	// Disable transmit after sending (i.e., for inverted protocols)
	TX_PORT &= ~_BV(TX_BIT);
#ifdef RCSW_RX
	// enable receiver again if we just disabled it
	if (receiver_enabled) rcsw_enable_rx();
#endif
}

/* Receiver */
#ifdef RCSW_RX

/* RX statics */
static volatile uint64_t nReceivedValue = 0;
static volatile unsigned int nReceivedBitlength = 0;
static volatile unsigned int nReceivedDelay = 0;
static volatile unsigned int nReceivedProtocol = 0;
static int nReceiveTolerance = 60;
static const unsigned int nSeparationLimit = 3000;
// separationLimit: minimum microseconds between received codes, closer codes are ignored.
// according to discussion on issue #14 it might be more suitable to set the separation
// limit to the same time as the 'low' part of the sync signal for the current protocol.
static unsigned int timings[RCSWITCH_MAX_CHANGES];
static uint8_t values[RCSWITCH_MAX_CHANGES];

void rcsw_enable_rx(void)
{
	nReceivedValue = 0;
	nReceivedBitlength = 0;
	PCIFR |= _BV(PCIF2);
	PCICR |= _BV(PCIE2);
	PCMSK2 |= _BV(RX_BIT);
}

void rcsw_disable_rx(void)
{
	PCMSK2 &= ~_BV(RX_BIT);
}

void rcsw_set_rx_tol(int nPercent)
{
	nReceiveTolerance = nPercent;
}

Bool rcsw_has_rx_data(void)
{
	return nReceivedValue != 0;
}

void rcsw_clear_rx_data(void)
{
	nReceivedValue = 0;
}

unsigned long rcsw_get_rx_data(void)
{
	return nReceivedValue;
}

unsigned int rcsw_get_rx_bitlen(void)
{
	return nReceivedBitlength;
}

unsigned int rcsw_get_rx_delay(void)
{
	return nReceivedDelay;
}

unsigned int rcsw_get_rx_proto(void)
{
	return nReceivedProtocol;
}

unsigned int* rcsw_get_rx_times(void)
{
	return timings;
}

/* helper function for the receiveProtocol method */
static inline unsigned int diff(int A, int B)
{
	return abs(A - B);
}

static Bool receiveProtocol(const int p, unsigned int changeCount)
{
	struct Protocol pro;
	memcpy_P(&pro, &proto[p-1], sizeof(struct Protocol));
	
	SEND('a' + p);

	uint64_t code = 0;
	//Assuming the longer pulse length is the pulse captured in timings[0]
	const unsigned int syncLengthInPulses =  ((pro.syncFactor.low) > (pro.syncFactor.high)) ?
	                (pro.syncFactor.low) : (pro.syncFactor.high);
	const unsigned int delay = timings[0] / syncLengthInPulses;
	const unsigned int delayTolerance = delay * nReceiveTolerance / 100;

	luint2outdual(delay);
	
	/* For protocols that start low, the sync period looks like
	 *               _________
	 * _____________|         |XXXXXXXXXXXX|
	 *
	 * |--1st dur--|-2nd dur-|-Start data-|
	 *
	 * The 3rd saved duration starts the data.
	 *
	 * For protocols that start high, the sync period looks like
	 *
	 *  ______________
	 * |              |____________|XXXXXXXXXXXXX|
	 *
	 * |-filtered out-|--1st dur--|--Start data--|
	 *
	 * The 2nd saved duration starts the data
	 */
	const unsigned int firstDataTiming = (pro.invertedSignal) ? (2) : (1);

	for (unsigned int i = firstDataTiming; i < changeCount - 1; i += 2) {
		code <<= 1;
		if (diff(timings[i], delay * pro.zero.high) < delayTolerance &&
		    diff(timings[i + 1], delay * pro.zero.low) < delayTolerance) {
			SEND('0');
			// zero
		} else if (diff(timings[i], delay * pro.one.high) < delayTolerance &&
		           diff(timings[i + 1], delay * pro.one.low) < delayTolerance) {
			SEND('1');
			// one
			code |= 1;
		} else {
			SEND('F');
			rcsw_rx_debug_report();
			// Failed
			return false;
		}
	}

	if (changeCount > 7) {    // ignore very short transmissions: no device sends them, so this must be noise
		nReceivedValue = code;
		nReceivedBitlength = (changeCount - 1) / 2;
		nReceivedDelay = delay;
		nReceivedProtocol = p;
		SEND('Z');
		return true;
	}
	SEND('Y');
	rcsw_rx_debug_report();
	return false;
}

static volatile uint8_t changeCount = 0;
static uint8_t repeatCount = 0;


void rcsw_rx_debug_report(void) {
	uint8_t cc = changeCount;
	sendstr_P(PSTR("CC: "));
	luint2outdual(cc);
	if (cc) {
		sendstr_P(PSTR(" TIMES"));
		for (uint8_t i = 0; i < cc; i ++) {
			luint2outdual(timings[i]);
			if (values[i]) SEND('H');
			else SEND('L');
		}
	}
	sendstr_P(PSTR("\r\n"));
	changeCount = 0;
}

ISR(PCINT2_vect)
{
	static uint16_t lastTime = 0;
	uint8_t v = PIND & _BV(RX_BIT);
	// Disable PCINT (because subsectimer fetch enables interrupts)
	uint8_t pcmsk = PCMSK2;
	PCMSK2 = 0;

	uint16_t ss  = timer_get_subsectimer();
	uint16_t duration = ss - lastTime;
	if (lastTime > ss) duration += SSTC;
	lastTime = ss;
	duration *= US_PER_SSUNIT;

	if ((duration > nSeparationLimit) && (duration < 40000)) {
		// A long stretch without signal level change occurred. This could
		// be the gap between two transmission.
		if (changeCount > 80) {
		//if (diff(duration, timings[0]) < 200) {
			// This long signal is close in length to the long signal which
			// started the previously recorded timings; this suggests that
			// it may indeed by a a gap between two transmissions (we assume
			// here that a sender will send the signal multiple times,
			// with roughly the same gap between them).
			//repeatCount++;
			SEND('R');
			//if (repeatCount == 2) {
				for(unsigned int i = 1; i <= 1; i++) {
					if (receiveProtocol(i, changeCount)) {
						// receive succeeded for protocol i
						break;
					}
				}
				//repeatCount = 0;
			//}
		//}
		}
		changeCount = 0;
	}

	// detect overflow
	if (changeCount >= RCSWITCH_MAX_CHANGES) {
		SEND('O');
		changeCount = 0;
		repeatCount = 0;
	}
	values[changeCount] = v;
	timings[changeCount++] = duration;

	cli();
	PCMSK2 = pcmsk;
}
#endif
