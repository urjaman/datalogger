/*
 * Copyright (C) 2009 Urja Rannikko <urjaman@gmail.com>
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
#include "swi2c.h"
#include "i2c.h"
#include "uart.h"
#include "ciface.h"
#include "lcd.h"
#include "buttons.h"
#include "timer.h"
#include "tui.h"
#include "console.h"
#include "powermgmt.h"
#include "logger.h"

void mini_mainloop(void) {
	timer_run();
	if ((uart_isdata()) ||(getline_i) ) timer_activity();
	ciface_run();
	logger_run();
}

void main (void) __attribute__ ((noreturn));


void main(void) {
	uart_init();
	ciface_init();
	pm_init();
	timer_init();
	buttons_init();
	swi2c_init();
	lcd_init();
	i2c_init();
	logger_init();
	tui_init();
	for(;;) {
		mini_mainloop();
		tui_run();
	}
}


