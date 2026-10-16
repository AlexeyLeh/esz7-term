/*!
    \file   uart.c
    \author Aleksey Lekhtin
    \date   8.04.2024
    \brief  Uart (common)
 */

#include <stdbool.h>

#include "adc_t.h"
#include "uart.h"

static const char hex_symbols[17] = "0123456789abcdef";

//---------------------------------------------------------------------------------------
uint8_t GetSymb()
{
    uint8_t Symb = Uart_Getc();
    if (isprint(Symb)) {
        uart_putc(Symb);
    }
    return Symb;
}

//---------------------------------------------------------------------------------------
/**
 * Read numberic symbol from Uart and return it as digital number
 * @return error
 * @return - 0 - succeded
 * @return - 1 - cancel (by ESC)
 */
int GetStr(char *buf, uint8_t buflen)
{
    uint8_t i = 0;
    while (i < buflen) {
        uint8_t ch = GetSymb();
        if (ch == 0x1b) return GET_CANCEL;
        if (ch == '\r' || ch == '\n') break;
        if (ch == 0x7F) { // backspace
            if (i > 0) {
                i--;
                buf[i] = '\0';
                uart_puts("\b \b");
            }
        } else {
            buf[i++] = ch;
        }
    }
    return GET_OK;
}

//---------------------------------------------------------------------------------------
/**
 * Read numberic symbol from Uart and return it as digital number
 * @return error
 * @return - 0 - succeded
 * @return - 1 - cancel (by ESC)
 * @return - 2 - error (invalid symbol, not number)
 */
int GetNumber(NUM_MODE mode, uint32_t *OutInt, uint8_t Len) //
{
    uint8_t c = 0;
    int Err = GET_OK;
    uint8_t num;
    uint8_t counter = 0;

    *OutInt = GET_DEFAULT;
    while (1) {
        c = GetSymb(); // Wait input & Send echo
        //uart_printf("\n> %02X %u '%c'\n", c, c, c);
        if (c == ' ') {
            continue;                        // Space
        } else if (c == '\r' || c == '\n') { // Return
            break;
        } else if (c == 0x1b) { // Esc
            Err = GET_CANCEL;
            break;
        } else if (c == 0x7F) { // backspace
            uart_puts("\b \b");
            if (mode == HEXMODE) {
                *OutInt = ((*OutInt) >> 4);
            }
            if (mode == DECMODE) {
                *OutInt = *OutInt / 10;
            }
            counter--;
        } else {
            if (mode == HEXMODE) {
                num = 0;
                if (!Char2HexBin(c, &num)) { // convert key to bin
                    Err = GET_ERR;
                    *OutInt = c;
                    break;
                }
                if (*OutInt == GET_DEFAULT) *OutInt = 0;
                *OutInt = ((*OutInt) << 4) | num;
            }
            if (mode == DECMODE) {
                if (!isdigit(c)) {
                    Err = GET_ERR;
                    *OutInt = c;
                    break;
                }
                if (*OutInt == GET_DEFAULT) *OutInt = 0;
                *OutInt = (*OutInt * 10) + (c - '0');
            }

            counter++;
            if (counter >= Len) break;
        }
    }
    WaitRxEnd();

#if 0
    Err = GET_OK;
    int Err = GetNumber(DECMODE, &adcData, 6);
    if (Err == GET_CANCEL) {
        uart_puts("\n");
    } else if (Err) {
        uart_puts("\n=== Incorrect number\n");
    }
#endif
    if (Err == GET_CANCEL) {
        uart_puts("\n");
    }

    return Err;
}
//---------------------------------------------------------------------------------------
// @return bool
// @return - true  - ok
// @return - false - error
//
bool Char2HexBin(char InChar, uint8_t *Val) // Return Low tetra!
{
    bool ok = true;
    if (InChar > 0x40) InChar &= 0xdf; // Convert to upper register
    switch (InChar) {
    case '0':
        *Val = 0x0;
        break;
    case '1':
        *Val = 0x1;
        break;
    case '2':
        *Val = 0x2;
        break;
    case '3':
        *Val = 0x3;
        break;
    case '4':
        *Val = 0x4;
        break;
    case '5':
        *Val = 0x5;
        break;
    case '6':
        *Val = 0x6;
        break;
    case '7':
        *Val = 0x7;
        break;
    case '8':
        *Val = 0x8;
        break;
    case '9':
        *Val = 0x9;
        break;
    case 'A':
        *Val = 0xa;
        break;
    case 'B':
        *Val = 0xb;
        break;
    case 'C':
        *Val = 0xc;
        break;
    case 'D':
        *Val = 0xd;
        break;
    case 'E':
        *Val = 0xe;
        break;
    case 'F':
        *Val = 0xf;
        break;
    default:
        ok = false;
        break;
    }
    return ok;
}

//---------------------------------------------------------------------------------------
/* Print 32-bit number as 8-digit hexadecimal value (with "0x" suffix).
 * @param[in] num - Number to print as hexadecimal.
 * @param[in] width - number of leading zero
 */
void PrintHexWord(bool x, uint32_t num)
{
    int i;
    uint8_t index;
    if (x) {
        uart_puts("0x");
    }
    for (i = 0; i < 8; i++) {
        index = (num >> (28 - 4 * i)) & 0x0F;
        uart_putc(hex_symbols[index]);
    }
}
//---------------------------------------------------------------------------------------
/* Print 8-bit number as 8-digit hexadecimal value (with "0x" suffix).
 @param[in] num Number to print as hexadecimal.*/
void PrintHexByte(bool x, uint8_t num)
{
    uint8_t index;
    if (x) uart_puts("0x");
    index = (num >> 4) & 0x0F;
    uart_putc(hex_symbols[index]);
    index = num & 0x0F;
    uart_putc(hex_symbols[index]);
}

//---------------------------------------------------------------------------------------
// Converts a given integer x to decimal string str[].
// d is the number of digits required in the output.
// If d is more than the number of digits in x,
// then 0s are added at the beginning.
uint8_t IntToDecStr(uint32_t x, uint8_t str[], uint8_t d)
{
    uint8_t i;
    uint8_t ii;
    uint8_t j;
    uint8_t temp;
    i = 0;
    while (x) {
        str[i++] = (x % 10) + '0';
        x = x / 10;
    }
    while (i < d) {
        str[i++] = '0';
    } // If number of digits required is more, then add 0s at the beginning

    //    reverse(str, i);
    ii = 0;
    j = i - 1; // len - 1;
    while (ii < j) {
        temp = str[ii];
        str[ii] = str[j];
        str[j] = temp;
        ii++;
        j--;
    }

    str[i] = '\0';
    return i;
}

//------------------------------------------------------------------
inline uint8_t Uart_IsRx()
{
#ifdef LINUX_VARIANT
    return uart_any_key_pressed();
#else
    uint32_t tmp;
    uint32_t val;
    tmp = Xil_In32(0xe000002c);
    tmp &= XUARTPS_SR_RXEMPTY;
    // if (!tmp)
    //     return true;
    // return false;
    if (!tmp) {
        val = *(volatile u32 *)0xE0000030;
        return (uint8_t)val;
    }
    return 0;
#endif
}

//-----------------
inline uint8_t Uart_Getc()
{
#ifdef LINUX_VARIANT
    while (1) {
        int num = uart_recv();
        if (num == 1) {
            WaitRxEnd();
            return (uint8_t)Uart.buf[0];
        }
    }
#else
    uint32_t tmp;
    uint32_t val;
    while (1) {
        tmp = *(volatile u32 *)0xE000002C;
        tmp &= XUARTPS_SR_RXEMPTY;
        if (!tmp) {
            val = *(volatile u32 *)0xE0000030;
            return (uint8_t)val;
        }
    }
#endif
}
//------------------------------------------------------------------
void WaitRxEnd() // need to empty fifo
{
#ifdef LINUX_VARIANT
    tcflush(Uart.fd, TCIFLUSH);
#else
    uint32_t d; // 31'nd bit in data - data available
    while (1) {
        d = *(volatile u32 *)0xE000002C;
        d &= XUARTPS_SR_RXEMPTY;
        if (d) break;
        d = *(volatile u32 *)0xE0000030;
        usleep(2000);
    }
    usleep(2000);
#endif
}

//-----------------
inline void uart_putc(uint8_t c)
{
#ifdef LINUX_VARIANT
    if (!Uart.inited) return;
    static char buf[2] = {0};
    buf[0] = c;
    write(Uart.fd, buf, 1);
#else
    uint32_t tmp;
    while (1) {
        tmp = *(volatile u32 *)0xE000002C;
        tmp &= XUARTPS_SR_TXFULL;
        if (!tmp) {
            volatile u32 *LocalAddr = (volatile u32 *)0xe0000030;
            *LocalAddr = c;
            break;
        }
    }
#endif
}

//-----------------
inline void uart_puts(const char *s)
{
    char c;
    while ((c = *s++)) {
        uart_putc(c);
    }
}

//-----------------
bool AkXUartPs_IsTransmitFull()
{
#ifdef LINUX_VARIANT
    return true;
#else
    uint32_t tmp;
    tmp = Xil_In32(0xe000002c);
    tmp &= XUARTPS_SR_TXFULL;
    if (tmp) return true;
    return false;
#endif
}
