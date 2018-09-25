#include "main.h"
#include "i2c.h"
#include "uart.h"
#include "console.h"
#include "lib.h"
#include "ciface.h"
#include "appdb.h"
#include "SSD1306.h"

CIFACE_APP(ssd1306_cmd, "SSD1306")
{
	swi2c_init();
	ssd1306_init();
	ssd1306_display();
}


CIFACE_APP(i2c_scan_cmd, "I2CSCAN")
{
	uint8_t a=2;
	do {
		if (!swi2c_writem(a, 0, 0)) {
			luint2outdual(a);
		}
		a += 2;
	} while (a);	
}