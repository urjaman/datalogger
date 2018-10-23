#include "main.h"
#include "timer.h"
#include "logger.h"
#include "fat.h"
#include "partition.h"
#include "sd_raw.h"
#include <stdio.h>

#define LOGBUF_SZ 592

#define LOGBUF_FLUSH 512
#define LOGGER_INTERVAL 300

static char logbuf[LOGBUF_SZ];
static uint16_t logbuf_woff = 0;


static void logger_line(void) {
	const int log_len = 40;
	struct mtm tm;
	if (logbuf_woff >= (LOGBUF_SZ-log_len)) return;
	timer_get_time(&tm);
	logbuf_woff += sprintf_P(logbuf + logbuf_woff,
	     /*  4     3    3    3    3    3   3  11 1 */
		PSTR("%04u-%02u-%02u %02u:%02:%02u,%c,%010lu\n"),
		tm.year + TIME_EPOCH_YEAR, tm.month, tm.day,
		tm.hour, tm.min, tm.sec, timer_time_isvalid() ? '*' : '?',
		timer_get()
	);
}

static uint32_t next_log;

static int8_t sd_stat = 0;
static struct partition_struct sd_part;
static struct fat_fs_struct sd_fat;
static struct fat_file_struct log_file;

static void logger_sd_init(void) {
	PGM_P fn_P = PSTR("DATALOG.TXT");
	char fn[12];
	strcpy_P(fn, fn_P);
	if (sd_stat != 0) return;
	if (!sd_raw_init()) return;
        struct partition_struct* partition = 
		partition_open(sd_raw_read,
			sd_raw_read_interval,
			sd_raw_write,
			sd_raw_write_interval,
			0, &sd_part);
        if (!partition) {
		/* try superfloppy mode */
		partition = partition_open(sd_raw_read,
                                       sd_raw_read_interval,
                                       sd_raw_write,
                                       sd_raw_write_interval,
                                       -1, &sd_part
                                      );
		if(!partition) return;
        }
        struct fat_fs_struct* fs = fat_open(partition, &sd_fat);
        if (!fs) {
		goto err_partition;
        }
	struct fat_dir_struct d_in;
        struct fat_dir_entry_struct directory;
        fat_get_dir_entry_of_path(fs, "/", &directory);

        struct fat_dir_struct* dd = fat_open_dir(fs, &directory, &d_in);
	if (!dd) {
		goto err_fat;
	}
	struct fat_dir_entry_struct file_de;
	uint8_t r = fat_create_file(dd, fn, &file_de);
	fat_close_dir(dd);
	if (r == 1) goto err_fat;
	struct fat_file_struct *fp = fat_open_file(fs, &file_de, &log_file);
	if (!fp) {
		goto err_fat;
	}
	if (!fat_seek_file(fp, 0, FAT_SEEK_END)) {
		fat_close_file(fp);
		goto err_fat;
	}
	sd_stat = 1;
	return;
	
err_fat:
        fat_close(fs);
err_partition:
        partition_close(partition);
}

/* Try to safely "detach" from the SD. */
static void logger_sd_detach(void) {
	fat_close_file(&log_file);
	fat_close(&sd_fat);
	partition_close(&sd_part);
	sd_raw_sync();
	sd_stat = 0;
}

static void logger_flush(void) {
	if (sd_stat != 1) return;
	if (!logbuf_woff) return;
	if (fat_write_file(&log_file, (void*)logbuf, logbuf_woff) != (int)logbuf_woff) {
		logger_sd_detach();
		return;
	}
	if (!sd_raw_sync()) {
		logger_sd_detach();
		return;
	}
	logbuf_woff = 0;
}

void logger_init(void) {
	next_log = 5;
}

void logger_run(void) {
	if (timer_get_1hzp()) {
		uint32_t now = timer_get();
		int32_t diff = next_log - now;
		if (diff >= 0) {
			logger_line();
			if (!sd_stat) {
				logger_sd_init();
			} else {
				/* "poll" the card */
				struct sd_raw_info dummy;
				if (!sd_raw_get_info(&dummy)) {
					logger_sd_detach();
					logger_sd_init();
				}
			}
			next_log = now + LOGGER_INTERVAL;
			if (logbuf_woff >= LOGBUF_FLUSH) {
				logger_flush();
			}
		}
	}
}

void logger_sd_eject(uint8_t eject) {
	if (eject) {
		logger_flush();
		logger_sd_detach();
		sd_stat = -1;
	} else if (sd_stat == -1) {
		sd_stat = 0;
		logger_sd_init();
	}
}

uint8_t logger_sd_status(void) {
	return sd_stat;
}