/**
\file   sram.c
\author Aleksey Lekhtin
\date   April 5, 2024
\brief  ESZ7 SRAM API
 */

#include <stdint.h>

#include "dev.h"
#include "sram.h"
#include "uart.h"

volatile CtrlMem_t *CtrlMem = ((volatile CtrlMem_t *) (ESZ7_ADDR + SRAM_ADCDEV_ADDR));
volatile StatMem_t *StatMem = ((volatile StatMem_t *) (ESZ7_ADDR + SRAM_STATREG_ADDR));
volatile LenMem_t  *LenMem0 = ((volatile LenMem_t  *) (ESZ7_ADDR + SRAM_BUFLEN0_ADDR));
volatile LenMem_t  *LenMem1 = ((volatile LenMem_t  *) (ESZ7_ADDR + SRAM_BUFLEN1_ADDR));

static sram_t m_sram = {
    .numChannels = sizeof(m_sram.mem) / sizeof(m_sram.mem[0]),
    .max_recs = SRAM_MAX_RECS,
    .mem_size = sizeof(sram_mem_t),
    .mem[0] = (sram_mem_t *)SRAM_ADDR0,
    .mem[1] = (sram_mem_t *)SRAM_ADDR1,
    .mem[2] = (sram_mem_t *)SRAM_ADDR2,
    .mem[3] = (sram_mem_t *)SRAM_ADDR3
};

//---------------------------------------------------------
inline sram_t *sram()
{
    return &m_sram;
}

//---------------------------------------------------------
inline int sram_total_channels()
{
    return sizeof(m_sram.mem) / sizeof(m_sram.mem[0]);
}

//---------------------------------------------------------
inline int sram_recs()
{
    return m_sram.numChannels;
}

//---------------------------------------------------------
inline sram_mem_t *sram_mem(int channel)
{
    // channel - [1..4]
    int idx = channel - 1;
    return m_sram.mem[idx];
}

//---------------------------------------------------------
inline int sram_mem_size()
{
    return m_sram.mem_size;
}

//---------------------------------------------------------
inline void WriteSramCtrlReg(const CtrlMem_t *ctrl)
{
    *CtrlMem = *ctrl;
}

//---------------------------------------------------------
inline void ReadSramStatReg(StatMem_t *stat)
{
    *stat = *StatMem;
}

//---------------------------------------------------------
void GetSramStat(SramStat_t *sram_stat)
{
    sram_stat->stat = *StatMem;
    LenMem_t len = *LenMem0;
    sram_stat->BufLen0 = len.GivenBufLenEven;
    sram_stat->BufLen1 = len.GivenBufLenOdd;
    len = *LenMem1;
    sram_stat->BufLen2 = len.GivenBufLenEven;
    sram_stat->BufLen3 = len.GivenBufLenOdd;
}

//---------------------------------------------------------
int GetSramLen(int channel, const SramStat_t *sram_stat)
{
    if (channel == 1) {
	return sram_stat->BufLen0;
    }
    if (channel == 2) {
	return sram_stat->BufLen1;
    }
    if (channel == 3) {
	return sram_stat->BufLen2;
    }
    if (channel == 4) {
	return sram_stat->BufLen3;
    }
    return 0;
}

//---------------------------------------------------------
int GetSramMemEn(int channel, const SramStat_t *sram_stat)
{
    if (channel == 1) {
	return sram_stat->stat.MemEn0;
    }
    if (channel == 2) {
	return sram_stat->stat.MemEn1;
    }
    if (channel == 3) {
	return sram_stat->stat.MemEn2;
    }
    if (channel == 4) {
	return sram_stat->stat.MemEn3;
    }
    return 0;
}

//---------------------------------------------------------
int GetSramBundlReady(int channel, const SramStat_t *sram_stat)
{
    if (channel == 1) {
	if (sram_stat->stat.Bundl0Rdy0) return 0;
	if (sram_stat->stat.Bundl1Rdy0) return 1;
	return -1;
    }
    if (channel == 2) {
	if (sram_stat->stat.Bundl0Rdy1) return 0;
	if (sram_stat->stat.Bundl1Rdy1) return 1;
	return -1;
    }
    if (channel == 3) {
	if (sram_stat->stat.Bundl0Rdy2) return 0;
	if (sram_stat->stat.Bundl1Rdy2) return 1;
	return -1;
    }
    if (channel == 4) {
	if (sram_stat->stat.Bundl0Rdy3) return 0;
	if (sram_stat->stat.Bundl1Rdy3) return 1;
	return -1;
    }
    return -1;
}

//---------------------------------------------------------
int ReadSramLen(int channel)
{
    LenMem_t buflen;
    if (channel == 1) {
	    buflen = *LenMem0;
	    return buflen.GivenBufLenEven;
    }
    if (channel == 2) {
	    buflen = *LenMem0;
	    return buflen.GivenBufLenOdd;
    }
    if (channel == 3) {
	    buflen = *LenMem1;
	    return buflen.GivenBufLenEven;
    }
    if (channel == 4) {
	    buflen = *LenMem1;
	    return buflen.GivenBufLenOdd;
    }
    return 0;
}

//---------------------------------------------------------
void sram_check(int mode)
{
    if (autoscan_mode(selectedADC)) return;

    uint32_t fails = 0;
    uart_puts("-- SRAM check\n");

    //---------- побайтное тестирование
    uint8_t pattern_8bit[] = {
	0x00,
	0xFF,
	0x7F,
	0x77,
	0x44,
	0x33,
	0xCC,
	0x55,
	0x00,
	0xAA
    };

    int pattern_len = (int)(sizeof(pattern_8bit) / sizeof(pattern_8bit[0]));
    if (mode > 0) {
	uart_printf("\nByte Check (8-bit) - %d (0x%04X) bytes: \n",
		    sram_mem_size(), sram_mem_size());
    }

    //-------------------------------------------
    fails = 0;
    for (int idx = 0; idx < sram_total_channels(); idx++) {
	uart_printf("Channel: %d - ", idx+1);

	for (int i = 0; i < pattern_len; i++) {
	    uart_printf("%02X ", pattern_8bit[i]);

	    char *mem = (char *)m_sram.mem[idx];
	    int counter = sram_mem_size();
	    while(counter > 0) {
		*mem = pattern_8bit[i];
		if (*mem != pattern_8bit[i]) {
		    fails++;
		}
		mem++;
		counter--;
	    }

	    if (fails != 0) {
		uart_printf("[E]\n");
	    }
	}
	uart_putc('\n');
    }

    if (fails) {
	uart_printf("--- detected %u errors\n\n", fails);
	return;
    }

    if (mode == 0) {
	uart_puts("\n\n");
	return;
    }

    //---------- пословное тестирование (64 bit)
    uint64_t pattern_64bit[] = {
	0x0000000000000000,
	0xFFFFFFFFFFFFFFFF,
	0x7FFFFFFF7FFFFFFF,
	0x7777777777777777,
	0x4444444444444444,
	0x3333333333333333,
	0xCCCCCCCCCCCCCCCC,
	0x5555555555555555,
	0x0000000000000000,
	0xAAAAAAAAAAAAAAAA
    };

    pattern_len = (int)(sizeof(pattern_64bit) / sizeof(pattern_64bit[0]));
    int nWords = (int)sizeof(sram_t) / sizeof(uint64_t);
    if (mode > 0) {
	uart_printf("\nWord Check (64-bit) - %d (0x%04X) words: \n", nWords, nWords);
    }

    fails = 0;
    for (int idx = 0; idx < sram_total_channels(); idx++) {
	uart_printf("Channel: %d\n", idx+1);

	for (int i = 0; i < pattern_len; i++) {
	    uart_printf("%08X", (uint32_t)(pattern_64bit[i] >> 32) & 0x00FFFFFFFF);
	    uart_printf("%08X ", (uint32_t)(pattern_64bit[i] & 0x00FFFFFFFF));

	    uint64_t *mem = (uint64_t *)m_sram.mem[idx];
	    int counter = nWords;
	    while(counter > 0) {
		*mem = pattern_64bit[i];
		if (*mem != pattern_64bit[i]) {
		    fails++;
		}
		mem++;
		counter--;
	    }
	    usleep(10000);
	    if (i > 0 && ((i+1) % 5) == 0)
		uart_putc('\n');
	}
	uart_putc('\n');
    }

    if (fails) {
	uart_printf("detected %u errors\n\n", fails);
	return;
    }
    uart_putc('\n');

    //-------------------------------------------
    for (int idx = 0; idx < sram_total_channels(); idx++) {
	uart_printf("fill channel %d by %02Xh\n", idx+1, pattern_8bit[idx+1]);
	char *mem = (char *)m_sram.mem[idx];
	int counter = sram_mem_size();
	while(counter > 0) {
	    *mem = pattern_8bit[idx+1];
	    mem++;
	    counter--;
	}
    }

}

//---------------------------------------------------------
void sram_dump()
{
    if (autoscan_mode(selectedADC)) return;

    uart_puts("-- SRAM dump\n");

    // 1. Ask bundle number to display
    uart_printf("Bundle?  [0/1]    (default: %3d) : ", 0);
    uint8_t ch = GetSymb();
    if (ch != 0x1b) {
	uart_putc('\n');
	return;
    }

    int bundle;
    if (ch == '0') {
	    bundle = 0;
    } else if (ch == '1') {
	    bundle = 1;
    } else if (ch == '\r' || ch == '\n') {
	    bundle = 0;
	    uart_puts("[0]");
    } else {
	    uart_puts("\nincorrect number\n");
	    return;
    }
    uart_putc('\n');

    // 2. Ask number of records for display
#define NRECS_DEFAULT 16
    int recs = 0;
    uart_printf("Records? [1..%d] (default: %3d) : ", sram()->max_recs, NRECS_DEFAULT);
    int Err = GetNumber(DECMODE, (uint32_t*)&recs, 3);
    if (Err) return;
    if (recs == GET_DEFAULT) {
	recs = NRECS_DEFAULT;
	uart_printf("[%u]", recs);
    }
    if (recs > sram()->max_recs) {
	uart_printf("\nincorrect number - expected not more than %d\n", sram()->max_recs);
	return;
    }
    uart_putc('\n');

    // 4. go
    const char *mem = (const char *)sram_mem(selectedADC);
    uart_printf("\nSRAM-%d: 0x%08x - 0x%08x, %d bytes\n\n", selectedADC,
		mem, (mem + sram_mem_size()), sram_mem_size());

    uint32_t nbytes = recs * sizeof(uint64_t);

    if (bundle == 0) {
	uart_printf("------ CH:%d Bundle: 0 ------\n", selectedADC);
	print_hex_dump(mem, nbytes);
	uart_puts("\n\n");
    } else {
	uart_printf("------ CH:%d Bundle: 1 ------\n", selectedADC);
	print_hex_dump(mem + sizeof(Bundle_t), nbytes);
	uart_puts("\n\n");
    }
}

//-----------------------------------------------
void print_sram_status(int channel, const SramStat_t *sram_stat)
{
    // channel - [1..4]
    if (channel == 1) {
	uart_printf("Len:%d MemEn:%d B0Rdy:%d B1Rdy:%d",
		    sram_stat->BufLen0,
		    sram_stat->stat.MemEn0,
		    sram_stat->stat.Bundl0Rdy0,
		    sram_stat->stat.Bundl1Rdy0);
    } else if (channel == 2) {
	uart_printf("Len:%d MemEn:%d B0Rdy:%d B1Rdy:%d",
		    sram_stat->BufLen1,
		    sram_stat->stat.MemEn1,
		    sram_stat->stat.Bundl0Rdy1,
		    sram_stat->stat.Bundl1Rdy1);
    } else if (channel == 3) {
	uart_printf("Len:%d MemEn:%d B0Rdy:%d B1Rdy:%d",
		    sram_stat->BufLen2,
		    sram_stat->stat.MemEn2,
		    sram_stat->stat.Bundl0Rdy2,
		    sram_stat->stat.Bundl1Rdy2);
    } else if (channel == 4) {
	uart_printf("Len:%d MemEn:%d B0Rdy:%d B1Rdy:%d",
		    sram_stat->BufLen3,
		    sram_stat->stat.MemEn3,
		    sram_stat->stat.Bundl0Rdy3,
		    sram_stat->stat.Bundl1Rdy3);
    }
    uart_putc('\n');
}

//---------------------------------------------------------
void print_hex_dump(const char *ptr, uint32_t nbytes)
{
    uint32_t i;
    uart_printf("0   : ");
    for (i = 0; i < nbytes; i++) {
	if (i > 0) {
	    if ((i % 4) == 0) uart_putc(' ');
	    if ((i % 16) == 0) uart_printf("\n%-4d: ", i);
	}
	uart_printf("%02X ", *ptr);
	ptr++;
    }
    uart_printf("\n%-4d: \n", i);
}
