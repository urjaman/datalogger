/*
 * Copyright (C) 2009,2016 Urja Rannikko <urjaman@gmail.com>
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

unsigned char uart_isdata(void);
unsigned char uart_recv(void);
void uart_send(unsigned char val);
void uart_init(void);
uint8_t uart_peek(void);
void uart_wait_txdone(void);
#define BAUD 9600
#define RECEIVE() uart_recv()
#define SEND(n) uart_send(n)
#define PEEK() uart_peek()
#define UART_BUFLEN 16
#define UARTTX_BUFLEN 16

