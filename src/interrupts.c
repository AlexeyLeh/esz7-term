/**
\file   interrupts.c
\author Aleksey Lekhtin
\date   Oct  3, 2025
\brief  General interrupts controller test
 */

#ifdef INTERRUPT
#include "interrupts.h"
#include "dev.h"
#include "uart.h"

#include "xscugic.h"

//-----------------------------------------------
XScuGic gic;
static XScuGic_Config *giccfg = NULL;

bool irq_enabled = NO;
uint32_t irq_counter = 0;

uint32_t FIQ_ID[4] = {61U, 62U, 63U, 64U};

//-----------------------------------------------
static void FIQ_Handler(void *callback)
{
    // Interrupt Acknowledge Register ICCIAR
    uint32_t irq_register = XScuGic_CPUReadReg(&gic, XSCUGIC_INT_ACK_OFFSET);
    uint32_t CPU = irq_register & XSCUGIC_CPUID_MASK;
    uint32_t ID = irq_register & XSCUGIC_ACK_INTID_MASK;
    uart_printf("\n+++ %s(): INT Register: 0x%08X - Cpu: %u, IRQ: %03Xh (%u)\n", __func__, irq_register, CPU, ID, ID);

    print_irq_pending_reg();
    print_irq_active_bit();

    irq_counter++;
    uart_printf("+++ Total IRQ counter: %d\n", irq_counter);

    if (!WaitADCReady(selectedADC)) {
	uart_printf("\n+++ IRQ - ADC-%d not ready\n", selectedADC);
    }

#if 1
    static volatile uint32_t valueLo, valueHi;
    static volatile AdcResp_t resp;
    valueLo = *AdcRegAdrLo;
    valueHi = *AdcRegAdrHi;
    resp.value = ((uint64_t)valueHi << 32) | valueLo;
    int32_t value = (resp.bits.AdcMsbData << 16);
    value |= (resp.bits.AdcMidData << 8);
    value |= resp.bits.AdcLsbData;
    value <<= 8; // знак размножить
    value >>= 8;
    uart_printf("ADC Data (L/H): %08X %08X, ADcValue: 0x%08X (%d)\n", valueLo, valueHi, value, value);
#endif
    XScuGic_CPUWriteReg(&gic, XSCUGIC_EOI_OFFSET, irq_register);
}

//-----------------------------------------------
bool gic_enable_interrupts()
{
    XScuGic_Enable(&gic, FIQ_ID[0]);
    XScuGic_Enable(&gic, FIQ_ID[1]);
    XScuGic_Enable(&gic, FIQ_ID[2]);
    XScuGic_Enable(&gic, FIQ_ID[3]);
    irq_enabled = YES;
    uart_printf("\nIRQ enabled:  %d %d %d %d\n", FIQ_ID[0], FIQ_ID[1], FIQ_ID[2], FIQ_ID[3]);
    return OK;
}

//-----------------------------------------------
void gic_disable_interrupts()
{
    XScuGic_Disable(&gic, FIQ_ID[0]);
    XScuGic_Disable(&gic, FIQ_ID[1]);
    XScuGic_Disable(&gic, FIQ_ID[2]);
    XScuGic_Disable(&gic, FIQ_ID[3]);
    irq_enabled = NO;
    uart_printf("\nIRQ disabled: %d %d %d %d\n", FIQ_ID[0], FIQ_ID[1], FIQ_ID[2], FIQ_ID[3]);

    uart_printf("IRQ counter: %u\n", irq_counter);
    if (gic.UnhandledInterrupts > 0) {
	uart_printf("Unhandled Interrupts: %u\n", gic.UnhandledInterrupts);
    }
}

//-----------------------------------------------
bool gic_irq_enable(bool enable)
{
    bool currentStatus = irq_enabled;
    if (enable && !irq_enabled) {
	gic_enable_interrupts();
    } else if (!enable && irq_enabled) {
	gic_disable_interrupts();
    }
    return currentStatus;
}

//-----------------------------------------------
bool gic_inited()
{
    Xil_ExceptionHandler h;
    void *Data = NULL;
    Xil_GetExceptionRegisterHandler(XIL_EXCEPTION_ID_FIQ_INT, &h, &Data);
    if (Data) {
	return YES;
    }
    return NO;
}

//-----------------------------------------------
bool gic_init()
{
    u32 off, value;
    uart_printf("GIC init       ");

    if (gic_inited()) {
	uart_printf("already inited\n");
	return OK;
    }

    giccfg = XScuGic_LookupConfig(XPAR_SCUGIC_0_DEVICE_ID);
    if (!giccfg) {
	uart_puts("FAILED\n");
	return FAIL;
    }
    if (XScuGic_CfgInitialize(&gic, giccfg, 0 /*!!!*/) != XST_SUCCESS) {
	XScuGic_Stop(&gic);
	uart_puts("FAILED\n");
	return FAIL;
    }
    uart_puts("OK\n");

    uart_printf("GIC:           0x%08X\n"
		"GetCpuID():    %d\n"
		"GIC ID:        %d\n"
		"CPU Address:   0x%08X\n"
		"DistAddress:   0x%08X\n"
		"IntrHandler:   0x%08X (XScuGic_InterruptHandler)\n"
		"FIQ_Handler(): 0x%08X (user callback)\n"
		"\n",
		&gic, XScuGic_GetCpuID(), gic.Config->DeviceId, gic.Config->CpuBaseAddress, gic.Config->DistBaseAddress,
		XScuGic_InterruptHandler, FIQ_Handler);

    uart_puts("CPU Interface Control Reg\n");
    off = XSCUGIC_CONTROL_OFFSET; // CPU Interface Control Register
    value = XScuGic_CPUReadReg(&gic, off);
    u8 SBPR = (value & XSCUGIC_CNTR_SBPR_MASK ? 1 : 0);
    u8 FIQEn = (value & XSCUGIC_CNTR_FIQEN_MASK ? 1 : 0);
    u8 AckCtl = (value & XSCUGIC_CNTR_ACKCTL_MASK ? 1 : 0);
    u8 EnableNS = (value & XSCUGIC_CNTR_EN_NS_MASK ? 1 : 0);
    u8 EnableS = (value & XSCUGIC_CNTR_EN_S_MASK ? 1 : 0);
    uart_printf("ICCICR: 0x%08X: 0x%08X - SBPR:%d FIQEn:%d AckCtl:%d EnableNS:%d EnableS:%d\n",
		(giccfg->CpuBaseAddress + off), value, SBPR, FIQEn, AckCtl, EnableNS, EnableS);

    uart_puts("Set FIQEN\n");
    value |= XSCUGIC_CNTR_FIQEN_MASK;
    XScuGic_CPUWriteReg(&gic, off, value);
    value = XScuGic_CPUReadReg(&gic, off);
    SBPR = (value & XSCUGIC_CNTR_SBPR_MASK ? 1 : 0);
    FIQEn = (value & XSCUGIC_CNTR_FIQEN_MASK ? 1 : 0);
    AckCtl = (value & XSCUGIC_CNTR_ACKCTL_MASK ? 1 : 0);
    EnableNS = (value & XSCUGIC_CNTR_EN_NS_MASK ? 1 : 0);
    EnableS = (value & XSCUGIC_CNTR_EN_S_MASK ? 1 : 0);
    uart_printf("ICCICR: 0x%08X: 0x%08X - SBPR:%d FIQEn:%d AckCtl:%d EnableNS:%d EnableS:%d\n",
		(giccfg->CpuBaseAddress + off), value, SBPR, FIQEn, AckCtl, EnableNS, EnableS);
    uart_putc('\n');

    uart_printf("GIC self test     ... ");
    if (XScuGic_SelfTest(&gic) != XST_SUCCESS) {
	uart_puts("FAILED\n");
	return FAIL;
    }
    uart_puts("OK\n");

    Xil_ExceptionInit();
    uart_printf("Setup exception   ... ");
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_FIQ_INT, (Xil_ExceptionHandler)XScuGic_InterruptHandler, &gic);
    Xil_ExceptionEnableMask(XIL_EXCEPTION_FIQ);
    uart_puts("OK\n");

    for (int i = 0; i < 4; i++) {
	uart_printf("GIC connect to %u ... ", FIQ_ID[i]);
	if (XScuGic_Connect(&gic, FIQ_ID[i], (Xil_ExceptionHandler)FIQ_Handler, (void *)&gic) != XST_SUCCESS) {
	    uart_puts("FAILED\n");
	    return FAIL;
	}
	uart_printf("OK (callback: 0x%08X)\n", FIQ_Handler);
    }
    uart_putc('\n');

    XScuGic_SetPriorityTriggerType(&gic, FIQ_ID[0], 0, 3);
    XScuGic_SetPriorityTriggerType(&gic, FIQ_ID[1], 0, 3);
    XScuGic_SetPriorityTriggerType(&gic, FIQ_ID[2], 0, 3);
    XScuGic_SetPriorityTriggerType(&gic, FIQ_ID[3], 0, 3);

    uart_puts("--- configuration ---\n");
    print_gic_regs();

    uart_puts("--- regs ---\n");
    print_irq_regs(0);

    ////uart_puts("\n--- regs #final ---");
    ////print_irq_regs(1);

    ////enable_interrupts();
    gic_disable_interrupts();
    return OK;
}

//-----------------------------------------------
void print_irq_active_bit()
{
    u32 value0, value1, value2, off;

    // Active Bit register
    uart_printf("ICDABR0[0:2] ");

    off = XSCUGIC_ACTIVE_OFFSET;
    value0 = XScuGic_DistReadReg(&gic, off);
    uart_printf("0x%08X ", value0);

    off += 4;
    value1 = XScuGic_DistReadReg(&gic, off);
    uart_printf("0x%08X ", value1);

    off += 4;
    value2 = XScuGic_DistReadReg(&gic, off);
    uart_printf("0x%08X ", value2);

    if (value0 > 0 || value1 > 0 || value2 > 0) {
	uart_printf("- Active irq: ");
    }

    if (value0 > 0) {
	for (int n = 0; n < 32; n++) {
	    if ((value0 >> n) & 0x1) uart_printf("%d ", n);
	}
    }

    if (value1 > 0) {
	for (int n = 0; n < 32; n++) {
	    if ((value1 >> n) & 0x1) uart_printf("%d ", 32 + n);
	}
    }

    if (value2 > 0) {
	for (int n = 0; n < 32; n++) {
	    if ((value2 >> n) & 0x1) uart_printf("%d ", 64 + n);
	}
    }
    uart_putc('\n');
}

void print_irq_pending_reg()
{
    u32 value, off;
    u32 value1, value2, value3;

    // Interrupt Set-pending Register
    off = XSCUGIC_PENDING_SET_OFFSET;
    uart_printf("ICDISPR[0:3] ");
    for (int i = 0; i < 3; i++, off += 4) {
	value = XScuGic_DistReadReg(&gic, off);
	uart_printf("0x%08X ", value);
	if (i == 0) value1 = value;
	if (i == 1) value2 = value;
	if (i == 2) value3 = value;
    }
    if (value1 > 0) {
	for (int n = 0; n < 32; n++) {
	    if ((value1 >> n) & 0x1) uart_printf("%d ", n);
	}
    }
    if (value2 > 0) {
	for (int n = 0; n < 32; n++) {
	    if ((value2 >> n) & 0x1) uart_printf("%d ", 32 + n);
	}
    }
    if (value3 > 0) {
	for (int n = 0; n < 32; n++) {
	    if ((value3 >> n) & 0x1) uart_printf("%d ", 64 + n);
	}
    }

    uart_putc('\n');
}

//-----------------------------------------------
void print_irq_regs(int mode)
{
    print_irq_active_bit();
    print_irq_pending_reg();
}
#endif // INTERRUPT

//-----------------------------------------------
#ifdef INTERRUPT
void print_gic_regs()
{
    u32 value, off, n;
    Xil_ExceptionHandler H;
    void *D;

    for (int i = 0; i <= XIL_EXCEPTION_ID_LAST; i++) {
	Xil_GetExceptionRegisterHandler(i, &H, &D);
	if (H != (Xil_ExceptionHandler)XScuGic_InterruptHandler) {
	    continue;
	}
	uart_printf("Exception[%d]%s: Handler: 0x%08X, Data: 0x%08X \n", i,
		    (i == XIL_EXCEPTION_ID_FIQ_INT ? " (FIQ_INT)" : ""), H, D);
    }

    for (int i = 0; i < XSCUGIC_MAX_NUM_INTR_INPUTS; i++) {
	if (gic.Config->HandlerTable[i].Handler != (Xil_ExceptionHandler)FIQ_Handler) {
	    continue;
	}
	uart_printf("IRQ %d - Handler:0x%08X, Arg:0x%08X\n", i, gic.Config->HandlerTable[i].Handler,
		    gic.Config->HandlerTable[i].CallBackRef);
    }
    uart_putc('\n');

    for (int i = 61; i <= 64; i++) {
	off = XSCUGIC_SPI_TARGET_OFFSET_CALC(i);
	value = XScuGic_DistReadReg(&gic, off);
	uart_printf("ICDIPTR[%-2d] 0x%08X: 0x%08X ", i / 4, (giccfg->DistBaseAddress + off), value);
	if (i >= 61 && i <= 63) {
	    n = (value >> (i - 60) * 8) & 0x03;
	} else if (i == 64) {
	    n = value & 0x03;
	}
	uart_printf("- CPU for [%2d:%-2d] regs - #%d (%X): ", (i / 4 * 4) + 3, (i / 4 * 4), i, n);
	if (n == 0) uart_printf("no CPU ");
	if (n & 0x01) uart_printf("CPU0 ");
	if (n & 0x10) uart_printf("CPU1 ");
	uart_putc('\n');
    }

    for (int i = 61; i <= 64; i++) {
	off = XSCUGIC_INT_CFG_OFFSET_CALC(i);
	value = XScuGic_DistReadReg(&gic, off);
	uart_printf("ICDICFR[%-2d] 0x%08X: 0x%08X ", i / 4, (giccfg->DistBaseAddress + off), value);
	if (i >= 61 && i <= 63) {
	    n = (value >> (i - 48) * 2) & 0x03;
	} else if (i == 64) {
	    n = value & 0x03;
	}
	uart_printf("- Config for [%2d:%-2d]- #%d (%Xh): ", (i / 4 * 4) + 3, (i / 4 * 4), i, n);
	if (n == 1) uart_printf("high-level active ");
	if (n == 3) uart_printf("rising-edge ");
	uart_putc('\n');
    }

    // Interrupt Priority Mask Register
    off = XSCUGIC_CPU_PRIOR_OFFSET;
    value = XScuGic_CPUReadReg(&gic, off);
    uart_printf("ICCPMR      0x%08X: 0x%08X - CPU priority mask: 0x%X (%u)\n", (giccfg->DistBaseAddress + off), value,
		(value & 0xFF), (value & 0xFF));

    // Interrupt Priority Register
    for (int i = 61; i <= 64; i++) {
	off = XSCUGIC_PRIORITY_OFFSET_CALC(i);
	value = XScuGic_DistReadReg(&gic, off);
	uart_printf("ICDIPR[%-2d]  0x%08X: 0x%08X - "
		    "priority for [%2d:%-2d] regs - 0x%02X 0x%02X 0x%02X 0x%02X\n",
		    i / 4, (giccfg->DistBaseAddress + off), value, (i / 4 * 4) + 3, (i / 4 * 4), ((value >> 24) & 0xf8),
		    ((value >> 16) & 0xf8), ((value >> 8) & 0xf8), ((value >> 0) & 0xf8));
    }

#if 0
	// Set Interrupt Priority Register
	for (int i = 61; i <= 64; i++) {
		off = XSCUGIC_PRIORITY_OFFSET_CALC(i);
		value = XScuGic_DistReadReg(&gic, off);
		if (i >= 61 && i <= 63) {
			value |= (0xF8 << (i-60)*8);
		} else if (i == 64) {
			value |= 0xF8;
		}
		uart_printf("Set priority for #%d => 0x%08X\n", i, value);
		XScuGic_DistWriteReg(&gic, off, value);
	}

	for (int i = 61; i <= 64; i++) {
		off = XSCUGIC_PRIORITY_OFFSET_CALC(i);
		value = XScuGic_DistReadReg(&gic, off);
		uart_printf("ICDIPR[%-2d]  0x%08X: 0x%08X - "
				"priority for [%2d:%-2d] regs - 0x%02X 0x%02X 0x%02X 0x%02X\n",
				i/4, (giccfg->DistBaseAddress + off), value,
				(i/4*4)+3, (i/4*4),
				((value >> 24) & 0xf8),
				((value >> 16) & 0xf8),
				((value >>  8) & 0xf8),
				((value >>  0) & 0xf8));
	}
#endif

    uart_putc('\n');
}
#endif // INTERRUPT
