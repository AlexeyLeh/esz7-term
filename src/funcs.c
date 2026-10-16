/**
\file   funcs.c
\author Aleksey Lekhtin
\date   April 5, 2024
\brief  Application features
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "crc.h"
#include "funcs.h"
#include "interrupts.h"
#include "sram.h"
#include "sram_t.h"
#include "uart.h"

int selectedADC = 1;

//---------------------------------------------------------
void select_adc()
{
    uart_printf("Current ADC: %d\n", selectedADC);
    uart_puts("Select ADC (1..4) [1]: ");
    int adc_num;
    int err = GetNumber(DECMODE, (uint32_t *)&adc_num, 1);
    if (err) {
	if (err == GET_CANCEL) {
	    uart_printf("\n[cancel]\n");
	    return;
	}
	uart_printf("\n\ninvalid data was entered\n");
	return;
    }
    if (adc_num == GET_DEFAULT) {
	adc_num = 1;
	uart_printf("%u", adc_num);
    }
    uart_putc('\n');

    if (adc_num <= 0 || adc_num > 4) {
	uart_printf("\nERROR: %d is invalid ADC number => (1..4)\n", adc_num);
	return;
    }

    selectedADC = adc_num;
    uart_printf("\nADC-%d is selected\n", selectedADC);

#if 0
    uint8_t value;
    if (ADCReadRegister(selectedADC, RegMode0, &value)) {
        // MODE0 factory default
        if (value == 0x24)     {
            uart_putc('\n');
            init();
        }
    }
#endif
}

//---------------------------------------------------------
static bool init_adc(int adc_num)
{
    // adc_num - 1..4
    // 1. Reset ADC
    uart_printf("ADC-%d  - RESET - ", adc_num);
    ADC_DirectCommand(adc_num, CMDOP_RESET, new_arbitrary(), NULL);
    // if (!ADC_DirectCommand(adc_num, CMDOP_RESET, new_arbitrary(), NULL)) {
    //     return FAIL;
    // }
    // uart_puts("OK\n");
    set_arbitrary(0);

    // 2. ADC - Check  ID (expected 0x4x)
    uint8_t id;
    uart_printf("ADC-%d  - ID ", adc_num);
    if (!ADCReadID(adc_num, &id)) return FAIL;
    uart_printf("%02X - %s", id, ((id & 0xF0) != 0x40 ? "FAIL" : ""));
    if ((id & 0xF0) != 0x40) return FAIL;
    uart_puts("OK\n");

    // 3. Setup ADC
    uint8_t value;
    // MODE 0 - DataRate & Filter
    value = 0x70; // default 0x24 - 20SPS & FIR
    if (!ADCWriteRegister(adc_num, RegMode0, value)) return FAIL;

    // MODE 1 - Conversion Mode & Conversion Start Delay
    value = 0x11; // Pulse & 50us
    if (!ADCWriteRegister(adc_num, RegMode1, value)) return FAIL;

    // MODE 3 - STATUS0 Byte Enable
    value = 0x40; // enable
    if (!ADCWriteRegister(adc_num, RegMode3, value)) return FAIL;

    // MODE 4 - Input Switch & PGA Gain
    value = 0x00; // External & 1.25
    if (!ADCWriteRegister(adc_num, RegMode4, value)) return FAIL;

    // REF - Reference Input Multiplexer
    value = 0x0A; // External
    if (!ADCWriteRegister(adc_num, RegRef, value)) return FAIL;
    return OK;
}

//---------------------------------------------------------
void init() // Init (all in one function)
{
    CtrlMem_t ctrl;
    SramStat_t sram_stat;

    uart_printf("Select ADC to init (1..4) (0 - all) [%d]: ", selectedADC);
    int adc_num;
    int err = GetNumber(DECMODE, (uint32_t *)&adc_num, 1);
    if (err) return;
    if (adc_num == GET_DEFAULT) {
	adc_num = selectedADC;
	uart_printf("%u", adc_num);
    }
    uart_putc('\n');
    if (adc_num > 4) {
	uart_printf("\nERROR: %d is invalid ADC number => (1..4)\n", adc_num);
	return;
    }

    // 1. DEV - Check status
    uart_puts("DEV    - Status: ");
    if (cfg.debug_level > 0) uart_putc('\n');

    DevStat_t stat = ReadDevStat();
    uart_printf("0x%04X - AdcMode: 0x%X, AdcBusy:0x%X, AdcErr:0x%X, DevErr:0x%X\n", stat.value, stat.bits.AdcMode,
		stat.bits.AdcBusy, stat.bits.AdcErr, stat.bits.DevErr);

    // 2. DEV - AutoTimer stop (if was activated)
    // TODO

    // 3. DEV - Set AutoTimer period
    uint32_t num = 200000;
    for (int ch = 1; ch <= sram_total_channels(); ch++) {
	if (adc_num != 0 && ch != adc_num) continue;
	uart_printf("DEV    - Set timer-%d to %u\n", ch, num);
	SetADCAutoscanPeriod(ch, num);
    }

    // 4. SRAM - Currernt status
    GetSramStat(&sram_stat);
    if (cfg.debug_level > 0) uart_putc('\n');
    for (int ch = 1; ch <= sram_total_channels(); ch++) {
	if (adc_num != 0 && ch != adc_num) continue;
	uart_printf("SRAM-%d - Status: ", ch);
	print_sram_status(ch, &sram_stat);
    }

    // 5. SRAM - Set buffer length
    uint32_t nrecs = 500;
    if (cfg.debug_level > 0) uart_putc('\n');
    GetSramStat(&sram_stat);
    for (int ch = 1; ch <= sram_total_channels(); ch++) {
	if (adc_num != 0 && ch != adc_num) continue;

	ctrl = (CtrlMem_t){0};
	ctrl.Channel = channel_to_onehot(ch);
	ctrl.GvnLenBuf = nrecs;
	WriteSramCtrlReg(&ctrl);
	uart_printf("SRAM-%d - Set buffers length %d\n", ch, nrecs);
    }

    // 6. SRAM - Clear Bundle ready bits
    if (cfg.debug_level > 0) uart_putc('\n');
    GetSramStat(&sram_stat);
    for (int ch = 1; ch <= sram_total_channels(); ch++) {
	if (adc_num != 0 && ch != adc_num) continue;

	ctrl = (CtrlMem_t){0};
	ctrl.Channel = channel_to_onehot(ch);
	ctrl.GvnLenBuf = GetSramLen(ch, &sram_stat);
	ctrl.ClrRdy0 = 1;
	ctrl.ClrRdy1 = 1;
	WriteSramCtrlReg(&ctrl);
	uart_printf("SRAM-%d - Clear Bundle ready bits\n", ch);
    }

    // 7. SRAM - zeroing (TODO зачищать данные только по текущему АЦП - selectedADC)
    for (int ch = 1; ch <= sram_total_channels(); ch++) {
	if (adc_num != 0 && ch != adc_num) continue;
	uart_printf("SRAM-%d - Clear - addr: 0x%08x, %u bytes\n", ch, sram_mem(ch), sram_mem_size());
	memset((char *)sram_mem(ch), 0, sram_mem_size());
    }

    for (int ch = 1; ch <= sram_total_channels(); ch++) {
	if (adc_num != 0 && ch != adc_num) continue;
	if (!init_adc(ch)) return;
	uart_putc('\n');
    }
}

//---------------------------------------------------------
void clear_encoders()
{
    *Enc0LoadAdr = 0;
    *Enc1LoadAdr = 0;
    usleep(1000);
    uart_printf("Enc0:      %p => %d (0x%08X)\n", Enc0RegAdr, *Enc0RegAdr, *Enc0RegAdr);
    uart_printf("Enc1:      %p => %d (0x%08X)\n", Enc1RegAdr, *Enc1RegAdr, *Enc1RegAdr);
}

//---------------------------------------------------------
void read_encoders()
{
    uart_printf("Enc0:      %p => %d (0x%08X)\n", Enc0RegAdr, *Enc0RegAdr, *Enc0RegAdr);
    uart_printf("Enc1:      %p => %d (0x%08X)\n", Enc1RegAdr, *Enc1RegAdr, *Enc1RegAdr);

    uart_printf("\nEncs Debounce: %p => %u (0x%04X)\n", EncDebAdr, *EncDebAdr, *EncDebAdr);
#ifdef LINUX_VARIANT
    int freq = 100000000;
    double debfreq = (double)freq / *EncDebAdr;
    double debclock = 1.0 / debfreq;
    uart_printf("               freq: %0.2fHz, clock: %f sec\n", debfreq, debclock);
#endif
    uart_putc('\n');
}

//---------------------------------------------------------
void adc_nop()
{
    uart_printf("ADC-%d NOP ... ", selectedADC);
    bool ok = ADC_DirectCommand(selectedADC, CMDOP_NOP, new_arbitrary(), NULL);
    if (ok) uart_puts("OK\n");
}

//---------------------------------------------------------
void adc_reset() //    reset
{
    uint32_t valueLo = *AdcRegAdrLo;
    uint32_t valueHi = *AdcRegAdrHi; // to reset FPGA State Machine.
    uart_printf("ADC Data (L/H): %08X %08X\n", valueLo, valueHi);

    uart_printf("ADC-%d reset ... ", selectedADC);
    bool ok = ADC_DirectCommand(selectedADC, CMDOP_RESET, new_arbitrary(), NULL);
    if (ok) uart_puts("OK\n");
    set_arbitrary(0);
}

//---------------------------------------------------------
void adc_start()
{
    uart_printf("ADC-%d start ... ", selectedADC);
    bool ok = ADC_DirectCommand(selectedADC, CMDOP_START, new_arbitrary(), NULL);
    if (ok) uart_puts("OK\n");
}

//---------------------------------------------------------
void adc_stop()
{
    uart_printf("ADC-%d stop ... ", selectedADC);
    bool ok = ADC_DirectCommand(selectedADC, CMDOP_STOP, new_arbitrary(), NULL);
    if (ok) uart_puts("OK\n");
}

//---------------------------------------------------------
void adc_rdata()
{
    uart_printf("ADC-%d read conversion data\n", selectedADC);
    AdcResp_t resp;
    bool ok = ADC_DirectCommand(selectedADC, CMDOP_RDATA, new_arbitrary(), &resp);
    if (!ok) return;
    uint32_t value = (resp.bits.AdcMsbData << 16);
    value |= (resp.bits.AdcMidData << 8);
    value |= resp.bits.AdcLsbData;
    uart_printf("0x%02X\n", value);

    if (resp.bits.AdcStatus & 0x04) // new data from the last data read
	uart_puts("STATUS0::DRDY:1 (new data from the last data read)\n");
}

//---------------------------------------------------------
void adc_start_and_rdata()
{
    bool ok = ADC_DirectCommand(selectedADC, CMDOP_START, new_arbitrary(), NULL);
    if (!ok) return;
    AdcResp_t resp;
    ok = ADC_DirectCommand(selectedADC, CMDOP_RDATA, new_arbitrary(), &resp);
    if (!ok) return;
    int32_t value = (resp.bits.AdcMsbData << 16);
    value |= (resp.bits.AdcMidData << 8);
    value |= resp.bits.AdcLsbData;
    value <<= 8; // знак размножить
    value >>= 8;
    uart_printf("value: %08X (%d) - 0x%02X %02X %02X %02X (", value, value, resp.bits.AdcStatus, resp.bits.AdcMsbData,
		resp.bits.AdcMidData, resp.bits.AdcLsbData);
    print_status0(resp.bits.AdcStatus, 1);
    uart_puts(")");
}

//---------------------------------------------------------
void adc_read_register()
{
    if (autoscan_mode(selectedADC)) return;

    // 1.
    uart_puts("Read ADC Register (0h..12h) [0]: ");
    uint32_t reg_num;
    const int err = GetNumber(HEXMODE, &reg_num, 2);
    if (err) return;
    if (reg_num == GET_DEFAULT) {
	reg_num = 0;
	uart_printf("%u", reg_num);
    }
    uart_printf(" [%s]", reg_name(reg_num));
    uart_putc('\n');

    // 2.
    uint8_t value;
    if (!ADCReadRegister(selectedADC, reg_num, &value)) {
	return;
    }
    uart_printf("ADC-%d Reg: %02Xh - 0x%02X - ", selectedADC, reg_num, value);
    uart_puts(print_response_details(reg_num, value));
    uart_putc('\n');
}

//-----------------------------------------------
void adc_write_register()
{
    if (autoscan_mode(selectedADC)) return;

    // 1.
    uart_puts("Write ADC Register (0h..12h) [2]: ");
    uint32_t reg_num;
    int err = GetNumber(HEXMODE, &reg_num, 2);
    if (err) return;
    if (reg_num == GET_DEFAULT) {
	reg_num = 2;
	uart_printf("%u", reg_num);
    }
    uart_printf(" [%s]\n", reg_name(reg_num));

    uint8_t value;
    if (ADCReadRegister(selectedADC, reg_num, &value)) {
	uart_printf("Current value: 0x%02X - ", value);
	uart_puts(print_response_details(reg_num, value));
	uart_putc('\n');
    }

    // 2.
    uart_puts("Set value to write: 0x");
    uint32_t num;
    err = GetNumber(HEXMODE, &num, 2);
    if (err) return;
    uart_putc('\n');

    // 3.
    uart_putc('\n');
    value = (num & 0x00FF);
    if (!ADCWriteRegister(selectedADC, reg_num, value)) {
	return;
    }
}

//// //---------------------------------------------------------
//// void adc_read_id()
//// {
////     bool ok;
////     uart_printf("Read ADC ID (1..4) (0 - all) [%d]: ", selectedADC);
////     int adc_num;
////     int err = GetNumber(DECMODE, (uint32_t *)&adc_num, 1);
////     if (err) return;
////     if (adc_num == GET_DEFAULT) {
////         adc_num = selectedADC;
////         uart_printf("%u", adc_num);
////     }
////     uart_putc('\n');
////
////     uint8_t id[4];
////     if (adc_num == 0) {
////         for(int num = 1; num <= 4; num++) {
////             ok = ADCReadID(num, &id[num-1]);
////             if (!ok) return;
////         }
////
////         for(int i = 0; i < 4; i++) {
////             uart_printf("ADC-%d ID: 0x%02X ... ", adc_num, id[0]);
////             if ((id[0] & 0xF0) == 0x40) uart_puts("OK");
////             uart_putc('\n');
////         }
////         return;
////     }
////
////     ok = ADCReadID(adc_num, &id[0]);
////     if (!ok) return;
////
////     uart_printf("ADC-%d ID: 0x%02X ... ", adc_num, id[0]);
////     if ((id[0] & 0xF0) == 0x40) uart_puts("OK");
////     uart_putc('\n');
//// }

//---------------------------------------------------------
void adc_lists_registers()
{
    print_available_adc_registers();
}

//---------------------------------------------------------
void adc_read_regs()
{
    if (autoscan_mode(selectedADC)) return;

    uint8_t reg_num, value;
    uint8_t regs[] = {0, 1, 2, 3, 5, 6, 7, 8, 9, 10, 11, 12, 16, 17, 18};
    for (int i = 0; i < (int)sizeof(regs); i++) {
	reg_num = regs[i];
	uart_printf("ADC-%d RegNo %2Xh %-7s - ", selectedADC, reg_num, reg_name(reg_num));
	if (ADCReadRegister(selectedADC, reg_num, &value)) {
	    uart_printf("0x%02X %s\n", value, print_response_details(reg_num, value));
	} else {
	    uart_puts("FAILED");
	    break;
	}
    }
}

//---------------------------------------------------------
bool ADCReadRegister(int adc_num, uint8_t reg_num, uint8_t *value)
{
    if (!value) return FAIL;
    *value = 0;
    AdcResp_t adc_resp;
    bool ok = ADC_DirectCommand(adc_num, CMDOP_RREG + reg_num, new_arbitrary(), &adc_resp);
    if (!ok) return FAIL;
    *value = adc_resp.bits.AdcLsbData;
    return OK;
}

//---------------------------------------------------------
bool ADCWriteRegister(int adc_num, uint8_t reg_num, uint8_t value)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "(%s)", reg_name(reg_num));

    uart_printf("ADC-%d  - Reg: %02Xh %-7s - write 0x%02X\n", adc_num, reg_num, buf, value);

    bool ok = ADC_DirectCommand(adc_num, CMDOP_WREG + reg_num, value, NULL);
    if (!ok) return FAIL;

    // check
    uart_printf("ADC-%d  - Reg: %02Xh %-7s - read  ", adc_num, reg_num, buf);
    AdcResp_t adc_resp;
    ok = ADC_DirectCommand(adc_num, CMDOP_RREG + reg_num, new_arbitrary(), &adc_resp);
    if (!ok) {
	uart_printf(" - FAIL");
	return FAIL;
    }

    uint8_t val = adc_resp.bits.AdcLsbData;
    uart_printf("0x%02X (", val);
    uart_printf(print_response_details(reg_num, val));
    uart_puts(") - OK\n");
    return OK;
}

//---------------------------------------------------------
bool ADCReadID(int adc_num, uint8_t *id)
{
    // adc_num - 1..4
    AdcResp_t resp;
    bool ok = ADC_DirectCommand(adc_num, CMDOP_RREG + RegID, new_arbitrary(), &resp);
    if (!ok) return FAIL;
    if (id) *id = resp.bits.AdcLsbData;
    return OK;
}

//-----------------------------------------------
void enable_irq()
{
#ifdef INTERRUPT
    if (irq_enabled == NO) {
	gic_enable_interrupts();
	uart_puts("irq enabled");
    } else {
	gic_disable_interrupts();
	uart_puts("irq disabled");
    }
#endif
}

//-----------------------------------------------
/// @param adc_num 1..4
///
void AutoScan_control(const int adc_num)
{
    uint32_t num;
    DevStat_t stat = ReadDevStat();
    print_dev_status(&stat);
    uart_printf("ADC-%d Set autoscan On/Off (1/0) : ", adc_num);
    const int err = GetNumber(DECMODE, &num, 1);
    if (err) return;
    if (num == GET_DEFAULT) {
	num = 0;
	uart_printf("%u", num);
    }
    uart_putc('\n');
    if (num == 1) {
	AutoScan_Start(adc_num);
    } else {
	AutoScan_Stop(adc_num);
    }
}

//-----------------------------------------------
void AutoScan_control_all()
{
    uint32_t num;
    DevStat_t stat = ReadDevStat();
    print_dev_status(&stat);
    uart_printf("Set ADC autoscan On/Off (1/0) : ");
    const int err = GetNumber(DECMODE, &num, 1);
    if (err) return;
    if (num == GET_DEFAULT) {
	num = 0;
	uart_printf("%u", num);
    }
    uart_putc('\n');
    for (int i = 1; i <= 4; i++) {
	if (num == 1) {
	    AutoScan_Start(i);
	} else {
	    AutoScan_Stop(i);
	}
    }
}

//-----------------------------------------------
void print_firmware_version()
{
    uint32_t data = *FwVerRegAdr;
    if (cfg.debug_level > 0) {
	uart_printf("%p => 0x%08X\n", FwVerRegAdr, data);
    }
    uart_printf("FPGA Firmware version: %08X\n", data);
}

//-----------------------------------------------
void set_debug_level()
{
    uart_printf("Set debug level. Current value: %d\n", cfg.debug_level);
    uart_puts("Set new value (1..3) [1] : ");
    uint32_t num;
    int err = GetNumber(DECMODE, &num, 1);
    if (err) {
	WaitRxEnd();
	if (err == GET_CANCEL) return;
	if (num >= ' ' && num <= '~') {
	    uart_printf("\n'%c' - invalid value\n", num);
	    return;
	}
	uart_printf("\ninvalid value");
	return;
    }
    if (num == GET_DEFAULT) {
	num = 1;
	uart_printf("%u", num);
    }
    uart_putc('\n');
    cfg.debug_level = (int)num;
    uart_printf("debug level: %d\n", cfg.debug_level);
}
