/**
\file   interrupts.h
\author Aleksey Lekhtin
\date   Oct  3, 2025
\brief  General interrupts controller test
 */

#ifndef INTERRUPTS__H__
#define INTERRUPTS__H__

#include "cfg.h"

#ifndef LINUX_VARIANT
#define INTERRUPT
#endif

#ifdef INTERRUPT

extern bool irq_enabled;
extern uint32_t irq_counter;

bool gic_init();
bool gic_inited();
bool gic_irq_enable(bool status);

bool gic_enable_interrupts();
void gic_disable_interrupts();


void print_irq_regs(int mode);
void print_gic_regs();
void print_irq_active_bit();
void print_irq_pending_reg();

#endif

#endif //INTERRUPTS__H__
