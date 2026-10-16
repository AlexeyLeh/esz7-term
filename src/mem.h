/*!
  \file   mem.h
  \author Aleksey Lekhtin
  \date   11.04.2024
  \brief  Register memory mappin to user space of application
*/

#ifndef MEM__H_
#define MEM__H_

#include <stdbool.h>
#include <stdint.h>

bool map_registers();
bool unmap_registers();
bool map_chip_mem();
bool unmap_chip_mem();
bool setup_registers_memory();

#endif // MEM__H_
