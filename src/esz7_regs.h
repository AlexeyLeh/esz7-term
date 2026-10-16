/**
 \file   regs.h
 \brief  Описание регистров ESZ7000 - Версия 3.0 02.12.2025
 \author LAV
 \date   3.11.2025
 \history Версия 0 06.03.2024 - AK
          Версия 1.1 18.07.2024 - LAV
          Версия 1.2 10.10.2024 - AK
          Версия 3.0 02.12.2025 - AK
 */

#ifndef REGS__H_
#define REGS__H_

#include <stdbool.h>
#include <stdint.h>

#define ESZ7_ADDR           0x43C00000
#define ESZ7_ADDR_SIZE      0x1000   // 4Kb

#define FMWREG_ADDR         0x00

#define DEVSTAT_ADDR        0x04
#define ADCREGL_ADDR        0x08
#define ADCREGH_ADDR        0x0C
#define DEVCTRL_ADDR        0x10

#define ENC0_ADDR           0x14
#define ENC1_ADDR           0x18
#define ENCDEB_ADDR         0x1C
#define ENCLOAD0_ADDR       0x20
#define ENCLOAD1_ADDR       0x24

#define SRAM_ADCDEV_ADDR    0x28
#define SRAM_STATREG_ADDR   0x2C
#define SRAM_BUFLEN0_ADDR   0x30
#define SRAM_BUFLEN1_ADDR   0x34

#define SRAM_ADDR0          0x40000000    // size: 0x1FFF
#define SRAM_ADDR1          0x42000000
#define SRAM_ADDR2          0x44000000
#define SRAM_ADDR3          0x46000000


#endif // REGS__H_
