/*!
    \file   uart_lnx.h
    \author Aleksey Lekhtin
    \date   8.04.2024
    \brief  Uart (for linux)
 */

/******************************************************************************
 * Copyright (C) 2023 - 2024 E-Service.  All rights reserved.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 * IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 ******************************************************************************/

#ifndef UART_LNX_H_
#define UART_LNX_H_

#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <termios.h>
#include <unistd.h>

#define UARTBUF_SIZE 512

typedef struct uart_t
{
    char devname[256];
    char description[1024];
    int inited;
    int fd;
    struct termios termsets_old;
    struct termios termsets;

    char buf[UARTBUF_SIZE+1];
    int buf_size;
    int buf_got;
} uart_t;

extern uart_t Uart;

int init_uart(const char *devname);
void uart_reset();
int uart_port();
uint32_t uart_addr();
const char *uart_name();
const char *uart_description();
bool is_uart_sending();
int uart_recv();
uint8_t uart_any_key_pressed();

int uart_printf(const char *format, ...);

#endif // UART_LNX_H_
