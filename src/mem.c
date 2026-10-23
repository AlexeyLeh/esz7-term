/*!
  \file   mem.c
  \author Aleksey Lekhtin
  \date   05.12.2025
  \brief  Register memory mapping to user space of application
 */

#include "cfg.h"

#ifdef LINUX_VARIANT

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "dev.h"
#include "mem.h"
#include "sram.h"
#include "uart.h"

/**
 * Отображение памяти регистров в пользовательское пространство программы
 * \return bool
 * \return OK - успешно выполнено
 * \return FAIL - ошибка
 */
bool map_registers()
{
    cfg.mem.reg_mem_fd = -1;
    cfg.mem.reg_mem_map = NULL;

    int mem_fd = open("/dev/mem", O_RDWR);
    if (mem_fd == -1) {
	    uart_puts("can't open /dev/mem\n");
	    unmap_registers();
	    return FAIL;
    }

    // cfg.reg_mem_map = mmap(0, ESZ7_ADDR, PROT_READ | PROT_WRITE, MAP_SHARED, cfg.mem_fd, BASE_ADDR);
    void *mem_map = mmap(0, ESZ7_ADDR_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, ESZ7_ADDR);
    if (mem_map == MAP_FAILED) {
	    uart_printf("Register address:  0x%02x  - mapping failed", ESZ7_ADDR);
	    unmap_registers();
    	return FAIL;
    }
    if (cfg.debug_level > 0) {
    	uart_printf("Register address:  0x%02x  (mapped to %p, /dev/mem fd:%d)\n", ESZ7_ADDR, mem_map, mem_fd);
    }

    cfg.mem.reg_mem_fd = mem_fd;
    cfg.mem.reg_mem_map = mem_map;
    return OK;
}

/**
 * Отменить отображение памяти регистров в пользовательское пространство программы
 * \return bool
 * \return OK - успешно выполнено
 * \return FAIL - ошибка
 */
bool unmap_registers()
{
    bool ret = OK; // success assumed
    if (cfg.mem.reg_mem_map) {
	if (munmap(cfg.mem.reg_mem_map, ESZ7_ADDR) == -1) {
	    uart_printf("%s(): %p unmapping failed (%s)\n", __func__, cfg.mem.reg_mem_map, strerror(errno));
	    ret = FAIL;
	}
	cfg.mem.reg_mem_map = NULL;
    }

    if (close(cfg.mem.reg_mem_fd) == -1) {
	uart_printf("%s(): fd:%d close error (%s)\n", __func__, cfg.mem.reg_mem_fd, strerror(errno));
	ret = FAIL;
    }
    cfg.mem.reg_mem_fd = -1;
    return ret;
}

/**
 * Отображение памяти RAM в пользовательское пространство программы
 * \return bool
 * \return OK - успешно выполнено
 * \return FAIL - ошибка
 */
bool map_chip_mem()
{
    memset(cfg.mem.sram_fd, -1, sizeof(cfg.mem.sram_fd));
    memset((void *)cfg.mem.sram_map, 0, sizeof(cfg.mem.sram_map));

    for (int ch = 1; ch <= sram()->numChannels; ch++) {
	int mem_fd = open("/dev/mem", O_RDWR);
	if (mem_fd == -1) {
	    uart_puts("can't open /dev/mem\n");
	    unmap_chip_mem();
	    return 0;
	}

	void *mem_map = mmap(0, sram_mem_size(), PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, (off_t)sram_mem(ch));
	if (mem_map == MAP_FAILED) {
	    uart_printf("SRAM-%d address:    0x%02x  - mapping failed", ch, (void *)sram_mem(ch));
	    unmap_chip_mem();
	    return 0;
	}
	if (cfg.debug_level > 0) {
	    uart_printf("SRAM-%d address:    0x%02x  (mapped to %p, /dev/mem fd:%d)\n", ch, (void *)sram_mem(ch),
			mem_map, mem_fd);
	}
	cfg.mem.sram_fd[ch-1] = mem_fd;
	cfg.mem.sram_map[ch-1] = mem_map;
    }
    return 1;
}

/**
 * Отменить отображение памяти RAM в пользовательское пространство программы
 * \return bool
 * \return OK - успешно выполнено
 * \return FAIL - ошибка
 */
bool unmap_chip_mem()
{
    int ret = OK; // success assumed
    for (int i = 0; i < sram()->numChannels; i++) {
	if (cfg.mem.sram_map[i]) {
	    if (munmap(cfg.mem.sram_map[i], sram_mem_size()) == -1) {
		uart_printf("%s(): %p unmapping failed (%s)\n", __func__, cfg.mem.sram_map[i], strerror(errno));
		ret = FAIL;
	    }
	    cfg.mem.sram_map[i] = NULL;
	}

	if (close(cfg.mem.sram_fd[i]) == -1) {
	    uart_printf("%s(): fd:%d close error (%s)\n", __func__, cfg.mem.sram_fd[i], strerror(errno));
	    ret = FAIL;
	}
	cfg.mem.sram_fd[i] = -1;
    }
    return ret;
}

bool setup_registers_memory()
{
    if (!map_registers()) {
	return FAIL;
    }
    if (!map_chip_mem()) {
	return FAIL;
    }

    // dev register memory
    FwVerRegAdr = (uint32_t *)(cfg.mem.reg_mem_map + FMWREG_ADDR);
    DevCtrlAdr  = (DevCmd_t *)(cfg.mem.reg_mem_map + DEVCTRL_ADDR);
    DevStatAdr  = (DevStat_t*)(cfg.mem.reg_mem_map + DEVSTAT_ADDR);
    AdcRegAdrLo = (uint32_t *)(cfg.mem.reg_mem_map + ADCREGL_ADDR);
    AdcRegAdrHi = (uint32_t *)(cfg.mem.reg_mem_map + ADCREGH_ADDR);

    CtrlMem     = ((volatile CtrlMem_t *) (cfg.mem.reg_mem_map + SRAM_ADCDEV_ADDR));
    StatMem     = ((volatile StatMem_t *) (cfg.mem.reg_mem_map + SRAM_STATREG_ADDR));
    LenMem0     = ((volatile LenMem_t  *) (cfg.mem.reg_mem_map + SRAM_BUFLEN0_ADDR));
    LenMem1     = ((volatile LenMem_t  *) (cfg.mem.reg_mem_map + SRAM_BUFLEN1_ADDR));

    Enc0RegAdr  = (volatile int32_t *)(cfg.mem.reg_mem_map + ENC0_ADDR);
    Enc1RegAdr  = (volatile int32_t *)(cfg.mem.reg_mem_map + ENC1_ADDR);
    Enc0LoadAdr = (volatile int32_t *)(cfg.mem.reg_mem_map + ENCLOAD0_ADDR);
    Enc1LoadAdr = (volatile int32_t *)(cfg.mem.reg_mem_map + ENCLOAD1_ADDR);
    EncDebAdr   = (volatile uint32_t*)(cfg.mem.reg_mem_map + ENCDEB_ADDR);

    sram()->mem[0] = (sram_mem_t *)((uint32_t)cfg.mem.sram_map[0]);
    sram()->mem[1] = (sram_mem_t *)((uint32_t)cfg.mem.sram_map[1]);
    sram()->mem[2] = (sram_mem_t *)((uint32_t)cfg.mem.sram_map[2]);
    sram()->mem[3] = (sram_mem_t *)((uint32_t)cfg.mem.sram_map[3]);

    if (cfg.debug_level > 0) {
    uart_printf("Registers:\n");
    uart_printf("  Firmware Vers:     %p (0x%02X)\n", FwVerRegAdr, FMWREG_ADDR);
    uart_printf("  Device status:     %p (0x%02X)\n", DevStatAdr,  DEVSTAT_ADDR);
    uart_printf("  ADC register LO:   %p (0x%02X)\n", AdcRegAdrLo, ADCREGL_ADDR);
    uart_printf("  ADC register Hi:   %p (0x%02X)\n", AdcRegAdrHi, ADCREGH_ADDR);
    uart_printf("  Device control:    %p (0x%02X)\n", DevCtrlAdr,  DEVCTRL_ADDR);

    uart_printf("  Enc0RegAdr:        %p (0x%02X)\n", Enc0RegAdr,  ENC0_ADDR);
    uart_printf("  Enc1RegAdr:        %p (0x%02X)\n", Enc1RegAdr,  ENC1_ADDR);
    uart_printf("  EncDebLoadAdr:     %p (0x%02X)\n", EncDebAdr,   ENCDEB_ADDR);
    uart_printf("  Enc1LoadAdr:       %p (0x%02X)\n", Enc1LoadAdr, ENCLOAD1_ADDR);
    uart_printf("  Enc0LoadAdr:       %p (0x%02X)\n", Enc0LoadAdr, ENCLOAD0_ADDR);

    uart_printf("  SRAM Control:      %p (0x%02X)\n", CtrlMem, SRAM_ADCDEV_ADDR);
    uart_printf("  SRAM Status:       %p (0x%02X)\n", CtrlMem, SRAM_STATREG_ADDR);
    uart_printf("  SRAM MemLen0:      %p (0x%02X)\n", LenMem0, SRAM_BUFLEN0_ADDR);
    uart_printf("  SRAM MemLen1:      %p (0x%02X)\n", LenMem1, SRAM_BUFLEN1_ADDR);

    for (int i = 0; i < sram()->numChannels; i++) {
	    uart_printf("  SRAM-%d memory:     %p         \n", i, (void *)sram()->mem[i]);
    }
	uart_putc('\n');
    }
    return OK;
}
#else

bool setup_registers_memory()
{
    return OK;
}

#endif
