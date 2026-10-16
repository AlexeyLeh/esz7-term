/**
\file   sram.h
\author Aleksey Lekhtin
\date   April 5, 2024
\brief  ESZ7 SRAM API
*/

#ifndef SRAM__H_
#define SRAM__H_

#include "sram_t.h"

typedef struct __attribute__((packed, aligned(4))) SramStat_t {
	uint16_t  BufLen0;
	uint16_t  BufLen1;
	uint16_t  BufLen2;
	uint16_t  BufLen3;
	StatMem_t stat;
} SramStat_t;

#ifdef LINUX_VARIANT
extern volatile CtrlMem_t *CtrlMem;
extern volatile StatMem_t *StatMem;
extern volatile LenMem_t  *LenMem0;
extern volatile LenMem_t  *LenMem1;
#endif

sram_t *sram();
int sram_total_channels();
int sram_recs();
sram_mem_t *sram_mem(int channel);
int sram_mem_size();

void WriteSramCtrlReg(const CtrlMem_t *ctrl);
void ReadSramStatReg(StatMem_t *stat);
int  ReadSramLen(int channel);

void GetSramStat(SramStat_t *sram_stat);
int GetSramLen(int channel, const SramStat_t *sram_stat);
int GetSramMemEn(int channel, const SramStat_t *sram_stat);
int GetSramBundlReady(int channel, const SramStat_t *sram_stat);

void sram_check(int mode);
void print_sram_status(int channel, const SramStat_t *sram_stat);
void sram_dump();
void print_hex_dump(const char *ptr, uint32_t len);

#endif // SRAM__H_
