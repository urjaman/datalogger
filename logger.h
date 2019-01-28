#pragma once

void logger_init(void);
void logger_run(void);
void logger_sd_eject(uint8_t eject);

uint8_t logger_sd_status(void);

PGM_P logger_sd_err(void);

uint16_t logger_buf_stat(void);
uint32_t logger_log_size(void);


#define LOGBUF_SZ 384

