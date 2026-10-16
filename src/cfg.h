/**
  \file   cfg.h
  \author Aleksey Lekhtin
  \date   Apr 05, 2024
  \brief  Config variables
*/

#ifndef CONFIG__H_
#define CONFIG__H_

#include <stdbool.h>
#include <stdint.h>

#undef LINUX_VARIANT
#if defined __linux__ || defined WIN32
#define LINUX_VARIANT
#endif

#define OK true
#define FAIL false
#define DONE false
#define YES true
#define NO false
#define ON true
#define OFF false

typedef struct mem_cfg_t
{
    int  reg_mem_fd;
    void *reg_mem_map;
    int  sram_fd[4];
    void *sram_map[4];
} mem_cfg_t;

typedef struct cfg_t
{
    int debug_level;
    mem_cfg_t mem;
} cfg_t;

extern cfg_t cfg;

void help();

#ifdef LINUX_VARIANT
void usage(const char *appname);
bool parse_args(int ac, char *av[]);
#endif

#endif // CONFIG__H_
