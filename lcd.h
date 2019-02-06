#pragma once

#include "SSD1306.h"

#define LCD_CHARW 8
#define LCDWIDTH SSD1306_LCDWIDTH
#define LCD_MAXX (SSD1306_LCDWIDTH/8)
#define LCD_MAXY (SSD1306_LCDHEIGHT/8)

void lcd_init(void);
void lcd_putchar(unsigned char c);
void lcd_puts(const unsigned char* str);
void lcd_puts_P(PGM_P str);
void lcd_clear(void);
// This is compatibility, in char blocks
void lcd_gotoxy(uint8_t x, uint8_t y);

// This is in the native format, x= pixels, y=blocks
void lcd_gotoxy_dw(uint8_t x, uint8_t y);

uint8_t lcd_puts_dw(const unsigned char *str);
uint8_t lcd_puts_dw_P(PGM_P str);

// These are for the dynamic extension of the font, obviously
uint8_t lcd_strwidth(const unsigned char *str);
uint8_t lcd_strwidth_P(PGM_P str);

/* Buffer should be w*h bytes big. */
void lcd_write_block_P(const PGM_P buffer, uint8_t w, uint8_t h);
void lcd_write_block(const uint8_t* buffer, uint8_t w, uint8_t h);

/* Dynwidth "gfx" functions. */
void lcd_clear_dw(uint8_t w);
void lcd_write_dwb(uint8_t *buf, uint8_t w);
void lcd_clear_eol(void);

/* Crude on/off control to preserve OLED life ... */
void lcd_idle(uint8_t idle);
