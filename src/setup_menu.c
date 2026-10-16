/**
\file   setup_stuff.c
\author Aleksey Lekhtin
\date   April 5, 2024
\brief  Setup some features in Dev and ADC
 */

#include <stdbool.h>

#include "dev.h"
#include "interrupts.h"
#include "funcs.h"
#include "sram.h"
#include "sram_t.h"
#include "uart.h"

static void setup_help() // setup help
{
    uart_puts("\nSetup commands\n");
    uart_puts("   0 - Set ADC MODE0 Register\n");
    uart_puts("   1 - Set ADC MODE1 Register\n");
    uart_puts("   3 - Set ADC MODE3 Register\n");
    uart_puts("   4 - Set ADC MODE4 Register\n");
    uart_puts("   r - Set ADC REF   Register\n");
    uart_putc('\n');
    uart_puts("   5 - Set SRAM buffer length\n");
    uart_putc('\n');
    uart_puts("   t - Set Timer value\n");
    uart_puts("   b - Set Debounce threshold (for encoders)\n");
    uart_putc('\n');
    uart_puts("   d - Set debug level\n");

    uart_putc('\n');
    uart_puts("(ESC - cancel input or back to Main menu)\n");
}

void setup_stuff() // Setup device and ADC
{
    int err;
    uint8_t reg_num;
    uint8_t value;
    uint32_t num;
    uint32_t cmd;

#ifdef INTERRUPT
    bool irqCurrentStatus = gic_irq_enable(NO);
#endif

    setup_help();

    do {
        while (1) {
            uart_puts("\n* setup> ");
            cmd = GetSymb();
            if (cmd == 0x1b) {
#ifdef INTERRUPT
                if (irqCurrentStatus == ON) {
                    uart_printf("\nrestore irq status - ");
                    gic_irq_enable(irqCurrentStatus);
                }
#endif
                return;
            }
            if (cmd != '\r' && cmd != '\n') break;
        }
        uart_puts("\n");

        switch (cmd) {
        case 'h':
        {
            setup_help();
            break;
        }

        case '0': // ADC register MODE0
        {
            reg_num = RegMode0;
            if (!ADCReadRegister(selectedADC, reg_num, &value)) break;
            uart_printf("ADC-%d Reg: %02Xh - 0x%02X - ", selectedADC, reg_num, value);
            uart_puts(print_response_details(reg_num, value));
            uart_printf("\nSet new value (default: 0x24) : 0x");
            if (GetNumber(HEXMODE, &num, 2) != GET_OK) break;
            if (num == GET_DEFAULT) {
		        num = 0x24;
		        uart_printf("%02Xh", num);
	        }
            uart_puts("\n\n");
            ADCWriteRegister(selectedADC, reg_num, num);
            break;
        }

        case '1': // ADC register MODE1
        {
            reg_num = RegMode1;
            if (!ADCReadRegister(selectedADC, reg_num, &value)) break;
            uart_puts(print_response_details(reg_num, value));
            uart_printf("\nSet new value (default: 0x01) : 0x");
            if (GetNumber(HEXMODE, &num, 2) != GET_OK) break;
            if (num == GET_DEFAULT) {
		        num = 0x01;
		        uart_printf("%02Xh", num);
	        }
            uart_puts("\n\n");
            ADCWriteRegister(selectedADC, reg_num, num);
            break;
        }

        case '3': // ADC register MODE3
        {
            reg_num = RegMode3;
            if (!ADCReadRegister(selectedADC, reg_num, &value)) break;
            uart_puts(print_response_details(reg_num, value));
            uart_printf("\nSet new value (default: 0x00) : 0x");
            if (GetNumber(HEXMODE, &num, 8) != GET_OK) break;
            if (num == GET_DEFAULT) {
		        num = 0x24;
		        uart_printf("%02Xh", num);
	        }   
            uart_puts("\n\n");
            ADCWriteRegister(selectedADC, reg_num, num);
            break;
        }

        case '4': // ADC register MODE4
        {
            reg_num = RegMode4;
            if (!ADCReadRegister(selectedADC, reg_num, &value)) break;
            uart_puts(print_response_details(reg_num, value));
            uart_printf("\nSet new value (default: 0x50 (MUX:5, GAIN:0)) : 0x");
            if (GetNumber(HEXMODE, &num, 2) != GET_OK) break;
            if (num == GET_DEFAULT) {
		        num = 0x50;
		        uart_printf("%02Xh", num);
	        }
            uart_puts("\n\n");
            ADCWriteRegister(selectedADC, reg_num, num);
            break;
        }

        case 'r': // ADC register REF
        {
            reg_num = RegRef;
            if (!ADCReadRegister(selectedADC, reg_num, &value)) break;
            uart_puts(print_response_details(reg_num, value));
            uart_printf("\nSet new value (default: 0x00) : 0x");
            if (GetNumber(HEXMODE, &num, 2) != GET_OK) break;
            if (num == GET_DEFAULT) {
		        num = 0x00;
		        uart_printf("%02Xh", num);
	        }
            uart_puts("\n\n");
            ADCWriteRegister(selectedADC, reg_num, num);
            break;
        }

        case '5': // Set SRAM buffer length
        {
            SramStat_t sram_stat;
            GetSramStat(&sram_stat);
            uint32_t recs = GetSramLen(selectedADC, &sram_stat);
            uart_printf("Current value: %u\n", recs);
            uart_printf("Set SRAM buffer length (default: %d) (dec): ", recs);
            err = GetNumber(DECMODE, &num, 3);
            if (err) break;
            if (num == GET_DEFAULT) {
		        num = sram_recs();
		        uart_printf("%u", num);
	        }
            uart_putc('\n');

            CtrlMem_t ctrl = {0};
            ctrl.Channel = channel_to_onehot(selectedADC);
            ctrl.GvnLenBuf = num;
            WriteSramCtrlReg(&ctrl);
            uart_printf("set %u\n", num);
            break;
        }

        case 't': // Set timer period
        {
            uint32_t defnum = 100000;
            uart_printf("Value for AutoTimer (default: %d) (dec) : ", defnum);
            err = GetNumber(DECMODE, &num, 10); // 50 000 000 => 1 sec
            if (err) break;
            if (num == GET_DEFAULT) {
		        num = defnum;
		        uart_printf("%u", num);
	        }
            uart_putc('\n');
			SetADCAutoscanPeriod(selectedADC, num);
            uart_printf("set %d\n", num);
            break;
        }

        case 'b': // Set debounce threshold for encoders
        {
            uint32_t debounce = *EncDebAdr;
            uint32_t defnum = 0x8000;
            uart_printf("Current value: %u (0x%04X)\n", debounce, debounce);
            uart_printf("Value for Encoders debounce (default: %u 0x%04X) (hex) : ",
                    defnum, defnum);
            err = GetNumber(HEXMODE, &num, 10);
            if (err) break;
            if (num == GET_DEFAULT) {
		        num = defnum;
		        uart_printf("%02Xh", num);
	        }
            uart_putc('\n');
            *EncDebAdr = num;
            num = *EncDebAdr;
            uart_printf("set %02Xh (%d)\n", num, num);
            break;
        }

        case 'd': // set debug level
        {
            set_debug_level();
            break;
        }

        default: // bad choice
        {
            uart_puts("invalid command\n");
            break;
        }
        } // switch
    } while (1);

}
