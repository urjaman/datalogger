#include "main.h"
#include "lcd.h"
#include "buttons.h"
#include "lib.h"
#include "tui-lib.h"
#include "timer.h"

uint8_t tui_pollkey(void) {
	uint8_t v = buttons_get();
	mini_mainloop();
	return v;
}

uint8_t tui_waitforkey(void) {
	uint8_t v;
	for(;;) {
		v = tui_pollkey();
		if (v) return v;
	}
}

void tui_gen_menuheader(PGM_P header) {
	uint8_t tw = lcd_strwidth_P(header);

	uint8_t banw1 = ((LCDWIDTH - tw) / 2) - 1;
	uint8_t banw2 = LCDWIDTH - tw - banw1 - 2;
	if (tw>=(LCDWIDTH-4)) {
		banw1 = 0;
		banw2 = 0;
		if (tw>=LCDWIDTH) tw = LCDWIDTH;
	}
	uint8_t clrw1 = (LCDWIDTH - tw - banw1 - banw2) / 2;
	uint8_t clrw2 = (LCDWIDTH - tw - banw1 - banw2 - clrw1);
	uint8_t banbwidth = (banw1 > banw2) ? banw1 : banw2;
	uint8_t banner[LCDWIDTH];

	for (uint8_t i=0;i<banbwidth;i++) banner[i] = i&1?0x89:0x91;
	lcd_clear();
	lcd_gotoxy(0,0);
	lcd_write_dwb(banner, banw1);
	lcd_clear_dw(clrw1);
	lcd_puts_dw_P(header);
	lcd_clear_dw(clrw2);
	lcd_write_dwb(banner, banw2);
}

int32_t tui_gen_menupart(printval_func_t *printer, int32_t min, int32_t max, int32_t start, int32_t step, uint8_t delay, uint8_t listmenu) {
	unsigned char buf[LCDWIDTH];
	uint8_t lbm = buttons_hw_count()==1?2:0;
	int32_t idx=start;
	int8_t brackpos = -1;
	PGM_P idstr = PSTR("> ");
	uint8_t idw = lcd_strwidth_P(idstr);
	const uint32_t entries = ((max-min)+1)/step;
	const uint8_t brackl = entries < (LCD_MAXY-1) ? entries : LCD_MAXY - 1;
	uint8_t dl = 125; /* entry delay */
	if (listmenu) {
		/* Enable a list-like menu */
		if (LCD_MAXY>2) {
			if (start==min) brackpos = 0;
			else if (start==max) brackpos = brackl-1;
			else brackpos = 1;
		}
		/* Disable low-buttons mode .. */
		lbm = 0;
	}
	/* No LBM for 16x2s... if i end up using them. */
	if (LCD_MAXY<=2) lbm = 0;
	for (;;) {
		if (brackpos >= 0) {
			int32_t vidx = idx;
			int8_t bp;
			for (bp=brackpos;bp<brackl;bp++) {
				uint8_t sl = printer(buf,vidx);
				lcd_gotoxy_dw(0, bp+1);
				if (bp==brackpos) lcd_puts_dw_P(idstr);
				else lcd_clear_dw(idw);
				buf[sl] = 0;
				lcd_puts_dw(buf);
				lcd_clear_eol();
				vidx += step;
			}
			if (brackpos) { /* Then the entries before our current position */
				vidx = idx - step;
				for (bp=brackpos-1;bp>=0;bp--) {
					uint8_t sl = printer(buf, vidx);
					lcd_gotoxy_dw(0, bp+1);
					lcd_clear_dw(idw);
					buf[sl] = 0;
					lcd_puts_dw(buf);
					lcd_clear_eol();
					if ((vidx-step) < min) vidx = max;
					else vidx -= step;
				}
			}
		} else {
			uint8_t sl = printer(buf, idx);
			buf[sl] = 0;
			sl = lcd_strwidth(buf);
			lcd_gotoxy_dw(0,1);
			lcd_clear_dw((LCDWIDTH - sl)/2);
			lcd_puts_dw(buf);
			lcd_clear_eol();
			if (lbm) {
				lcd_gotoxy_dw(0,2);
				lcd_puts_dw_P(lbm==2?PSTR("DIR: NEXT"):PSTR("DIR: PREV"));
				lcd_clear_eol();
			}
		}
		if (lbm) {
			timer_delay_ms(180);
			timer_delay_ms(100);
		}
		if (dl) timer_delay_ms(dl);
		dl = delay; /* Delay once the pre-set entry delay, and after that the given delay. */
		uint8_t key = tui_waitforkey();
		if ((lbm==1)&&(key&BUTTON_NEXT)) key = BUTTON_PREV;
		switch (key) {
			case BUTTON_NEXT:
				if ((idx+step) > max) {
					idx = min;
					if (brackpos>=0) brackpos = 0;
				} else {
					idx += step;
					if (brackpos>=0) {
						brackpos++;
						if ((idx+step)>max) {
							if (brackpos >= brackl) brackpos = brackl-1;
						} else {
							if (brackpos >= brackl-1) brackpos = brackl-2;
						}
					}
				}
				break;
			case BUTTON_PREV:
				if ((idx-step) < min) {
					idx = max;
					if (brackpos >= 0) brackpos = brackl -1;
				} else {
					idx -= step;
					if (brackpos >= 0) {
						if ((idx-step) < min) {
							if (brackpos) brackpos--;
						} else {
							if (brackpos>1) brackpos--;
						}
					}
				}
				break;
			case BUTTON_OK:
				if (lbm==2) {
					lbm = 1;
					break;
				}
				return idx;
			case BUTTON_CANCEL:
				if (listmenu) return listmenu-1;
				else return start;
				break;
		}
	}
}

static PGM_P const * tui_pgm_menu_table;
static uint8_t tui_pgm_menupart_printer(unsigned char* buf, int32_t val) {
	strcpy_P((char*)buf, (PGM_P)pgm_read_word(&(tui_pgm_menu_table[val])));
	return strlen((char*)buf);
}

//Generic Exit Menu Item
const unsigned char tui_exit_menu[] PROGMEM = "Exit Menu";

uint8_t tui_pgm_menupart(PGM_P const menu_table[], uint8_t itemcnt, uint8_t start) {
	tui_pgm_menu_table = menu_table;
	uint8_t listmenu = start+1;
	PGM_P last_entry = (PGM_P)pgm_read_word(&menu_table[itemcnt-1]);
	if (last_entry == (PGM_P)tui_exit_menu) listmenu = itemcnt;
	return tui_gen_menupart(tui_pgm_menupart_printer,0,itemcnt-1,start,1,50, listmenu);
}

int32_t tui_gen_adjmenu(PGM_P header, printval_func_t *printer,int32_t min, int32_t max, int32_t start, int32_t step) {
	tui_gen_menuheader(header);
	return tui_gen_menupart(printer,min,max,start,step,0,0);
}


uint8_t tui_gen_listmenu(PGM_P header, PGM_P const menu_table[], uint8_t itemcnt, uint8_t start) {
	tui_gen_menuheader(header);
	return tui_pgm_menupart(menu_table,itemcnt,start);
}


static uint8_t tui_nummenu_printer(unsigned char* buf, int32_t val) {
	return uint2str(buf,(uint16_t)val);
}

uint16_t tui_gen_nummenu(PGM_P header, uint16_t min, uint16_t max, uint16_t start) {
	return (uint16_t)tui_gen_adjmenu(header,tui_nummenu_printer,min,max,start,1);
}


static void tui_gen_message_start(PGM_P l1) {
	lcd_clear();
	lcd_gotoxy_dw((LCDWIDTH - lcd_strwidth_P(l1))/2,0);
	lcd_puts_dw_P(l1);
}

static void tui_gen_message_end(void) {
	timer_delay_ms(125);
	tui_waitforkey();
}

void tui_gen_message(PGM_P l1, PGM_P l2) {
	tui_gen_message_start(l1);
	lcd_gotoxy_dw((LCDWIDTH - lcd_strwidth_P(l2))/2,1);
	lcd_puts_dw_P(l2);
	tui_gen_message_end();
}

void tui_gen_message_m(PGM_P l1, const unsigned char* l2m) {
	tui_gen_message_start(l1);
	lcd_gotoxy_dw((LCDWIDTH - lcd_strwidth(l2m))/2,1);
	lcd_puts_dw(l2m);
	tui_gen_message_end();
}

const unsigned char tui_q_s1[] PROGMEM = "NO";
const unsigned char tui_q_s2[] PROGMEM = "YES";

PGM_P const tui_q_table[] PROGMEM = {
    (PGM_P)tui_q_s1,
    (PGM_P)tui_q_s2,
};

uint8_t tui_are_you_sure(void) {
	return tui_gen_listmenu(PSTR("Are you sure?"), tui_q_table, 2, 0);
}


void tui_time_print(uint32_t nt) {
	unsigned char time[16];
	// Time Format: "DDDDDd HH:MM:SS"
	uint16_t days;
	uint8_t hours, mins, secs, x,z;
	days = nt / 86400L; nt = nt % 86400L;
	hours = nt / 3600L; nt = nt % 3600L;
	mins = nt / 60; 
	secs = nt % 60;

	time[0] = (days/10000)|0x30; days = days % 10000;
	time[1] = (days /1000)|0x30; days = days % 1000;
	time[2] = (days / 100)|0x30; days = days % 100;
	time[3] = (days / 10 )|0x30;
	time[4] = (days % 10 )|0x30;
	time[5] = 'd';
	time[6] = ' ';
	time[7] = (hours/10) | 0x30;
	time[8] = (hours%10) | 0x30;
	time[9] = ':';
	time[10]= (mins /10) | 0x30;
	time[11]= (mins %10) | 0x30;
	time[12]= ':';
	time[13]= (secs /10) | 0x30;
	time[14]= (secs %10) | 0x30;
	time[15] = 0;
	z=0;
	if (time[0] == '0') {
		z=1;
		if (time[1] == '0') {
			z=2;
			if (time[2] == '0') {
				z=3;
				if (time[3]  == '0') {
					z=4;
					if (time[4] == '0') {
						z = 7;
						if (hours == 0) {
							z = 10;
						}
					}
				}
			}
		}
	}
	x = (16 - (15-z)) / 2;

	lcd_gotoxy(x,1);
	lcd_puts(&(time[z]));
}
