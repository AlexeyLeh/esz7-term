/**
  \file     sram_t.h
  \author   Aleksey Lekhtin
  \date     Dec  3, 2025
  \brief    Структуры управления SRAM - Версия 3.0 03.12.2025
  \history  Версия 0     15.07.2024 - AK
            Версия 0.9   18.07.2024 - AVL
            Версия 0.9.1 15.08.2024 - AVL
            Версия 0.9.2 21.08.2024 - AVL
            Версия 0.9.3 22.08.2024 - AVL
            Версия 0.9.4 19.09.2024 - AVL
            Версия 1.0.0 04.08.2025 - AK
            Версия 3.0 02.12.2025   - AK
*/

#ifndef SRAM_T__H_
#define SRAM_T__H_

#define SRAM_MAX_RECS 512

#include "esz7_regs.h"

typedef struct CtrlMem_t
{
    uint32_t GvnLenBuf     : 9;     // 8..0     Given Len of Buffer - max 510(0x1fe) records(without 2 records of Encoders)
    uint32_t ClrRdy0       : 1;     // 9        Clear ready of Bungle0
    uint32_t ClrRdy1       : 1;     // 10       Clear ready of Bungle1
    uint32_t MemEn         : 1;     // 11       Enable write to memory
    uint32_t Channel       : 4;     // 15..12   Belonging to channel number
    uint32_t Reserv        : 16;    // 31..16   unused
} CtrlMem_t;

typedef struct StatMem_t {
    uint32_t Bundl0Rdy0    : 1;     //  0 Bungle0 of Buffers ready
    uint32_t Bundl1Rdy0    : 1;     //  1 Bungle1 of Buffers ready
    uint32_t MemEn0        : 1;     //  2 Enable write to memory
    uint32_t Bundl0Rdy1    : 1;     //  3 Bungle0 of Buffers ready
    uint32_t Bundl1Rdy1    : 1;     //  4 Bungle1 of Buffers ready
    uint32_t MemEn1        : 1;     //  5 Enable write to memory
    uint32_t Bundl0Rdy2    : 1;     //  6 Bungle0 of Buffers ready
    uint32_t Bundl1Rdy2    : 1;     //  7 Bungle1 of Buffers ready
    uint32_t MemEn2        : 1;     //  8 Enable write to memory
    uint32_t Bundl0Rdy3    : 1;     //  9 Bungle0 of Buffers ready
    uint32_t Bundl1Rdy3    : 1;     // 10 Bungle1 of Buffers ready
    uint32_t MemEn3        : 1;     // 11 Enable write to memory
    uint32_t Reserv        : 20;    // unused
} StatMem_t;

typedef struct LenMem_t {
	uint32_t GivenBufLenEven : 9;
	uint32_t GivenBufLenOdd  : 9;
    uint32_t Reserv          : 14;  // unused
} LenMem_t;

typedef union __attribute__((packed, aligned(4))) AdcRecStatus_t
{
    struct {
        uint32_t CMD : 8;
        uint32_t DAT : 8;
        uint32_t CRC : 8;
        uint32_t STATUS0: 8;
    } bits;
    uint32_t value;
} AdcRecStatus_t;

typedef union __attribute__((packed, aligned(4))) AdcRec_t
{
    struct bits {
        AdcRecStatus_t AdcStatus;   // Echo CMD/DATA/CRC, 7...0 - STATUS0
        uint32_t Value     : 24;    // ADC conversion obj
        uint32_t ValueCRC  : 8;     // Conversion obj CRC
    } bits;
    uint64_t value;
} AdcRec_t;

typedef struct Bundle_t
{
    AdcRec_t recs[SRAM_MAX_RECS];   // 512 x 8b = 4096b
} Bundle_t;

typedef struct sram_mem_t
{
    Bundle_t bundle[2];             // 4k x 2 = 8k
} sram_mem_t;

typedef struct sram_t
{
    int numChannels;
    int max_recs;
    int mem_size;
    sram_mem_t *mem[4];             // 8k x x4 = 32k
} sram_t;

#endif // SRAM_T__H_
