#include "main.h"
#include "timer.h"
#include "logger.h"
#include "fat.h"
#include "partition.h"
#include "sd_raw.h"
#include "ams2302.h"
#include <stdio.h>


#define LOGBUF_FLUSH 256
#define LOGGER_INTERVAL 300

static char logbuf[LOGBUF_SZ];
static uint16_t logbuf_woff = 0;

static void logger_line(void) {
	int16_t t10;
	uint16_t rh10;
	uint8_t ts[8], rhs[8];
	const int log_len = 56;
	struct mtm tm;
	if (logbuf_woff >= (LOGBUF_SZ-log_len)) return;
	timer_get_time(&tm);
	ts[0] = 0;
	rhs[0] = 0;
	if (!ams_get(&t10, &rh10, LOGGER_INTERVAL/2)) {
		make_v10_str(ts, t10);
		make_v10_str(rhs, rh10);
	}
	logbuf_woff += sprintf_P(logbuf + logbuf_woff,
	     /*  4     3    3    3    3    3   3  11 1 */
		PSTR("%04u-%02u-%02u %02u:%02u:%02u,%c,%010lu,%s,%s\n"),
		tm.year + TIME_EPOCH_YEAR, tm.month, tm.day,
		tm.hour, tm.min, tm.sec, timer_time_isvalid() ? '*' : '?',
		timer_get(), ts, rhs
	);
}

static uint32_t next_log;

static PGM_P fat_err;
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
		if(!partition) {
			fat_err = PSTR("No part");
			return;
		}
        }
        struct fat_fs_struct* fs = fat_open(partition, &sd_fat);
        if (!fs) {
		fat_err = PSTR("No fs");
		goto err_partition;
        }
	struct fat_dir_struct d_in;
        struct fat_dir_entry_struct directory;
        fat_get_dir_entry_of_path(fs, "/", &directory);

        struct fat_dir_struct* dd = fat_open_dir(fs, &directory, &d_in);
	if (!dd) {
		fat_err = PSTR("Open dir");
		goto err_fat;
	}
	struct fat_dir_entry_struct file_de;
	uint8_t r = fat_create_file(dd, fn, &file_de);
	fat_close_dir(dd);
	if (r == 0) {
		fat_err = PSTR("Create file");
		goto err_fat;
	}
	struct fat_file_struct *fp = fat_open_file(fs, &file_de, &log_file);
	if (!fp) {
		fat_err = PSTR("Open file");
		goto err_fat;
	}
	int32_t seek_off = 0;
	if (!fat_seek_file(fp, &seek_off, FAT_SEEK_END)) {
		fat_err = PSTR("Seek end");
		fat_close_file(fp);
		goto err_fat;
	}
	fat_err = NULL;
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
	//return;
	if (timer_get_1hzp()) {
		uint32_t now = timer_get();
		int32_t diff = next_log - now;
		if (diff <= 0) {
			logger_line();
			if (!sd_stat) {
				logger_sd_init();
			} else if (sd_stat == 1) {
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

uint16_t logger_buf_stat(void) {
	return logbuf_woff;
}

PGM_P logger_sd_err(void) {
	return fat_err;
}

uint32_t logger_log_size(void) {
	return log_file.pos;
}