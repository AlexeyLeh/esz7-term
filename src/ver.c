//
// Created by LAV on 05.08.2026.
//

#include "ver.h"
#include "uart.h"

/**
 * Вывод версии
 */
//---------------------------------------------------------
inline const char *version()
{
    return APP_NAME " Ver. " VER_NUM " " VER_DATE " [built " __DATE__ " " __TIME__ "]";
}

//---------------------------------------------------------
void about()
{
    uart_printf("XME0724-20 Xilinx SoC (ESZ7 Control Console)\n");
}

//---------------------------------------------------------
void print_app_version()
{
    uart_printf("%s\n", version());
}
