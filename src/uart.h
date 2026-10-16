#ifndef UART__H_
#define UART__H_

#include <stdbool.h>

#include "cfg.h"

#ifdef LINUX_VARIANT
#include <stdio.h>
#include <unistd.h>
#include "uart_lnx.h"
#else
#include <sleep.h>
#include "xil_printf.h"
#include "xparameters.h"
#include "xuartps.h"
#define uart_printf xil_printf
#endif // LINUX_VARIANT

#define GET_OK 0
#define GET_CANCEL 1
#define GET_ERR 2
#define GET_DEFAULT 0x7FFFFFFF

typedef enum NUM_MODE
{
    HEXMODE,
    DECMODE
} NUM_MODE;

int GetNumber(NUM_MODE mode, uint32_t *OutInt, uint8_t Len);
int GetStr(char *buf, uint8_t buflen);
uint8_t GetSymb();
void WaitRxEnd();

bool Char2HexBin(char InChar, uint8_t *Val);
void PrintHexWord(bool x, uint32_t num);
void PrintHexByte(bool x, uint8_t num);
uint8_t IntToDecStr(uint32_t x, uint8_t str[], uint8_t d);

uint8_t Uart_Getc();
uint8_t Uart_IsRx();
void uart_puts(const char *s);
void uart_putc(uint8_t c);

#endif // UART__H_
