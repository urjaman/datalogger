/*
$Id:$

ST7565 LCD library! - No RAM buffer edition (NR)

Copyright (C) 2010 Limor Fried, Adafruit Industries
Copyright (C) 2014,2016 Urja Rannikko <urjaman@gmail.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

 // some of this code was written by <cstone@pobox.com> originally; it is in the public domain.
*/
#include "main.h" // Most important: define F_CPU
#include <avr/pgmspace.h>
#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>
// These are ST library stuff (keep these)
#include "stlcdhw.h"
#include "stlcdnr.h"
// These only for debug
#include "console.h"
#include "uart.h"


/* HW config. */
/* Note: The code is currently setup for SCLK_PORT == SID_PORT. */
/* See the #if 0 below for code if you need SCLK_PORT != SID_PORT. */

#define SID_DDR DDRB
#define SID_PIN PINB
#define SID_PORT PORTB
#define SID 0

#define SCLK_DDR DDRB
#define SCLK_PIN PINB
#define SCLK_PORT PORTB
#define SCLK 1

#define A0_DDR DDRA
#define A0_PIN PINA
#define A0_PORT PORTA
#define A0 4

#define RST_DDR DDRA
#define RST_PIN PINA
#define RST_PORT PORTA
#define RST 5

#define CS_DDR DDRA
#define CS_PIN PINA
#define CS_PORT PORTA
#define CS 6

/* Internal state */
static uint8_t lcd_char_x = 0;
static uint8_t lcd_char_y = 0;

/* Internal functions */
static void st7565_clear(void);
static void st7565_init(void);
static void spiwrite(uint8_t c);
static void st7565_command(uint8_t c);
static void st7565_data(uint8_t c);
static void st7565_gotoxy(uint8_t x, uint8_t y);

/* Init Debug is optional. */
#ifndef SENDSTR
#define SENDSTR(x)
#endif

#define nop asm volatile ("nop\n\t")

void lcd_init(void)
{
    SENDSTR("SETUP\r\n");
    st7565_init();
    SENDSTR("INIT\r\n");
    st7565_command(CMD_DISPLAY_ON);
    SENDSTR("DISPLAY_ON\r\n");
    st7565_command(CMD_SET_ALLPTS_NORMAL);
    SENDSTR("ALLPTS_NORMAL\r\n");
    st7565_set_contrast(32);
    SENDSTR("CONTRAST\r\n");
    lcd_clear();
    SENDSTR("LCD_INIT DONE\r\n");
}

void lcd_gotoxy_dw(uint8_t x, uint8_t y)
{
    if (x >= LCDWIDTH) x=LCDWIDTH-1;
    if (y >= LCD_MAXY) y=LCD_MAXY-1;
    lcd_char_x = x;
    lcd_char_y = y;
}

void lcd_gotoxy(uint8_t x, uint8_t y)
{
    lcd_gotoxy_dw(LCD_CHARW*x,y);
}


void lcd_clear(void)
{
    st7565_clear();
    lcd_char_x = 0;
    lcd_char_y = 0;
}

static void st7565_init(void) {
  // set pin directions
  SID_DDR |= _BV(SID);
  SCLK_DDR |= _BV(SCLK);
  A0_DDR |= _BV(A0);
  RST_DDR |= _BV(RST);
  CS_DDR |= _BV(CS);

  // toggle RST low to reset; CS low so it'll listen to us
  CS_PORT &= ~_BV(CS);
  RST_PORT &= ~_BV(RST);
  _delay_ms(50);
  RST_PORT |= _BV(RST);

  // LCD bias select
  st7565_command(CMD_SET_BIAS_7);
  // ADC select
  st7565_command(CMD_SET_ADC_REVERSE);
  // SHL select
  st7565_command(CMD_SET_COM_NORMAL);
  // Initial display line
  st7565_command(CMD_SET_DISP_START_LINE);

  // turn on voltage converter (VC=1, VR=0, VF=0)
  st7565_command(CMD_SET_POWER_CONTROL | 0x4);
  // wait for 50% rising
  _delay_ms(50);

  // turn on voltage regulator (VC=1, VR=1, VF=0)
  st7565_command(CMD_SET_POWER_CONTROL | 0x6);
  // wait >=50ms
  _delay_ms(50);

  // turn on voltage follower (VC=1, VR=1, VF=1)
  st7565_command(CMD_SET_POWER_CONTROL | 0x7);
  // wait
  _delay_ms(10);

  // set lcd operating voltage (regulator resistor, ref voltage resistor)
  st7565_command(CMD_SET_RESISTOR_RATIO | 0x6);

}

#if 0
// Compatible version
static void spiwrite(uint8_t c) {
  int8_t i;
  for (i=7; i>=0; i--) {
    SCLK_PORT &= ~_BV(SCLK);
    if (c & 0x80)
      SID_PORT |= _BV(SID);
    else
      SID_PORT &= ~_BV(SID);
    c = c<<1;
    nop;
    SCLK_PORT |= _BV(SCLK);
    nop;
  }
}
#else
#define bld(to,bit_to) bld2(to,bit_to)
#define bst(from,bit_from) bst2(from,bit_from)
#define bld2(to,bit_to) asm("bld %0, " #bit_to "\n\t" : "+r" (to) :)
#define bst2(from,bit_from) asm("bst %0, " #bit_from "\n\t" :: "r" (from))

// This code should be ok for F_CPU <= 16Mhz (because LCD max 4Mhz)
// Provides F = F_CPU/4, 50/50 duty cycle, tested with F_CPU = 12Mhz
// Assumes SCLK_PORT == SID_PORT.
// Note: if you have an ISR that touches this port, cli() this sequence.
static void spiwrite(uint8_t c) {
  uint8_t p = SID_PORT & ~_BV(SCLK);
  bst(c,7);
  bld(p,SID);
  SID_PORT = p;
  bst(c,6);
  SCLK_PORT |= _BV(SCLK);
  bld(p,SID);
  SID_PORT = p;
  bst(c,5);
  SCLK_PORT |= _BV(SCLK);
  bld(p,SID);
  SID_PORT = p;
  bst(c,4);
  SCLK_PORT |= _BV(SCLK);
  bld(p,SID);
  SID_PORT = p;
  bst(c,3);
  SCLK_PORT |= _BV(SCLK);
  bld(p,SID);
  SID_PORT = p;
  bst(c,2);
  SCLK_PORT |= _BV(SCLK);
  bld(p,SID);
  SID_PORT = p;
  bst(c,1);
  SCLK_PORT |= _BV(SCLK);
  bld(p,SID);
  SID_PORT = p;
  bst(c,0);
  SCLK_PORT |= _BV(SCLK);
  bld(p,SID);
  SID_PORT = p;
  nop;
  SCLK_PORT |= _BV(SCLK);
}

static void spiwrite_data(uint8_t c) {
  uint8_t p = SID_PORT & ~_BV(SCLK);
  bst(c,0);
  bld(p,SID);
  SID_PORT = p;
  bst(c,1);
  SCLK_PORT |= _BV(SCLK);
  bld(p,SID);
  SID_PORT = p;
  bst(c,2);
  SCLK_PORT |= _BV(SCLK);
  bld(p,SID);
  SID_PORT = p;
  bst(c,3);
  SCLK_PORT |= _BV(SCLK);
  bld(p,SID);
  SID_PORT = p;
  bst(c,4);
  SCLK_PORT |= _BV(SCLK);
  bld(p,SID);
  SID_PORT = p;
  bst(c,5);
  SCLK_PORT |= _BV(SCLK);
  bld(p,SID);
  SID_PORT = p;
  bst(c,6);
  SCLK_PORT |= _BV(SCLK);
  bld(p,SID);
  SID_PORT = p;
  bst(c,7);
  SCLK_PORT |= _BV(SCLK);
  bld(p,SID);
  SID_PORT = p;
  nop;
  SCLK_PORT |= _BV(SCLK);
}


#endif


static void st7565_command(uint8_t c) {
  A0_PORT &= ~_BV(A0);
  spiwrite(c);
}

static void st7565_data(uint8_t c) {
  A0_PORT |= _BV(A0);
  spiwrite_data(c);
}

static void st7565_clear(void) {
  uint8_t p, c;
  for(p = 0; p < 8; p++) {
    st7565_command(CMD_SET_PAGE | p);
    spiwrite(CMD_SET_COLUMN_LOWER | (0x2 & 0xf)); /* Already in command mode. */
    spiwrite(CMD_SET_COLUMN_UPPER | ((0x0 >> 4) & 0xf));
    spiwrite(CMD_RMW);
    st7565_data(0xff);
    for(c = 0; c < 128; c++) {
	spiwrite_data(0); // Already in data mode as above
    }
  }
}

void st7565_set_contrast(uint8_t val)
{
    st7565_command(CMD_SET_VOLUME_FIRST);
    st7565_command(CMD_SET_VOLUME_SECOND | (val & 0x3f));
}

//static const uint8_t PROGMEM pagemap[] = { 3, 2, 1, 0, 7, 6, 5, 4 };
static const uint8_t PROGMEM pagemap[] = { 4, 5, 6, 7, 0, 1, 2, 3 };

static void st7565_gotoxy(uint8_t x, uint8_t y) /* This is the hardware gotoxy */
{
	uint8_t cs = 3+x;
	st7565_command(CMD_SET_PAGE | pgm_read_byte(&(pagemap[y])) );
	spiwrite(CMD_SET_COLUMN_LOWER | (cs & 0xf));
	spiwrite(CMD_SET_COLUMN_UPPER | ((cs >> 4) & 0xf));
	spiwrite(CMD_RMW);
}

void lcd_write_block_P(const PGM_P buffer, uint8_t w, uint8_t h)
{
	uint8_t ye = lcd_char_y+h;
	uint8_t we = lcd_char_x+w;
	if (we > (LCD_CHARW*LCD_MAXX)) return; /* Dont waste time writing clipped stuff (this would break lines if we clipped). */
	if (ye > LCD_MAXY) ye = LCD_MAXY; /* This can be safely clipped... */
	for (uint8_t y=lcd_char_y;y<ye;y++) {
		st7565_gotoxy(lcd_char_x,y);
		for (uint8_t x=lcd_char_x;x<we;x++) {
			uint8_t d = pgm_read_byte(buffer);
			buffer++;
			st7565_data(d);
		}
	}
	lcd_char_x += w;
	if (lcd_char_x > (LCD_MAXX*LCD_CHARW)) lcd_char_x = (LCD_MAXX*LCD_CHARW); /* saturate */

}

void lcd_write_block(const uint8_t *buffer, uint8_t w, uint8_t h)
{
	uint8_t ye = lcd_char_y+h;
	uint8_t we = lcd_char_x+w;
	if (we > (LCD_CHARW*LCD_MAXX)) return;
	if (ye > LCD_MAXY) ye = LCD_MAXY; /* This can be safely clipped... */
	for (uint8_t y=lcd_char_y;y<ye;y++) {
		st7565_gotoxy(lcd_char_x,y);
		for (uint8_t x=lcd_char_x;x<we;x++) {
			uint8_t d = *buffer;
			buffer++;
			st7565_data(d);
		}
	}
	lcd_char_x += w;
	if (lcd_char_x > (LCD_MAXX*LCD_CHARW)) lcd_char_x = (LCD_MAXX*LCD_CHARW); /* saturate */
}

void lcd_clear_block(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
	for (uint8_t yi=y;yi<(y+h);yi++) {
		st7565_gotoxy(x, yi);
		for (uint8_t n=0;n<w;n++) st7565_data(0);
	}
}


// mfont8x8.c is generated with https://github.com/urjaman/st7565-fontgen
#include "mfont8x8.c"

// Some data for dynamic width mode with this font.
#include "font-dyn-meta.c"

static void lcd_putchar_(unsigned char c, uint8_t dw)
{

	PGM_P block;
	if (c < 0x20) c = 0x20;
	block = (const char*)&(st7565_font[c-0x20][0]);
	uint8_t w = LCD_CHARW;
	if (dw) {
	    uint8_t font_meta_b = pgm_read_byte(&(font_metadata[c-0x20]));
	    block += XOFF(font_meta_b);
	    w = DW(font_meta_b);
	}
    lcd_write_block_P(block,w,1);
}

void lcd_putchar(unsigned char c)
{
    lcd_putchar_(c,0);
}

void lcd_putchar_dw(unsigned char c)
{
    lcd_putchar_(c,1);
}

static void lcd_puts_(const unsigned char * str, uint8_t dw)
{
start:
        if (*str) lcd_putchar_(*str,dw);
        else return;
        str++;
        goto start;
}

static void lcd_puts_P_(PGM_P str,uint8_t dw)
{
        unsigned char c;
start:
        c = pgm_read_byte(str);
        if (c) lcd_putchar_(c,dw);
        else return;
        str++;
        goto start;
}

void lcd_puts(const unsigned char * str)
{
    lcd_puts_(str,0);
}

uint8_t lcd_puts_dw(const unsigned char *str)
{
    uint8_t xb = lcd_char_x;
    lcd_puts_(str,1);
    return lcd_char_x - xb;
}

void lcd_puts_P(PGM_P str)
{
    lcd_puts_P_(str,0);
}

uint8_t lcd_puts_dw_P(PGM_P str)
{
    uint8_t xb = lcd_char_x;
    lcd_puts_P_(str,1);
    return lcd_char_x - xb;
}

void lcd_clear_dw(uint8_t w) {
	if ((lcd_char_x+w)>LCDWIDTH) {
		w = LCDWIDTH - lcd_char_x;
	}
	lcd_clear_block(lcd_char_x, lcd_char_y, w, 1);
	lcd_char_x += w;
}

void lcd_clear_eol(void) {
	lcd_clear_dw(LCDWIDTH - lcd_char_x);
	lcd_char_x = 0;
	lcd_char_y++;
}

void lcd_write_dwb(uint8_t *buf, uint8_t w) {
        lcd_write_block(buf, w, 1);
}

// byte 0 bit 7 is Y=0 X=0
// byte 1 bit 7 is Y=0 X=1
// byte W bit 7 is Y=8 X=0
// ..
#if 0
// These doublers are hardcoded to do h 1=>2 (aka 8 to 16)
static void doubler(uint8_t* out, uint8_t* in, uint8_t w)
{
	memset(out,0,w*4); // so we can use |=  to generate output
	for (uint8_t x=0;x<w;x++) {
		uint8_t d = in[x];
		for (uint8_t y=0;y<8;y++) {
			uint8_t od = (d & _BV(7-y)) ? 0xF : 0x0;
			uint8_t hn = 0; // have-neighbors
			uint8_t nd = 0; // neigh-data;
			if (y>0) {
				hn |= _BV(0);
				if (d & _BV(7-(y-1))) nd |= _BV(0);
			}
			if (x>0) {
				hn |= _BV(1);
				if (in[x-1] & _BV(7-y)) nd |= _BV(1);
			}
			if (x<(w-1)) {
				hn |= _BV(2);
				if (in[x+1] & _BV(7-y)) nd |= _BV(2);
			}
			if (y<7) {
				hn |= _BV(3);
				if (d & _BV(7-(y+1))) nd |= _BV(3);
			}
			if (!od)  nd ^= 0xF;
			if (((hn&0x3) == 0x3)&&(nd == 0xC)) od ^= 0x8;
			if (((hn&0x5) == 0x5)&&(nd == 0xA)) od ^= 0x2;
			if (((hn&0xC) == 0xC)&&(nd == 0x3)) od ^= 0x1;
			if (((hn&0xA) == 0xA)&&(nd == 0x5)) od ^= 0x4;

			if (y>=4) {
				out[(w+x)*2]   |= (od>>2) << ((7-y)*2);
				out[(w+x)*2+1] |= (od&0x3) << ((7-y)*2);
			} else {
				out[x*2]   |= (od>>2) << ((3-y)*2);
				out[x*2+1] |= (od&0x3) << ((3-y)*2);
			}
		}
	}
}

#else

// This is the old stuff...
static void doubler(uint8_t* out, uint8_t* in, uint8_t w)
{
        for (uint8_t i=0;i<w;i++) {
        	uint8_t d = in[i];
        	uint8_t hi = 0;
        	uint8_t lo = 0;
        	for (uint8_t n=0;n<4;n++) {
        		hi = hi >> 2;
        		lo = lo >> 2;
        		if (d & _BV(4+n)) hi |= 0xC0;
        		if (d & _BV(n)) lo |= 0xC0;
        	}
        	out[i*2] = hi;
        	out[i*2+1] = hi;
        	out[(w+i)*2] = lo;
        	out[(w+i)*2+1] = lo;
        }
}
#endif

static void lcd_putchar_big(unsigned char c)
{
	uint8_t in[8];
	uint8_t buf[32];

	PGM_P block;
	if (c < 0x20) c = 0x20;
	block = (const char*)&(st7565_font[c-0x20][0]);
        uint8_t font_meta_b = pgm_read_byte(&(font_metadata[c-0x20]));
	uint8_t w = DW(font_meta_b);
        block += XOFF(font_meta_b);
	if ((lcd_char_x+(w*2))>LCDWIDTH) {
		w = (LCDWIDTH - lcd_char_x)/2;
	}
	memcpy_P(in, block, w);
	doubler(buf, in, w);
        lcd_write_block(buf,w*2,2);
}

static uint8_t lcd_dw_charw(uint8_t c)
{
	if (c < 0x20) c = 0x20;
        uint8_t font_meta_b = pgm_read_byte(&(font_metadata[c-0x20]));
	return DW(font_meta_b);
}


uint8_t lcd_strwidth(const unsigned char * str) {
	uint8_t w = 0;
	do {
		if (*str) w += lcd_dw_charw(*str);
		else return w;
		str++;
	} while(1);
}

uint8_t lcd_strwidth_P(PGM_P str) {
	uint8_t w = 0;
	do {
		uint8_t c = pgm_read_byte(str);
		if (c) w += lcd_dw_charw(c);
		else return w;
		str++;
	} while(1);
}

/* big = 2x small, but dont tell the user ;) */
uint8_t lcd_strwidth_big(const unsigned char * str) {
	return lcd_strwidth(str)*2;
}

uint8_t lcd_strwidth_big_P(PGM_P str) {
	return lcd_strwidth_P(str)*2;
}



void lcd_puts_big_P(PGM_P str) {
	unsigned char c;
start:
	c = pgm_read_byte(str);
	if (c) lcd_putchar_big(c);
	else return;
	str++;
	goto start;
}

void lcd_puts_big(const unsigned char * str) {
start:
	if (*str) lcd_putchar_big(*str);
	else return;
	str++;
	goto start;
}

