/**
  \file   cfg.c
  \author Aleksey Lekhtin
  \date   April 5, 2024
  \brief  Config variables
 */

#include <libgen.h>
#include <stdlib.h>
#include <string.h>

#include "cfg.h"
#include "uart.h"
#include "ver.h"

cfg_t cfg;

//---------------------------------------------------------
#ifdef LINUX_VARIANT
void usage(const char *appname)
{
    uart_putc('\n');
    about();
    print_app_version();
    uart_putc('\n');
    uart_printf("Usage: %s [option]\n", (appname ? appname : "app"));
    uart_puts("where option\n"
	      "  -d <level>       level - 1..3\n"
	      "  -v               print version\n"
	      "  -h               this help\n"
	      "\n");
}
#endif

//---------------------------------------------------------
void help()
{
    uart_puts("\nCommands:\n"
	      "  - 0              - Select ADC to operate (1..4)\n"
	      "  - 1              - Read Device Status\n"
	      "  - 2              - SRAM Status\n"
	      "  - 3              - SRAM check\n"
	      "  - 4              - SRAM dump\n"
	      "  - 5              - Clear encoders\n"
	      "  - 6              - Read encoders\n"
	      "\n"
	      "  - i              - Init all\n"
	      "  - S              - Setup commands\n"
	      "  - C              - Calibration commands\n"
	      "  - t  (T)         - ADC Autoscan Start/Stop\n"
	      "  - u              - Read ADC Output Registers\n"
#ifdef INTERRUPT
	      "  - g              - Enable/Disable interrupts\n"
	      "  - G              - Print Generic Interrupt Conntroller register\n"
#endif
	      "\n"
	      "  - l              - Print list of ADC Registers\n"
	      "  - x              - Print all ADC registers\n"
	      "\n"
	      "ADC Commands:\n"
	      "  - a              - Start ADC and read conversion\n"
	      "  - n              - ADC NOP (to validate the CRC response byte)\n"
	      "  - b              - Start ADC\n"
	      "  - c              - Conversion data read\n"
	      "  - e              - Stop ADC\n"
	      "  - r              - Read from ADC Registers\n"
	      "  - w              - Write to ADC Registers\n"
	      "  - z              - Reset ADC\n"
	      "\n"
	      "  - v              - Version app (and FPGA Firmware Version)\n"
	      "  - h              - Help\n"
#ifdef LINUX_VARIANT
	      "  - q  ctrl/x      - Exit\n"
#endif
	      "\n");
}

//---------------------------------------------------------
#ifdef LINUX_VARIANT
bool parse_args(int ac, char *av[])
{
    for (int i = 1; i < ac; ++i) {
	if (strcasecmp(av[i], "-v") == 0 || strcasecmp(av[i], "/v") == 0) {
	    uart_putc('\n');
	    print_app_version();
	    uart_putc('\n');
	    return DONE;
	}

	if (strcasecmp(av[i], "-h") == 0 || strcasecmp(av[i], "--help") == 0 || strcasecmp(av[i], "/?") == 0 ||
	    strcasecmp(av[i], "?") == 0) {
	    char *appname = basename(av[0]);
	    char *p = strrchr(appname, '.');
	    if (p) *p = 0;
	    usage(appname);
	    return DONE;
	}

	if (strcasecmp(av[i], "-d") == 0) {
	    if ((i + 1) == ac) {
		cfg.debug_level = 1;
		continue;
	    }
	    cfg.debug_level = (int)strtoul(av[i + 1], NULL, 10);
	    ++i;
	    continue;
	}

	if (strncasecmp(av[i], "-d", 2) == 0 && strlen(av[i]) > 2) {
	    cfg.debug_level = (int)strtoul(av[i] + 2, NULL, 10);
	    continue;
	}
    }

    return OK;
}
#endif
