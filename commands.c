#include "main.h"
#include "swi2c.h"
#include "uart.h"
#include "console.h"
#include "lib.h"
#include "ciface.h"
#include "appdb.h"
#include "lcd.h"
#include "timer.h"
#include "buttons.h"
#include "sd_raw.h"
#include "partition.h"
#include "fat.h"

CIFACE_APP(lcd_cmd, "LCDINIT")
{
	swi2c_init();
	lcd_init();
	lcd_clear();
	lcd_puts_dw_P(PSTR("Hello World!"));
}


CIFACE_APP(swi2c_scan_cmd, "SWI2CSCAN")
{
	uint8_t a=2;
	do {
		if (!swi2c_writem(a, 0, 0)) {
			luint2outdual(a);
		}
		a += 2;
	} while (a);	
}

CIFACE_APP(btns_cmd, "BTNS")
{
	sendstr_P(PSTR("BUTTON_"));
	uint8_t v = buttons_get();
	switch (v) {
		default:
			sendstr_P(PSTR("UNKNOWN"));
			break;
		case BUTTON_S1:
			sendstr_P(PSTR("S1"));
			break;
		case BUTTON_S2:
			sendstr_P(PSTR("S2"));
			break;
		case BUTTON_NONE:
			sendstr_P(PSTR("NONE"));
			break;
		case BUTTON_BOTH:
			sendstr_P(PSTR("BOTH"));
			break;
	}
}

CIFACE_APP(timer_cmd, "TIMER")
{
	luint2outdual(timer_get());
}

static uint8_t sd_initialized = 0;
static uint8_t sd_init(void) {
	if (!sd_raw_init()) {
		sd_initialized = 0;
		sendstr_P(PSTR("init failed"));
		return 0;
	}
	sd_initialized = 1;
	return 1;
}

CIFACE_APP(sdtest_cmd, "SDTEST")
{
	struct sd_raw_info info;
	if (!sd_init()) return;
	if (!sd_raw_get_info(&info)) {
		sendstr_P(PSTR("get_info failed"));
		return;
	}
	sendstr_P(PSTR("SD MiB:"));
	luint2outdual(info.capacity / (1024UL*1024));
}

CIFACE_APP(fattest_cmd, "FATTEST")
{
	if (!sd_initialized) {
		if (!sd_init()) return;
	}
	struct partition_struct part;
        struct partition_struct* partition = 
		partition_open(sd_raw_read,
			sd_raw_read_interval,
			sd_raw_write,
			sd_raw_write_interval,
			0, &part);
        if(!partition) {
		/* try superfloppy mode */
		partition = partition_open(sd_raw_read,
                                       sd_raw_read_interval,
                                       sd_raw_write,
                                       sd_raw_write_interval,
                                       -1, &part
                                      );
		if(!partition) {
			sendstr_P(PSTR("opening partition failed"));
			return;
		}
        }

        /* open file system */
	struct fat_fs_struct fat;
        struct fat_fs_struct* fs = fat_open(partition, &fat);
        if(!fs) {
		sendstr_P(PSTR("opening filesystem failed"));
		goto err_partition;
        }

        /* open root directory */
	struct fat_dir_struct d_in;
        struct fat_dir_entry_struct directory;
        fat_get_dir_entry_of_path(fs, "/", &directory);

        struct fat_dir_struct* dd = fat_open_dir(fs, &directory, &d_in);
        if(!dd) {
		sendstr_P(PSTR("opening root directory failed"));
		goto err_fat;
        }

	struct fat_dir_entry_struct dir_entry;
	while(fat_read_dir(dd, &dir_entry))
	{
		uint8_t spaces = sizeof(dir_entry.long_name) - strlen(dir_entry.long_name) + 4;
		sendstr(dir_entry.long_name);
		SEND(dir_entry.attributes & FAT_ATTRIB_DIR ? '/' : ' ');
		while(spaces--) SEND(' ');
		luint2outdual(dir_entry.file_size);
		sendstr_P(PSTR("\r\n"));
	}
        
        fat_close_dir(dd);
err_fat:
        fat_close(fs);
err_partition:
        partition_close(partition);
		
}