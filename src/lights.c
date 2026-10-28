/**
\file   lights.c
\author Aleksey Lekhtin
\date   Oct  16, 2026
\brief  Check some lights features ob SOC
 */

#include "dev.h"
#include "funcs.h"
#include "uart.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define GPIO_ADDR 0xE000A000 // GPIO base address
#define GPIO_ADDR_SIZE 0x1000

static int gpio_base = -1;
static int gpio_pin = -1;
static mem_cfg_t gpio_cfg = {};

#define XGPIOPS_DIRM_OFFSET 0x00000204	// смещение DIRM Bank 0
#define XGPIOPS_OUTEN_OFFSET 0x00000208 // смещение OEN Bank 0
#define XGPIOPS_DATA_OFFSET 0x00000040
#define XGPIOPS_DATA_RO_OFFSET 0x00000060

#define SLCR_ADDR 0xF8000000
#define SLCR_ADDR_SIZE 0x1000
#define SLCR_MIO_13_REG 0x734	 // Смещение для MIO_PIN_13
#define SLCR_APER_CLK_CTRL 0x12C // Смещение регистра тактирования периферии APER

#define PTR32(base, offset) ((volatile uint32_t *)((uintptr_t)(base) + (offset)))
#define REG32(base, offset) (*(volatile uint32_t *)((uintptr_t)(base) + (offset)))

#define PIN13 13

//---------------------------------------------------------
typedef enum {
    NONE,
    EXPORT,
    UNEXPORT,
} GPIO_EXPORT;

//---------------------------------------------------------
static bool unmap_gpio()
{
#ifndef LINUX_VARIANT
    return true;
#endif
    bool ret = OK; // success assumed
    if (gpio_cfg.reg_mem_map) {
	if (munmap(gpio_cfg.reg_mem_map, GPIO_ADDR) == -1) {
	    if (cfg.debug_level > 0) {
		uart_printf("%s(): %p unmapping failed (%s)\n", __func__, gpio_cfg.reg_mem_map, strerror(errno));
	    }
	    ret = FAIL;
	}
    } else if (cfg.debug_level > 0) {
	uart_printf("%s(): %p unmapped ok\n", __func__, gpio_cfg.reg_mem_map);
    }

    if (gpio_cfg.reg_mem_fd > 0) {
	if (close(gpio_cfg.reg_mem_fd) == -1) {
	    uart_printf("%s(): fd:%d close error (%s)\n", __func__, gpio_cfg.reg_mem_fd, strerror(errno));
	    ret = FAIL;
	} else if (cfg.debug_level > 0) {
	    uart_printf("%s(): fd: %d closed ok\n", __func__, gpio_cfg.reg_mem_fd);
	}
    }
    gpio_cfg.reg_mem_map = NULL;
    gpio_cfg.reg_mem_fd = -1;
    return ret;
}

//---------------------------------------------------------
static bool map_gpio()
{
#ifndef LINUX_VARIANT
    gpio_cfg.reg_mem_map = GPIO_ADDR;
    return true;
#endif

    gpio_cfg.reg_mem_fd = -1;
    gpio_cfg.reg_mem_map = NULL;

    const int mem_fd = open("/dev/mem", O_RDWR);
    if (mem_fd == -1) {
	uart_puts("can't open /dev/mem\n");
	unmap_gpio();
	return FAIL;
    }

    void *mem_map = mmap(0, GPIO_ADDR_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, GPIO_ADDR);
    if (mem_map == MAP_FAILED) {
	uart_printf("Register address:  0x%02x  - mapping failed\n", GPIO_ADDR);
	unmap_gpio();
	return FAIL;
    }
    if (cfg.debug_level > 0) {
	uart_printf("Register address:  0x%02x  (mapped to %p, /dev/mem fd:%d)\n", GPIO_ADDR, mem_map, mem_fd);
    }

    gpio_cfg.reg_mem_fd = mem_fd;
    gpio_cfg.reg_mem_map = mem_map;
    return OK;
}

//---------------------------------------------------------
static void init_slcr()
{
    // У Zynq за каждый вывод MIO отвечает отдельный регистр управления
    // в блоке SLCR (System Level Control Registers).
    // Если в Vivado этот пин не был сконфигурирован как GPIO, то
    // при старте Linux он может остаться в режиме «по умолчанию»
    // или быть назначенным на другую периферию (например, SPI или UART).

    // System Level Control Registers
    const int slcr_fd = open("/dev/mem", O_RDWR | O_SYNC);
    void *slcr_base = mmap(0, SLCR_ADDR_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, slcr_fd, SLCR_ADDR);

    uart_printf("SLCR> [%p] unlock \n", PTR32(slcr_base, SLCR_APER_CLK_CTRL));

    // Снимаем защиту от записи с регистров SLCR (Xilinx требует разблокировки)
    // SLCR_UNLOCK_OFFSET = 0x008, Ключ разблокировки = 0xDF0D
    REG32(slcr_base, 0x008) = 0xDF0D;

    // Читаем текущую конфигурацию пина 13
    uint32_t pin_cfg = REG32(slcr_base, SLCR_MIO_13_REG);
    uart_printf("SLCR> MIO_13 before:   0x%08X\n", pin_cfg);
    pin_cfg = 0; // сбросить все в 0!

    // Переводим пин в режим GPIO на выход:
    // Нужно СБРОСИТЬ бит 0 (Tri-state / отключить высокое сопротивление) [1]
    // Настроить биты выбора функции (биты 5:1) в режим GPIO (обычно 0x00) [1]
    // Попробуем для начала просто сбросить тристейт и подтяжку, оставив GPIO:
    pin_cfg &= ~(1 << 0); // Сбрасываем TRI_ENABLE (включаем выход драйвера) [1]
    uart_printf("SLCR> clear tri-state: 0x%08X\n", pin_cfg);

    //// pin_cfg &= ~0x0000003E; // Обнуляем биты мультиплексора peripheral (выбираем GPIO) [1]
    //// // Если нужна сильная подтяжка к 3.3В, можно поставить бит 12 (PULLUP), но для выхода важнее убрать TRI_ENABLE

    pin_cfg &= ~(1 << 8);
    uart_printf("SLCR> set slow speed:  0x%08X\n", pin_cfg);

    // pin_cfg |= 0x0E00;
    pin_cfg |= 0x0600;
    uart_printf("SLCR> io type 3.3v:    0x%08X\n", pin_cfg);

    pin_cfg &= ~(1 << 12);
    uart_printf("SLCR> clear pullup:    0x%08X\n", pin_cfg);

    pin_cfg |= (1 << PIN13);
    uart_printf("SLCR> DisableRcvr:     0x%08X\n", pin_cfg);

    REG32(slcr_base, SLCR_MIO_13_REG) = pin_cfg;
    uart_printf("SLCR> MIO_13 after:    0x%08X\n", REG32(slcr_base, SLCR_MIO_13_REG));

    // GPIO_CPU_1XCLKACT
    // Читаем текущее состояние тактирования
    uint32_t clk_ctrl = REG32(slcr_base, SLCR_APER_CLK_CTRL);
    uart_printf("SLCR> [%p] APER_CLK_CTRL before: 0x%08X (bit 22: %u)\n", PTR32(slcr_base, SLCR_APER_CLK_CTRL),
		clk_ctrl, (clk_ctrl >> 22) & 0x01);
#if 1
    if (((clk_ctrl >> 22) & 0x01) == 0) {
	// 4. Включаем 22-й бит (GPIO AMBA Clock Control)
	// Маска 400000 в hex — это как раз (1 << 22)
	clk_ctrl |= (1 << 22);
	REG32(slcr_base, SLCR_APER_CLK_CTRL) = clk_ctrl;

	// Проверяем, что бит записался
	clk_ctrl = REG32(slcr_base, SLCR_APER_CLK_CTRL);
	uart_printf("SLCR> [%p] APER_CLK_CTRL after:  0x%08X (bit 22: %u)\n", PTR32(slcr_base, SLCR_APER_CLK_CTRL),
		    clk_ctrl, (clk_ctrl >> 22) & 0x01);
    }
#endif

    // 5. Закрываем защиту SLCR обратно (SLCR_LOCK_OFFSET = 0x004)
    uart_printf("SLCR> [%p] lock \n\n", PTR32(slcr_base, SLCR_APER_CLK_CTRL));
    REG32(slcr_base, 0x004) = 0x767B;

    munmap(slcr_base, SLCR_ADDR_SIZE);
    close(slcr_fd);
}

//---------------------------------------------------------
static void lights_help() // help
{
    uart_puts("\nSetup commands\n");
    uart_printf("   1 - turn mio13 led on/off (/sys/class/gpio/export)\n", GPIO_ADDR);
    uart_puts("   2 - turn mio13 led on/off (/sys/class/leds/)\n");
    uart_puts("   3 - turn heartbeat on/off (/sys/class/leds/)\n");
    uart_printf("   4 - turn mio13 led on/off (using GPIO 0x%08X)\n", GPIO_ADDR);
    uart_putc('\n');
    uart_putc('\n');
    uart_puts("(ESC - cancel input or back to Main menu)\n");
}

//---------------------------------------------------------
#define TARGET_LABEL "zynq_gpio"
#define MIO_PIN_OFFSET 13

static int looking_for_gpio_base()
{
    struct dirent *entry;
    char path[1024];
    char label[64];
    int base = -1;

    const char *gpio_system_name = "/sys/class/gpio";
    DIR *dir = opendir(gpio_system_name);
    if (!dir) {
	uart_printf("ERROR: Can't open %s\n", gpio_system_name);
	return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
	if (strncmp(entry->d_name, "gpiochip", 8) == 0) {
	    snprintf(path, sizeof(path), "%s/%s/label", gpio_system_name, entry->d_name);
	    FILE *fd = fopen(path, "r");
	    if (fd) {
		if (fscanf(fd, "%63s", label) == 1) {
		    if (strcmp(label, TARGET_LABEL) == 0) {
			fclose(fd);
			snprintf(path, sizeof(path), "%s/%s/base", gpio_system_name, entry->d_name);
			fd = fopen(path, "r");
			if (fd) {

			    // Читаем первую строку файла
			    char buf[64];
			    if (fgets(buf, sizeof(buf), fd) != NULL) {
				char *endptr;
				errno = 0;
				const long val = strtol(buf, &endptr, 10);
				if (endptr != buf && errno != ERANGE && val >= INT_MIN && val <= INT_MAX) {
				    base = (int)val;
				} else {
				    base = -1;
				    uart_printf("ERROR: Can't get gpio base address from %s\n", path);
				}
			    }
			    fclose(fd);
			    break;
			}
		    }
		}
		fclose(fd);
	    }
	}
    }
    closedir(dir);
    return base;
}

//---------------------------------------------------------
static bool export_gpio_led(GPIO_EXPORT mode)
{
    const int mio_num = PIN13;
    if (gpio_base == -1) {
	gpio_base = looking_for_gpio_base();
	if (gpio_base == -1) {
	    return false;
	}
    }
    gpio_pin = gpio_base + mio_num;

    if (mode == EXPORT) {
	uart_printf("gpio base pin number: %d, pin-%d, gpio: %d\n", gpio_base, PIN13, gpio_pin);
    }

    char gpio_pin_name[32];
    char gpio_pin_filename[128];
    snprintf(gpio_pin_name, sizeof(gpio_pin_name), "%d", gpio_pin);
    snprintf(gpio_pin_filename, sizeof(gpio_pin_filename), "/sys/class/gpio/gpio%s/direction", gpio_pin_name);
    ////uart_printf("gpio_pin_filename: %s\n", gpio_pin_filename);

    if (mode == EXPORT) {
	struct stat st;
	if (stat(gpio_pin_filename, &st) != 0) {
	    const char *filename = "/sys/class/gpio/export";
	    const int fd = open(filename, O_WRONLY);
	    if (fd < 0) {
		uart_printf("ERROR: can't open gpio subsystem (%s)\n", filename);
		return false;
	    }
	    write(fd, gpio_pin_name, strlen(gpio_pin_name));
	    close(fd);
	}
        if (stat(gpio_pin_filename, &st) != 0) {
            uart_printf("ERROR: gpio pin-%d filename not found (%s)\n", PIN13, gpio_pin_filename);
            return false;
        }
    } else if (mode == UNEXPORT) {
	const char *filename = "/sys/class/gpio/unexport";
	const int fd = open(filename, O_WRONLY);
	if (fd < 0) {
	    uart_printf("ERROR: can't open gpio subsystem (%s)\n", filename);
	    return false;
	}
	write(fd, gpio_pin_name, strlen(gpio_pin_name));
	close(fd);
    } else {
        uart_printf("ERROR: invalid exporting mode %d\n", (int)mode);
        return false;
    }

    if (mode == UNEXPORT) {
	return true;
    }

    // set direction
    const int fd = open(gpio_pin_filename, O_RDWR);
    if (fd < 0) {
	uart_printf("ERROR: can't open gpio pin direction\n");
	return false;
    }
    char buf[32] = {};
    if (read(fd, buf, sizeof(buf)) == -1) {
        uart_puts("ERROR: can't read current direction status\n");
        close(fd);
        return false;
    }

    char *p = strchr(buf, '\r');
    if (p) *p = '\0';
    p = strchr(buf, '\n');
    if (p) *p = '\0';
    if (strcasecmp(buf, "out") != 0) {
        write(fd, "out", 3);
    }
    close(fd);
    return true;
}

//---------------------------------------------------------
void check_lights() // Check light features
{
    if (!map_gpio()) {
	return;
    }

    lights_help();

    uint32_t cmd = 0;
    // cmd = '1';
    int work = 1;
    while (work == 1) {
	if (cmd == 0) { // for debug
	    while (1) {
		uart_puts("\n* lights cmd> ");
		cmd = GetSymb();
		if (cmd == 0x1b) {
		    work = 0;
		    break;
		}
		if (cmd != '\r' && cmd != '\n') break;
	    }
	    uart_puts("\n");
	}

#ifndef LINUX_VARIANT
	uart_puts("this features is unsupported in PS mode\n");
	continue;
#endif

	switch (cmd) {
	case 'h':
	    {
		lights_help();
		break;
	    }

	case '1':
	    {
		if (!export_gpio_led(EXPORT)) {
		    break;
		}

		char filename[64];
		snprintf(filename, sizeof(filename), "/sys/class/gpio/gpio%d/value", gpio_pin);
		const int fd = open(filename, O_RDWR);
		if (fd < 0) {
		    uart_printf("ERROR: can't open gpio subsystem (%s)\n", filename);
		    break;
		}

		char buf[8] = {};
		if (read(fd, buf, sizeof(buf)) == -1) {
		    uart_printf("ERROR: can't read pin-%d status(%s)\n", PIN13, filename);
		    break;
		}
		char *p = strchr(buf, '\r');
		if (p) *p = '\0';
		p = strchr(buf, '\n');
		if (p) *p = '\0';

		errno = 0;
		const int value = strtol(buf, NULL, 10);
		if (errno > 0) {
		    uart_printf("ERROR %d: strtol() - %s\n", errno, strerror(errno));
		    break;
		}
		if (value == 1) {
		    uart_printf("current status: %d -> turn OFF\n", value);
		    write(fd, "0\n", 2);
		} else {
		    uart_printf("current status: %d -> turn ON\n", value);
		    write(fd, "1\n", 2);
		}
		close(fd);

		export_gpio_led(UNEXPORT);
		break;
	    }

	case '2':
	    {
		const char led_mio13_brightness_file[] = "/sys/class/leds/xme0724_20_led_mio13/brightness";
		struct stat st;
		if (stat(led_mio13_brightness_file, &st) != 0) {
		    uart_puts("led control file not found - feature unreachable\n");
		    uart_printf("%s\n", led_mio13_brightness_file);
		    break;
		}
		int fd = open(led_mio13_brightness_file, O_RDWR);
		if (fd == -1) {
		    uart_printf("ERROR: can't open file %s\n", led_mio13_brightness_file);
		    break;
		}
		char buf[8] = {0};
		if (read(fd, buf, sizeof(buf)) == -1) {
		    uart_printf("ERROR: can't read file %s\n", led_mio13_brightness_file);
		    break;
		}
		const int status = strtol(buf, NULL, 10);
		if (status == 0) {
		    uart_puts("turn led ON\n");
		    write(fd, "1\n", 2);
		} else {
		    uart_puts("turn led OFF\n");
		    write(fd, "0\n", 2);
		}
		close(fd);
		break;
	    }

	case '3':
	    {
		const char led_mio13_trigger_file[] = "/sys/class/leds/xme0724_20_led_mio13/trigger";
		struct stat st;
		if (stat(led_mio13_trigger_file, &st) != 0) {
		    uart_puts("led control file not found - feature unreachable\n");
		    uart_printf("%s\n", led_mio13_trigger_file);
		    break;
		}
		int fd = open(led_mio13_trigger_file, O_RDWR);
		if (fd == -1) {
		    uart_printf("ERROR: can't open file %s\n", led_mio13_trigger_file);
		    break;
		}
		char buf[1024] = {0};
		if (read(fd, buf, sizeof(buf)) == -1) {
		    uart_printf("ERROR: can't read file %s\n", led_mio13_trigger_file);
		    break;
		}

		const char *status = strstr(buf, "[heartbeat]");
		if (!status) {
		    uart_printf("turn led heartbeat ON\n", status);
		    write(fd, "heartbeat\n", 10);
		} else {
		    uart_printf("turn led heartbeat OFF\n", status);
		    write(fd, "none\n", 5);
		}
		close(fd);
		break;
	    }

	case '4':
	    {
		init_slcr();

		uint32_t v;
		void *base = gpio_cfg.reg_mem_map;
		uart_printf("GPIO base addres: %p, MIO PIN %u\n", gpio_cfg.reg_mem_map, PIN13);

		// 1. Direction (DIRM)
		v = REG32(base, XGPIOPS_DIRM_OFFSET);
		uart_printf("[%p] direction:  0x%08X, ", PTR32(base, XGPIOPS_DIRM_OFFSET), v);
		v |= (1 << PIN13); // MIO-13 for output
		uart_printf("write 0x%X\n", v);
		REG32(base, XGPIOPS_DIRM_OFFSET) = v;

		// 2. Output enable (OUTEN)
		v = REG32(base, XGPIOPS_OUTEN_OFFSET);
		uart_printf("[%p] out enable: 0x%08X, ", PTR32(base, XGPIOPS_OUTEN_OFFSET), v);
		v |= (1 << PIN13);
		uart_printf("write 0x%X\n", v);
		REG32(base, XGPIOPS_OUTEN_OFFSET) = v;

		// 4. Read pin status (DATA_RO)
		v = REG32(base, XGPIOPS_DATA_RO_OFFSET);
		uart_printf("[%p] read pin:   0x%08X\n", PTR32(base, XGPIOPS_DATA_RO_OFFSET), v);

		// 4. Write 1 for pin (DATA)40)
		v |= (1 << PIN13);
		uart_printf("[%p] write pin:  0x%08X\n", PTR32(base, XGPIOPS_DATA_OFFSET), v);
		REG32(base, XGPIOPS_DATA_OFFSET) = v;

		// 5. Read pin status (DATA_RO)
		v = REG32(base, XGPIOPS_DATA_RO_OFFSET);
		uart_printf("[%p] read pin:   0x%08X\n", PTR32(base, XGPIOPS_DATA_RO_OFFSET), v);

		break;
	    }

	case 'q':
	    {
		work = 0;
		unmap_gpio();
		finish();
		exit(0);
	    }

	case 0x1B:
	    {
		break;
	    }

	default: // bad choice
	    {
		uart_puts("invalid command\n");
		break;
	    }
	} // switch

	cmd = 0;
    } // while ();
    unmap_gpio();
}
