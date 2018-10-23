#pragma once

uint8_t rtc_read(struct mtm* tm);
void rtc_write(struct mtm* tm);
uint8_t rtc_valid(void);
