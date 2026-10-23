/**
\file   main.c
\author Aleksey Lekhtin
\date   August 12, 2024
\brief  Main.
 */

#include "cfg.h"
#include "dev.h"
#include "funcs.h"
#include "interrupts.h"
#include "mem.h"
#include "sram.h"
#include "uart.h"
#include "ver.h"

//---------------------------------------------------------
int main(int ac, char *av[])
{
#ifdef LINUX_VARIANT
    if (!init_uart("/dev/tty")) {
	return 1;
    }
    if (parse_args(ac, av) == DONE) {
	return 0;
    }
#endif

    //-------------------------------------------
    uart_puts("\n\n");
    if (!setup_registers_memory()) {
        return 1;
    }

    about();
    print_firmware_version();
    print_app_version();
    uart_putc('\n');

    if (cfg.debug_level > 0) {
	uart_printf("\ndebug_level: %d\n", cfg.debug_level);
	uart_puts("-- to change 'Setup command -> Set debug' ");
	uart_puts("['s' -> 'd']\n\n");
    }

#ifdef INTERRUPT
    if (!gic_inited()) {
	if (!gic_init()) {
	    return 1;
	}
	// gic_irq_enable(true);
    }
#endif

    sram_check(0);

    uart_putc('\n');
    uart_puts("-- type 'h' to see available commands\n");
    uart_puts("-- Press ESC to cancel any operation(s)\n\n");


    //-------------------------------------------
    uint8_t cmd = 0;
    //cmd = 'L';
    int work = 1;
    while (work) {
#if 0
	bool autoscan = autoscan_mode(selectedADC);
#ifdef INTERRUPT
	uart_printf("GIC%u IRQ%d:%u T%d ADC-%d> ",
				(gic_inited() ? 1 : 0),
				(irq_enabled ? 1 : 0),
				irq_counter,
				(autoscan ? 1 : 0), selectedADC);
#else
	uart_printf("T%d ADC-%d> ", (autoscan ? 1 : 0), selectedADC);
#endif
#endif

	uart_printf("ADC-%d> ", selectedADC);

        if (cmd == 0) { // for debug
            cmd = GetSymb(); // Wait input & Send echo
            // uart_printf("cmd: %Xh\n", cmd);
            if (cmd != '\r' && cmd != '\n') {
                uart_putc('\n');
            }
        }

	//-- control word analysis ------------------------------------------
	switch (cmd) {
#ifdef LINUX_VARIANT
	case 'q':
	case 0x18: // CAN
	    {
		// exit
		work = 0;
		break;
	    }
#endif
	case 'h':
	case '?':
	    {
		// help menu
		help();
		break;
	    }

	case 'v':
	    {
		about();
		print_app_version();
		print_firmware_version();
		break;
	    }

	case '0':
	    {
		select_adc();
		break;
	    }

	case '1':
	    {
		DevStat_t stat = ReadDevStat();
		print_dev_status(&stat);
		break;
	    }

	case '2':
	    {
		SramStat_t sram_stat;
		GetSramStat(&sram_stat);
		for (int ch = 1; ch <= sram_total_channels(); ch++) {
		    uart_printf("sram-%d - ", ch);
		    print_sram_status(ch, &sram_stat);
		}
		break;
	    }

	case '3':
	    {
		sram_check(1);
		break;
	    }

	case '4':
	    {
		sram_dump();
		break;
	    }

	case '5':
	    {
		clear_encoders();
		break;
	    }

	case '6':
	    {
		read_encoders();
		break;
	    }

	case 'i':
	    {
		init();
		break;
	    }

	case 'l':
	    {
		adc_lists_registers();
		break;
	    }

	case 'S':
	    {
		setup_stuff();
		break;
	    }

	case 'L':
	    {
		check_lights();
		break;
	    }

	case 'C':
	    {
		calibration_stuff();
		break;
	    }

	case 't':
	    {
		AutoScan_control(selectedADC);
		break;
	    }

	case 'T':
	    {
		AutoScan_control_all();
		break;
	    }

	// ADC Commands:
	case 'a':
	    {
		adc_start_and_rdata();
		break;
	    }

	case 'n':
	    {
		adc_nop();
		break;
	    }

	case 'b':
	    {
		adc_start();
		break;
	    }

	case 'c':
	    {
		adc_rdata();
		break;
	    }

	case 'e':
	    {
		adc_stop();
		break;
	    }

#ifdef INTERRUPT
	case 'g':
	    {
		enable_irq();
		break;
	    }

	case 'G':
	    {
		print_irq_regs(0);
		print_gic_regs();
		break;
	    }
#endif

	case 'r':
	    {
		adc_read_register();
		break;
	    }

	case 'w':
	    {
		adc_write_register();
		break;
	    }

	case 'x':
	    {
		adc_read_regs();
		break;
	    }

	case 'u':
	    {
		DevReadADCOutputRegs();
		break;
	    }

	case 'z':
	    {
		adc_reset();
		break;
	    }

	default:
	    {
		// bad input
		WaitRxEnd();
		if (cmd != '\r' && cmd != '\n' && cmd != 0x1B) {
		    uart_puts("invalid command\n");
		}
		break;
	    }
	} // end of switch(cmd)

	if (cmd != 0x1B) {
	    uart_puts("\n");
	}

        cmd = 0;

    } // end while(1);

    finish();
    return 0;
}
