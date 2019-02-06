/*
 * Copyright (C) 2018,2019 Urja Rannikko <urjaman@gmail.com>
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

/* This is the interface from a generic LCD API to the SSD1306 driver...
 * so it can know about the SSD1306, but nothing really low level, and
 * users of it need not even know it's really an oled :P */

#include "main.h"
#include "lcd.h"

static uint8_t lcd_char_y, lcd_char_x;
static uint8_t disp_on;

void lcd_idle(uint8_t idle) {
	if (disp_on == !idle) return;
	if (idle) {
		ssd1306_command(SSD1306_DISPLAYOFF);
	} else {
		ssd1306_command(SSD1306_DISPLAYON);
	}
	disp_on = !idle;
}

static uint8_t flip_bits(uint8_t bits) {
	bits = (bits >> 4) | (bits << 4);
	bits = ((bits >> 2) & 0x33) | ((bits << 2) & 0xCC);
	bits = ((bits >> 1) & 0x55) | ((bits << 1) & 0xAA);
	return bits;
}

void lcd_write_block_P(const PGM_P buffer, uint8_t w, uint8_t h)
{
	uint8_t ye = lcd_char_y+h;
	uint8_t we = lcd_char_x+w;
	ssd1306_setbox(lcd_char_x, lcd_char_y, w, h);
	if (ssd1306_start()) return;
	for (uint8_t y=lcd_char_y;y<ye;y++) {
		for (uint8_t x=lcd_char_x;x<we;x++) {
			uint8_t d = pgm_read_byte(buffer);
			buffer++;
			if (ssd1306_data(flip_bits(d))) return;
		}
	}
	ssd1306_end();
	lcd_char_x += w;
	if (lcd_char_x > (LCD_MAXX*LCD_CHARW)) lcd_char_x = (LCD_MAXX*LCD_CHARW); /* saturate */

}

void lcd_write_block(const uint8_t *buffer, uint8_t w, uint8_t h)
{
	uint8_t ye = lcd_char_y+h;
	uint8_t we = lcd_char_x+w;
	ssd1306_setbox(lcd_char_x, lcd_char_y, w, h);
	if (ssd1306_start()) return;
	for (uint8_t y=lcd_char_y;y<ye;y++) {
		for (uint8_t x=lcd_char_x;x<we;x++) {
			if (ssd1306_data(flip_bits(*buffer++))) return;
		}
	}
	ssd1306_end();
	lcd_char_x += w;

}

void lcd_clear_block(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
	ssd1306_setbox(x, y, w, h);
	uint16_t total = (uint16_t)w*(uint16_t)h;
	if (ssd1306_start()) return;
	do {
		if (ssd1306_data(0)) return;
	} while (--total);
	ssd1306_end();
}


// mfont8x8.c is generated with https://github.com/urjaman/st7565-fontgen
#include "mfont8x8.c"

// Some data for dynamic width mode with this font.
#include "font-dyn-meta.c"

static void lcd_putchar_(unsigned char c, uint8_t dw)
{

	PGM_P block;
	if (c < 0x20) c = 0x20;
	block = (const char*)&(mfont[c-0x20][0]);
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
	lcd_clear_block(0,0, LCDWIDTH, LCD_MAXY);
	lcd_char_x = 0;
	lcd_char_y = 0;
}

void lcd_init(void)
{
	ssd1306_init();
	disp_on = 1;
}
