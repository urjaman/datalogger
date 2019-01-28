#pragma once
#include <avr/pgmspace.h>
#include <stdlib.h>

PGM_P ams_get(int16_t *tempC10, uint16_t *rh10, uint8_t max_age);
void ams_init(void);
void ams_run(void);

void make_v10_str(unsigned char*buf, int16_t t10);
