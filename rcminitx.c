/*
 * Copyright (C) 2019 Urja Rannikko <urjaman@gmail.com>
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
#include "timer.h"
#include "rcminitx.h"

#define TX_PORT PORTD
#define TX_DDR DDRD
#define TX_BIT 6

#define PROTO_UNIT 104
#define PROTO_SYNCH 32
#define PROTO_SYNCL 33
#define PROTO_ZEROH 4
#define PROTO_ZEROL 5
#define PROTO_ONEH 4
#define PROTO_ONEL 13

static void tx_sync(void)
{
	TX_PORT |= _BV(TX_BIT);
	timer_delay_us(PROTO_UNIT * PROTO_SYNCH);
	TX_PORT &= ~_BV(TX_BIT);
	timer_delay_us(PROTO_UNIT * PROTO_SYNCL);
}

static void tx_one(void)
{
	TX_PORT |= _BV(TX_BIT);
	timer_delay_us(PROTO_UNIT * PROTO_ONEH);
	TX_PORT &= ~_BV(TX_BIT);
	timer_delay_us(PROTO_UNIT * PROTO_ONEL);
}

static void tx_zero(void)
{
	TX_PORT |= _BV(TX_BIT);
	timer_delay_us(PROTO_UNIT * PROTO_ZEROH);
	TX_PORT &= ~_BV(TX_BIT);
	timer_delay_us(PROTO_UNIT * PROTO_ZEROL);
}

void rcminitx(const uint8_t *code, uint8_t bits, uint8_t rep)
{	
	TX_PORT &= ~_BV(TX_BIT);
	TX_DDR  |= _BV(TX_BIT);

	uint8_t bytes = (bits+7) / 8;
	for (uint8_t n = 0; n < rep; n++) {
		for (uint8_t z = 0; z < bytes; z++) {
			uint8_t bits = (z == bytes-1) ? (bits % 8) : 8;
			if (bits == 0) bits = 8;
			uint8_t db = code[z];
			for (uint8_t i = 0; i < bits; i++) {
				if (db & 0x80) {
					tx_one();
				} else {
					tx_zero();
				}
				db = db << 1;
			}
		}
		tx_sync();
	}
}