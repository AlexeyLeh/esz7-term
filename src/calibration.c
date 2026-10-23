/**
\file   calibration.c
\author Aleksey Lekhtin
\date   Sep  8, 2026
\brief  ADC Calibration preocedures
*/

#include "dev.h"
#include "funcs.h"
#include "uart.h"

#include <stdbool.h>
#include <stdlib.h>
#include <threads.h>

void diff__time(struct timespec *tm_begin, struct timespec *tm_end, struct timespec *diff);

static void calibration_help() // calibration help
{
    uart_puts("\ncalibration commands\n");
    uart_puts("   p - Print calibration information\n");
    uart_puts("   o - Offset calibration\n");
    uart_puts("   g - Gain calibration\n");
    uart_puts("   z - Reset calibration\n");
    uart_putc('\n');
    uart_puts("   P - ALL ADC print calibration information\n");
    uart_puts("   O - All ADC offset calibration\n");
    uart_puts("   G - All ADC gain calibration\n");
    uart_puts("   Z - All ADC reset calibration\n");
    uart_putc('\n');
    uart_puts("   q - exit\n");
    uart_putc('\n');
    uart_puts("(ESC - cancel input or back to Main menu)\n");
}

//-----------------------------------------------
/// @param msg Confirm message
///
static bool confirm_action(const char *msg)
{
    uart_printf("\nContinue (Y/N)? [N] ", msg);
    const uint8_t ch = GetSymb();
    if (ch != 'y' && ch != 'Y') {
	uart_puts("\n* no\n");
	return false;
    }
    uart_puts("\n\n");
    return true;
}

//-----------------------------------------------
/// @param [in] adc_num 1..4
/// @param[out] value regiser content
///
static bool read_offset_calibration_register(const int adc_num, uint32_t *value)
{
    // adc_num - 1..4
    *value = 0;
    uint8_t OFCAL0 = 0, OFCAL1 = 0, OFCAL2 = 0;
    AdcResp_t adc_resp;
    bool ok = ADC_DirectCommand(adc_num, CMDOP_RREG + RegOFCAL0, new_arbitrary(), &adc_resp);
    if (ok) {
	OFCAL0 = adc_resp.bits.AdcLsbData;

	ok = ADC_DirectCommand(adc_num, CMDOP_RREG + RegOFCAL1, new_arbitrary(), &adc_resp);
	if (ok) {
	    OFCAL1 = adc_resp.bits.AdcLsbData;

	    ok = ADC_DirectCommand(adc_num, CMDOP_RREG + RegOFCAL2, new_arbitrary(), &adc_resp);
	    if (ok) {
		OFCAL2 = adc_resp.bits.AdcLsbData;
	    }
	}
    }
    if (!ok) return FAIL;

    *value = OFCAL2 << 16;
    *value |= OFCAL1 << 8;
    *value |= OFCAL0;
    return OK;
}

//-----------------------------------------------
/// @param [in] adc_num 1..4
/// @param[out] value regiser content
///
static bool read_gain_calibration_register(const int adc_num, uint32_t *value)
{
    uint8_t FSCAL0 = 0, FSCAL1 = 0, FSCAL2 = 0;
    AdcResp_t adc_resp;
    bool ok = ADC_DirectCommand(adc_num, CMDOP_RREG + RegFSCAL0, new_arbitrary(), &adc_resp);
    if (ok) {
	FSCAL0 = adc_resp.bits.AdcLsbData;

	ok = ADC_DirectCommand(adc_num, CMDOP_RREG + RegFSCAL1, new_arbitrary(), &adc_resp);
	if (ok) {
	    FSCAL1 = adc_resp.bits.AdcLsbData;

	    ok = ADC_DirectCommand(adc_num, CMDOP_RREG + RegFSCAL2, new_arbitrary(), &adc_resp);
	    if (ok) {
		FSCAL2 = adc_resp.bits.AdcLsbData;
	    }
	}
    }
    if (!ok) return FAIL;

    *value = (FSCAL2 << 16);
    *value |= (FSCAL1 << 8);
    *value |= FSCAL0;
    return OK;
}

//-----------------------------------------------
/// @param adc_num 1..4
///
static void print_offset_calibration_info(const int adc_num)
{
    uint32_t value = 0;
    if (read_offset_calibration_register(adc_num, &value)) {
	uart_printf("ADC-%d Full-Scale: 0x%06X (%u)\n", adc_num, value, value);
    } else {
	uart_puts("       Full-Scale: ERROR\n\n");
    }
}

//-----------------------------------------------
/// @param adc_num 1..4
///
static void print_gain_calibration_info(const int adc_num)
{
    uint32_t value = 0;
    if (read_gain_calibration_register(adc_num, &value)) {
	uart_printf("ADC-%d Full-Scale: 0x%06X (%u)\n", adc_num, value, value);
    } else {
	uart_puts("       Full-Scale: ERROR\n\n");
    }
}

//-----------------------------------------------
/// @param adc_num 1..4
///
static void print_calibration_info(const int adc_num)
{
    print_offset_calibration_info(adc_num);
    print_gain_calibration_info(adc_num);
}

//---------------------------------------------------------
/// @param adc_num 1..4
///
static void reset_offset_calibration(const int adc_num)
{
    ADC_DirectCommand(adc_num, CMDOP_WREG + RegOFCAL0, 0x00, NULL);
    ADC_DirectCommand(adc_num, CMDOP_WREG + RegOFCAL1, 0x00, NULL);
    ADC_DirectCommand(adc_num, CMDOP_WREG + RegOFCAL2, 0x00, NULL);
    usleep(100000);
}

//---------------------------------------------------------
/// @param adc_num 1..4
///
static void reset_gain_calibration(const int adc_num)
{
    ADC_DirectCommand(adc_num, CMDOP_WREG + RegFSCAL0, 0x00, NULL);
    ADC_DirectCommand(adc_num, CMDOP_WREG + RegFSCAL1, 0x00, NULL);
    ADC_DirectCommand(adc_num, CMDOP_WREG + RegFSCAL2, 0x40, NULL);
    usleep(100000);
}

//---------------------------------------------------------
/// @param adc_num 1..4
///
static void reset_calibration(const int adc_num)
{
    uart_printf("ADC-%d Reset offset to default (0)\n", adc_num);
    reset_offset_calibration(adc_num);
    uart_printf("ADC-%d Reset gain to default (0x400000)\n", adc_num);
    reset_gain_calibration(adc_num);
    uart_putc('\n');
    print_calibration_info(adc_num);
}

//---------------------------------------------------------
/// @param adc_num 1..4
/// @param value
///
static bool set_calibration_offset(const int adc_num, const uint32_t value)
{
    bool ok = ADC_DirectCommand(adc_num, CMDOP_WREG + RegOFCAL0, value & 0xFF, NULL);
    if (ok) {
	ok = ADC_DirectCommand(adc_num, CMDOP_WREG + RegOFCAL1, (value >> 8) & 0xFF, NULL);
    }
    if (ok) {
	ADC_DirectCommand(adc_num, CMDOP_WREG + RegOFCAL2, (value >> 16) & 0xFF, NULL);
    }
    if (!ok) {
	uart_putc('\n');
	return false;
    }
    usleep(100000);
    return true;
}

//---------------------------------------------------------
/// @param adc_num 1..4
/// @param value
///
static bool set_calibration_gain(const int adc_num, const uint32_t value)
{
    bool ok = ADC_DirectCommand(adc_num, CMDOP_WREG + RegFSCAL0, value & 0xFF, NULL);
    if (ok) {
	ok = ADC_DirectCommand(adc_num, CMDOP_WREG + RegFSCAL1, (value >> 8) & 0xFF, NULL);
    }
    if (ok) {
	ADC_DirectCommand(adc_num, CMDOP_WREG + RegFSCAL2, (value >> 16) & 0xFF, NULL);
    }
    if (!ok) {
	uart_putc('\n');
	return false;
    }
    usleep(100000);
    return true;
}

//---------------------------------------------------------
/// @param[in] adc_num 1..4
/// @param[in] nsec how many seconds make conversions to accumulate min/max
/// @param[out] min_value
/// @param[out] max_value @param counts
///
static bool get_minimal_value(const int adc_num, const int nsec, int32_t *min_value, int32_t *max_value,
			      int32_t *counts)
{
    AdcResp_t resp;
    struct timespec tm1, tm2, tm;
    bool first = true;
    *counts = 0;

    clock_gettime(CLOCK_REALTIME, &tm1);

    for (;;) {
	clock_gettime(CLOCK_REALTIME, &tm2);
	diff__time(&tm1, &tm2, &tm);
	if (tm.tv_sec >= nsec) {
	    break;
	}

	bool ok = ADC_DirectCommand(adc_num, CMDOP_START, new_arbitrary(), NULL);
	if (ok) {
	    ok = ADC_DirectCommand(adc_num, CMDOP_RDATA, new_arbitrary(), &resp);
	}
	if (!ok) {
	    uart_putc('\n');
	    return false;
	}
	int32_t v = resp.bits.AdcMsbData << 16;
	v |= resp.bits.AdcMidData << 8;
	v |= resp.bits.AdcLsbData;
	v <<= 8; // знак размножить
	v >>= 8;

	if (first) {
	    first = false;
	    *min_value = v;
	    *max_value = v;
	} else {
	    if (v < *min_value) {
		*min_value = v;
	    } else if (v > *max_value) {
		*max_value = v;
	    }
	}
	(*counts)++;
    }
    return true;
}

//---------------------------------------------------------
/// @param adc_num 1..4
///
static void offset_calibration(const int adc_num)
{
    int32_t vmin, vmax, counts;
    uart_printf("\nADC-%d Current calibration values:\n", adc_num);
    print_offset_calibration_info(adc_num);

    uint32_t offset = 0;
    uart_printf("ADC-%d Reset offset to default (0)\n", adc_num);
    reset_offset_calibration(adc_num);

    uart_printf("ADC-%d Performing user offset calibration ...\n", adc_num);
    for (;;) {
	const int nsec = 5;
	if (!get_minimal_value(adc_num, nsec, &vmin, &vmax, &counts)) {
	    return; // error
	}
	uart_printf("ADC-%d got min/max value on %d items: %08X / %08X (%d / %d)\n", adc_num, counts, vmin, vmax, vmin,
		    vmax);
	if (vmin >= 0x100) {
	    break; // offset found
	}
	offset -= 0x100;
	set_calibration_offset(adc_num, offset);
    }

    uart_printf("ADC-%d Set offset calibration value:\n", adc_num);
    usleep(100000);
    print_offset_calibration_info(adc_num);
}

//---------------------------------------------------------
/// @param adc_num 1..4
/// @return Full Scale Calibration Value stored in ADC register
///
static uint32_t gain_calibration(const int adc_num)
{
    uart_printf("\nADC-%d Current calibration values:\n", adc_num);
    print_gain_calibration_info(adc_num);

    uart_printf("ADC-%d Reset gain to default (0x400000)\n", adc_num);
    reset_gain_calibration(adc_num);

    uart_printf("ADC-%d temporarily reduce the full-scale register by 95%% to 3CCCCCh\n", adc_num);
    set_calibration_gain(adc_num, 0x3CCCCC);
    print_gain_calibration_info(adc_num);

    const int nsec = 5;
    uart_printf("ADC-%d Performing user gain calibration %d sec ...\n", adc_num, nsec);
    int32_t vmin, vmax, counts;
    if (!get_minimal_value(adc_num, nsec, &vmin, &vmax, &counts)) {
	return 0; // error
    }
    uart_printf("ADC-%d got min/max value on %d items: %08X / %08X (%d / %d)\n", adc_num, counts, vmin, vmax, vmin,
		vmax);

    const uint32_t ExpectedCode = 0x799998; // 7fffff x 0.95
    const uint32_t ActualCode = vmax;
    const double v = (double)ExpectedCode / ActualCode * 0x400000;
    const uint32_t FullScaleCalibrationValue = (uint32_t)v;
    uart_printf("ADC-%d full-scale calibration value: %08X (%u)\n", adc_num, FullScaleCalibrationValue,
		FullScaleCalibrationValue);
    set_calibration_gain(adc_num, FullScaleCalibrationValue);
    usleep(100000);
    print_gain_calibration_info(adc_num);
    return FullScaleCalibrationValue;
}

//---------------------------------------------------------
static void gain_calibration_all()
{
    uart_puts("\nThe result of one ADC full-scale calibration will be applied to ALL ADC!\nSure?\n");
    uart_puts("Which ADC connected to full-scale source? (1 2 3 4) [ESC] ");
    const uint8_t ch = GetSymb();
    if (ch < '1' || ch > '4') {
	uart_puts("\n* cancel\n");
	return;
    }
    uart_puts("\n\n");
    const int adc_num = ch - '0';
    const uint32_t value = gain_calibration(adc_num);
    if (value > 0) {
	for (int i = 1; i <= 4; i++) {
	    if (i != adc_num) {
        	uart_printf("ADC-%d set full-scale calibration value: 0x%06X (%u)\n", adc_num, value, value);
		set_calibration_gain(i, value);
	    }
	}
    }
}

//---------------------------------------------------------
void calibration_stuff() // ADC calibration
{
    uint32_t cmd;
    calibration_help();
    do {
	while (1) {
	    uart_printf("\n* ADC-%d calibration> ", selectedADC);
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

	if (cmd == 'h') {
	    calibration_help();
	} else if (cmd == 'p') {
	    uart_puts("\nPrint calibration value (offset/full-scale)\n\n");
	    print_calibration_info(selectedADC);

	} else if (cmd == 'P') {
	    uart_puts("\nPrint calibration value (offset/full-scale)\n\n");
	    for (int adc_num = 1; adc_num <= 4; adc_num++) {
		print_calibration_info(adc_num);
		uart_putc('\n');
	    }

	} else if (cmd == 'o') {
	    offset_calibration(selectedADC);

	} else if (cmd == 'O') {
	    for (int i = 1; i <= 4; i++) {
		uart_putc('\n');
		offset_calibration(i);
	    }

	} else if (cmd == 'g') {
	    gain_calibration(selectedADC);

	} else if (cmd == 'G') {
	    gain_calibration_all();

	} else if (cmd == 'z') {
	    if (confirm_action("Perform reset calibration")) {
		uart_printf("Current values:\n");
		print_calibration_info(selectedADC);
		reset_calibration(selectedADC);
	    }

	} else if (cmd == 'Z') {
	    if (confirm_action("Perform reset calibration")) {
		for (int i = 1; i <= 4; i++) {
		    reset_calibration(i);
		}
	    }

	} else if (cmd == 'q') {
	    finish();
	    exit(0);
	} else {
	    uart_puts("invalid command\n");
	}
    } while (1);
}
