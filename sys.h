#ifndef __SYS_H
#define __SYS_H

#include <stdint.h>
#include "ch558.h"

//#define USE_EXT_OSC
#define FREQ_SYS        48000000
#define UART_BUF_SIZE   32
#define UART1_TIMEOUT   1000


void mset(uint8_t* ptr, uint8_t val, size_t siz);
void mmov(uint8_t* dst, uint8_t* src, size_t siz);

void sysClockConfig();
void delay(uint16_t t);
void delay_us(uint16_t t);
void sysTickConfig();
uint32_t sysGetTickCount();

void uart0Config(uint32_t baud);
void uart0Send(uint8_t data);
void uart0Write(char* s);
uint8_t uart0Get();
uint8_t uart0Gets(uint8_t* dat, uint8_t len);
uint16_t uart0BlockRecv(uint8_t* ptr, uint16_t siz);

void uart1Config(uint32_t baud);
void uart1Send(uint8_t data);
void uart1Write(char* s);
uint8_t uart1Get();
uint8_t uart1Gets(uint8_t* dat, uint8_t len);

#endif
