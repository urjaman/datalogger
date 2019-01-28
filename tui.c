#include "main.h"
#include "buttons.h"
#include "timer.h"
#include "lcd.h"
#include "lib.h"
#include "tui.h"
#include "tui-lib.h"
#include "rtc.h"
#include "ams2302.h"
#include "logger.h"
#include "fat.h"

#define TUI_DEFAULT_REFRESH_INTERVAL 5

static uint8_t tui_force_draw;
static uint8_t tui_next_refresh;
static uint8_t tui_refresh_interval=TUI_DEFAULT_REFRESH_INTERVAL; // by default 1s


void tui_num_helper(unsigned char* buf, uint8_t n) {
	buf[0] = n/10|0x30;
	buf[1] = n%10|0x30;
}

static void tui_datetime(struct mtm* tm, uint8_t* lb) {
	uint16_t fyear = tm->year + TIME_EPOCH_YEAR;
	tui_num_helper(lb, fyear / 100);
	tui_num_helper(lb+2, fyear % 100);
	lb[4] = '-';
	tui_num_helper(lb+5,tm->month);
	lb[7] = '-';
	tui_num_helper(lb+8,tm->day);
	lb[10] = ' ';
	tui_num_helper(lb+11,tm->hour);
	lb[13] = ':';
	tui_num_helper(lb+14,tm->min);
	lb[16] = 0;
}


static void tui_draw_mainpage(uint8_t forced) {
	int16_t temp10;
	uint16_t rh10;

	uint8_t timetxt[18];
	tui_force_draw = 0;
	if (!forced) {
		tui_next_refresh = timer_get_5hz_cnt()+tui_refresh_interval;
	}
	lcd_gotoxy(0,0);
	struct mtm tm;
	timer_get_time(&tm);
	tui_datetime(&tm, timetxt);
	lcd_puts(timetxt);

	lcd_gotoxy(0, 1);
	PGM_P r = ams_get(&temp10, &rh10, 5);
	if (r) {
		lcd_puts_dw_P(r);
	} else {
		unsigned char v10s[8];
		lcd_puts_dw_P(PSTR("T: "));
		make_v10_str(v10s, temp10);
		lcd_puts_dw(v10s);
		lcd_puts_dw_P(PSTR("\xB0" "C RH:"));
		make_v10_str(v10s, rh10);
		lcd_puts_dw(v10s);
		lcd_puts_dw_P(PSTR("%"));
	}
	lcd_clear_eol();

	lcd_gotoxy(0, 2);
	lcd_puts_dw_P(PSTR("SD: "));
	timetxt[0] = 0x30 | logger_sd_status();
	timetxt[1] = ' ';
	timetxt[2] = 0;
	lcd_puts_dw(timetxt);
	PGM_P e = logger_sd_err();
	if (e) {
		lcd_puts_dw_P(PSTR("E:"));
		lcd_puts_dw_P(e);
		lcd_clear_eol();

		e = fat_get_last_error();
		lcd_gotoxy(0, 3);
		if (e) {
			lcd_puts_dw_P(PSTR("F:"));
			lcd_puts_dw_P(e);
		}
	} else {
		uint8_t *wp = timetxt;
		lcd_puts_dw_P(PSTR("LB:"));
		wp += uint2str(wp, logger_buf_stat());
		*wp++ = '/';
		uint2str(wp, LOGBUF_SZ);
		lcd_puts_dw(timetxt);
		lcd_clear_eol();

		lcd_gotoxy(0, 3);
		luint2str(timetxt, logger_log_size());
		lcd_puts_dw_P(PSTR("LS:"));
		lcd_puts_dw(timetxt);
	}
	lcd_clear_eol();
}

void tui_init(void) {
	lcd_clear();
	tui_draw_mainpage(0);
}


void tui_run(void) {
	uint8_t k = buttons_get();
#if 0
	if ((prev_k != k)&&(k)) {
		prev_k = k;
		tui_force_draw = 1;
	}
#endif
	if (k==BUTTON_OK) {
		tui_mainmenu();
		lcd_clear();
		tui_force_draw = 1;
	}
	if (tui_force_draw) {
		tui_draw_mainpage(tui_force_draw);
		return;
	} else {
		if (timer_get_5hzp()) {
			signed char update = (signed char)(timer_get_5hz_cnt() - tui_next_refresh);
			if (update>=0) {
				tui_draw_mainpage(0);
				return;
			}
		}
	}
}


void tui_activate(void) {
	tui_force_draw=1;
}




const unsigned char tui_setclock_name[] PROGMEM = "Set Clock";
void tui_set_clock(void) {
	struct mtm tm;
	timer_get_time(&tm);
	tm.sec = 0;
	uint16_t year = tui_gen_nummenu(PSTR("Year"),TIME_EPOCH_YEAR,TIME_EPOCH_YEAR+130,tm.year+TIME_EPOCH_YEAR);
	uint8_t month = tui_gen_nummenu(PSTR("Month"),1,12,tm.month);
	year = year - TIME_EPOCH_YEAR;
	if ((tm.month != month)||(tm.year != year)) { // Day count in the month possibly changed, cannot start from old day.
		tm.day = 1;
	}
	tm.day = tui_gen_nummenu(PSTR("Day"),1,month_days(year,month-1),tm.day);
	tm.year = year;
	tm.month = month;
	tm.hour = tui_gen_nummenu(PSTR("Hours"), 0, 23, tm.hour);
	tm.min = tui_gen_nummenu(PSTR("Minutes"),0, 59, tm.min);
	timer_set_time(&tm);
}

const unsigned char tui_sdeject_name[] PROGMEM = "Eject SD";
void tui_eject_sd(void) {
	if (!tui_are_you_sure()) return;
	logger_sd_eject(1);
	tui_gen_message(PSTR("Swap SD Card"), PSTR("Now"));
	logger_sd_eject(0);
}


const unsigned char tui_mm_s2[] PROGMEM = "RTC Status";

PGM_P const tui_mm_table[] PROGMEM = {
    (PGM_P)tui_sdeject_name,
    (PGM_P)tui_setclock_name,
    (PGM_P)tui_mm_s2,
    (PGM_P)tui_exit_menu
};

void tui_mainmenu(void) {
	uint8_t sel=0;
	for (;;) {
		sel = tui_gen_listmenu(PSTR("MAIN MENU"), tui_mm_table, 4, sel);
		switch (sel) {
			case 0:
				tui_eject_sd();
				return;
			case 1:
				tui_set_clock();
				return;
			case 2:
				{
					PGM_P l1 = PSTR("RTC IS");
					if (rtc_valid()) {
						tui_gen_message(l1,PSTR("VALID"));
					} else {
						tui_gen_message(l1,PSTR("INVALID"));
					}
				}				
				break;
			default:
				return;
		}
	}
}
