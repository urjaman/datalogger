#pragma once
/* This is the RCSwitch thing, but heavily modified
 * (from C++ to C, stripped down and removed the arduino API usage)
 * by Urja Rannikko 2018 - my modifications are under GPLv2,
 * as included in the file COPYING in this repository. */

/*
  RCSwitch - Arduino libary for remote control outlet switches
  Copyright (c) 2011 Suat Özgür.  All right reserved.

  Contributors:
  - Andre Koehler / info(at)tomate-online(dot)de
  - Gordeev Andrey Vladimirovich / gordeev(at)openpyro(dot)com
  - Skineffect / http://forum.ardumote.com/viewtopic.php?f=2&t=46
  - Dominik Fischer / dom_fischer(at)web(dot)de
  - Frank Oltmanns / <first name>.<last name>(at)gmail(dot)com
  - Max Horn / max(at)quendi(dot)de
  - Robert ter Vehn / <first name>.<last name>(at)gmail(dot)com

  (Original Project home: https://github.com/sui77/rc-switch/ )

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

*/

#include <stdint.h>
typedef uint8_t Bool;
#define true 1
#define false 0

//#define RCSW_RX

/* transmitter */
void rcsw_enable_tx(void);
void rcsw_disable_tx(void);
void rcsw_set_tx_proto(int nProtocol);
void rcsw_set_tx_pulselen(int nPulseLength);
void rcsw_set_tx_repeats(int nRepeatTransmit);
void rcsw_tx(uint64_t code, unsigned int length);

#ifdef RCSW_RX

/* receiver */
void rcsw_enable_rx(void);
void rcsw_disable_rx(void);
void rcsw_set_rx_tol(int nPercent);
Bool rcsw_has_rx_data(void);
unsigned long rcsw_get_rx_data(void);
unsigned int rcsw_get_rx_bitlen(void);
unsigned int rcsw_get_rx_delay(void);
unsigned int rcsw_get_rx_proto(void);
unsigned int* rcsw_get_rx_times(void);
void rcsw_clear_rx_data(void);

void rcsw_rx_debug_report(void);
#endif
