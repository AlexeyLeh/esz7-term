/*!
 \file   Uart.c
 \author Aleksey Lekhtin
 \date   8.04.2024
 \brief  uart_t (for Xilinx linux)
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

#include "cfg.h"

#ifdef LINUX_VARIANT

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>

#include "uart_lnx.h"

uart_t Uart;

static bool uart_init(uart_t *uart, const char *devname)
{
    memset(uart, 0, sizeof(uart_t));
    uart->buf_size = UARTBUF_SIZE;
    uart->fd = -1;
    strncpy(uart->devname, devname, sizeof(uart->devname) - 1);

    // int fd = open(uart->devname, O_RDWR | O_NOCTTY | O_SYNC);
    int fd = open(uart->devname, O_RDWR | O_NOCTTY | O_SYNC | O_NONBLOCK);
    if (fd < 0) {
        int e = errno;
        snprintf(uart->description, sizeof(uart->description) - 1,
                 "ERROR: can't open UART %d, fd: %d, devname: %s, err: %d, %s ", uart_port(), uart_addr(), uart_name(),
                 e, strerror(e));
        return 0;
    }
    // fcntl(fd, F_SETFL, O_NONBLOCK); // FNDELAY
    uart->fd = fd;
    uart->inited = 1;
    snprintf(uart->description, sizeof(uart->description) - 1, "Uart id: %d, fd: %d, devname: %s", uart_port(),
             uart_addr(), uart_name());

    tcgetattr(uart->fd, &uart->termsets_old);
    uart->termsets = uart->termsets_old;
    uart->termsets.c_lflag &= ~(ECHO | ICANON);
    // uart->termsets.c_cc[VMIN] = 1;
    // uart->termsets.c_cc[VTIME] = 1;
    tcsetattr(uart->fd, TCSANOW, &uart->termsets);
    return 1;
}

int uart_recv()
{
    // LINUX VARIANT
    Uart.buf_got = (int)read(Uart.fd, Uart.buf, Uart.buf_size);
    if (Uart.buf_got > 0) {
        Uart.buf[Uart.buf_got] = '\0';
        if (cfg.debug_level >= 2) {
            uart_printf("\r\ngot: %d - %x %x %x %x %x ... \r\n",
                        Uart.buf_got, Uart.buf[0], Uart.buf[1], Uart.buf[2], Uart.buf[3], Uart.buf[4]);
        }
    }
    tcflush(Uart.fd, TCIFLUSH);
    usleep(1000);
    return Uart.buf_got;
}

void uart_reset()
{
    if (Uart.fd > 0) {
        usleep(1000);
        tcflush(Uart.fd, TCIOFLUSH);
        int ret = tcsetattr(Uart.fd, TCSANOW, &Uart.termsets_old);
        if (ret == -1)
            fprintf(stderr, "tcsetattr() error while restore original settings \nerr:%d, %s\n", errno, strerror(errno));
        close(Uart.fd);
    }
    Uart.fd = -1;
}

bool is_uart_sending()
{
#if 0
#ifdef __USE_MISC
    /* ioctl (fd, TIOCSERGETLSR, &result) where result may be as below */
#define TIOCSER_TEMT 0x01 /* Transmitter physically empty */
#endif
#endif
    tcflush(Uart.fd, TCIFLUSH);
    return 0;
}

int uart_port()
{
    char *p = Uart.devname + strlen(Uart.devname) - 1;
    int port = (int)strtol(p, NULL, 10);
    if (errno != 0) port = 0;
    return port;
}

const char *uart_name()
{
    return Uart.devname;
}

const char *uart_description()
{
    return Uart.description;
}

uint32_t uart_addr()
{
    return Uart.fd;
}

/**
 * Поиск и инициализация консоли
 * \return
 * - 1 - успешно
 * - 0 - ошибка
 */
int init_uart(const char *devname)
{
    if (!uart_init(&Uart, devname)) {
        fprintf(stderr, "\n\n\n%s\n", Uart.description);
        return 0;
    }

    usleep(1000 * 10);
    return 1;
}

#if 0
bool set_blocking(int fd, int should_block)
{
    struct termios tty;
    memset(&tty, 0, sizeof tty);
    if (tcgetattr (fd, &tty) != 0) return 0; // errno

    tty.c_cc[VMIN]  = should_block ? 1 : 0;
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout
    if (tcsetattr(fd, TCSANOW, &tty) != 0) return 0; // errno
    return 1;
}
#endif

uint8_t uart_any_key_pressed()
{
    int num = uart_recv();
    if (num > 0) {
        return (uint8_t)Uart.buf[0];
    }
    return '\0';
}

int uart_printf(const char *format, ...)
{
    static char buf[4 * 1024];
    if (!Uart.inited) return 0;

    va_list ap;
    va_start(ap, format);
    vsnprintf(buf, sizeof(buf), format, ap);
    fflush(stdout);
    va_end(ap);

    size_t len = strlen(buf);
    write(Uart.fd, buf, len);
    return (int)len;
}

#endif // LINUX_VARIANT
