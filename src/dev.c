/**
 \file   dev.c
 \author Aleksey Lekhtin
 \date   April 5, 2024
 \brief  ESZ7 DEV API
 */

#include <stdio.h>

#include "cfg.h"
#include "crc.h"
#include "dev.h"
#include "esz7_regs.h"
#include "uart.h"

volatile uint32_t  *FwVerRegAdr = ((volatile uint32_t *) (ESZ7_ADDR + FMWREG_ADDR));
volatile DevStat_t *DevStatAdr  = ((volatile DevStat_t *)(ESZ7_ADDR + DEVSTAT_ADDR));
volatile DevCmd_t  *DevCtrlAdr = ((volatile DevCmd_t *) (ESZ7_ADDR + DEVCTRL_ADDR));
volatile uint32_t  *AdcRegAdrLo = ((volatile uint32_t *) (ESZ7_ADDR + ADCREGL_ADDR));
volatile uint32_t  *AdcRegAdrHi = ((volatile uint32_t *) (ESZ7_ADDR + ADCREGH_ADDR));

volatile int32_t   *Enc0RegAdr  = ((volatile int32_t *)  (ESZ7_ADDR + ENC0_ADDR));
volatile int32_t   *Enc1RegAdr  = ((volatile int32_t *)  (ESZ7_ADDR + ENC1_ADDR));
volatile int32_t   *Enc0LoadAdr = ((volatile int32_t *)  (ESZ7_ADDR + ENCLOAD0_ADDR));
volatile int32_t   *Enc1LoadAdr = ((volatile int32_t *)  (ESZ7_ADDR + ENCLOAD1_ADDR));
volatile uint32_t  *EncDebAdr   = ((volatile uint32_t *) (ESZ7_ADDR + ENCDEB_ADDR));

static uint8_t m_arbitrary = 0;

//---------------------------------------------------------
void set_arbitrary(uint8_t num) {
    m_arbitrary = num;
}

//---------------------------------------------------------
uint8_t new_arbitrary() {
    m_arbitrary++;
    if (m_arbitrary == 0) {
	m_arbitrary = 1;
    }
    return m_arbitrary;
}

//---------------------------------------------------------
bool WaitADCReady(int adc_num)
{
    static int busy;
    static int counter;
    static DevCmd_t cmd;
    static DevStat_t stat;

    counter = 0;
    cmd.value = 0;
    cmd.bits.CmdOpCode = CmdReadDevStat;
    cmd.bits.AdcChannels = channel_to_onehot(adc_num);
    cmd.bits.CmdParam = 0;
    do {
	counter++;
	if (counter >= 500) {
	    return FAIL;
	}
	*DevCtrlAdr = cmd;
	stat = *DevStatAdr;
	uint32_t mask = (1 << (adc_num - 1));
	busy = (int)(stat.bits.AdcBusy & mask);
    } while (busy);
    return OK;
}

//---------------------------------------------------------
bool ADC_DirectCommand(const int adc_num, const uint8_t cmdop, const uint8_t value, AdcResp_t *adc_resp)
{
    // adc_num - 1..4
    if (cfg.debug_level > 0) uart_putc('\n');

    if (adc_resp) {
        *adc_resp = (AdcResp_t ) { 0 };
    }

#if 1
    if (autoscan_mode(adc_num)) {
	uart_puts("WARNING! - Auto mode is enabled. Operations is not allowed.\n");
	return FAIL;
	//if (cmdop != CMDOP_RESET) {
	//    if (autoscan_mode(adc_num))
	//        return FAIL;
	//}
    }
#endif

    if (adc_num <= 0 || adc_num > 4) {
	uart_printf("\nERROR: %d is invalid ADC number (1..4)\n", adc_num);
	return FAIL;
    }

    uint8_t reg_num = 0;
    if (cmdop & CMDOP_RREG) {
	reg_num = cmdop - CMDOP_RREG;
    } else if (cmdop & CMDOP_WREG) {
	reg_num = cmdop - CMDOP_WREG;
    }

    if ((cmdop & CMDOP_RREG) || (cmdop & CMDOP_WREG)) {
	if (reg_num > 0x12) {
	    uart_puts("\nERROR: Invalid number - allowed numbers is 0h..12h");
	    return FAIL;
	}
    }

    // 1.
    uint8_t param[3];
    param[0] = cmdop;
    param[1] = value;
    param[2] = CalcCRC(param, 2);

    const uint8_t CMD = param[0];
    const uint8_t VAL = param[1];
    const uint8_t CRC = param[2];

    DevCmd_t cmd;
    cmd.value = 0;
    cmd.bits.CmdOpCode = CmdSetDirectCommand;
    cmd.bits.CmdParam = param[0] | (param[1] << 8) | (param[2] << 16);
    cmd.bits.AdcChannels = channel_to_onehot(adc_num);
    if (cfg.debug_level > 0) {
	uart_printf("DevCmd::%p <= 0x%08X (%s:%u, ch:%u, cmd: 0x%02X (%s), data: 0x%02X, crc: 0x%02X)\n",
		    DevCtrlAdr, cmd.value, CmdOpCodeName(cmd.bits.CmdOpCode), cmd.bits.CmdOpCode,
		    cmd.bits.AdcChannels, (cmd.bits.CmdParam >> 0) & 0x00FF,
		    cmd_name(CMD), (cmd.bits.CmdParam >> 8) & 0x00FF,
		    (cmd.bits.CmdParam >> 16) & 0x00FF);
    }

    *DevCtrlAdr = cmd;

    // 2. wait ADC ready
    bool adc_ready = WaitADCReady(adc_num);
#if 0
    if (irq_enabled) {
        // забрать результат в обработчике прерывания
        uart_puts("see data in irq hadler...");
        return OK;
    }
#endif

    // 3. get response
    // Всегда вычитывать ответ !!!
    // Response must be read always !!!
    static AdcResp_t resp;
    const uint32_t valueLo = *AdcRegAdrLo;
    const uint32_t valueHi = *AdcRegAdrHi;
    resp.value = ((uint64_t) valueHi << 32) | valueLo;

    if (!adc_ready) {
	if (adc_resp) {
	    *adc_resp = resp;
	}
	uart_printf("\nERROR: ADC-%d not ready\n", adc_num);
	return FAIL;
    }

    // 4. check for errors
    bool err = NO;
    if (resp.bits.EchoCmd != CMD)  err = YES;
    if (resp.bits.EchoData != VAL) err = YES;
    if (resp.bits.EchoCrc != CRC)  err = YES;
    if (resp.bits.AdcStatus & 0x40) err = YES; // CRC1 Error
    if (resp.bits.AdcStatus & 0x10) err = YES; // STAT12 Error Flag

    if (err && cfg.debug_level == 0) {
	uart_putc('\n');
	uart_printf("DevCmd::%p <= 0x%08X (%s:%u, ch:%u, cmd: 0x%02X (%s), data: 0x%02X, crc: 0x%02X)\n",
		    DevCtrlAdr, cmd.value, CmdOpCodeName(cmd.bits.CmdOpCode), cmd.bits.CmdOpCode,
		    cmd.bits.AdcChannels, (cmd.bits.CmdParam >> 0) & 0x00FF,
		    cmd_name(CMD), (cmd.bits.CmdParam >> 8) & 0x00FF,
		    (cmd.bits.CmdParam >> 16) & 0x00FF);
    }

    // 5. print errors
    if (err) {
	print_AdcResp(valueHi, valueLo);

	bool echo_error = NO;
	if (resp.bits.EchoCmd != CMD) {
	    echo_error = YES;
	    uart_printf("ERROR: Echo CMD  mismatch (0x%02X != 0x%02X)\n",
			resp.bits.EchoCmd, CMD);
	}
	if (resp.bits.EchoData != VAL) {
	    echo_error = YES;
	    uart_printf("ERROR: Echo DATA mismatch (0x%02X != 0x%02X)\n",
			resp.bits.EchoData, VAL);
	}
	if (resp.bits.EchoCrc != CRC) {
	    echo_error = YES;
	    uart_printf("ERROR: Echo CRC  mismatch (0x%02X != 0x%02X)\n",
			resp.bits.EchoCrc, CRC);
	    if (resp.bits.EchoCrc == (uint8_t)~CRC) {
		uint8_t crc = CalcCRC(param, 2);
		uart_printf("       error in input CRC (0x%02X)\n",
			    resp.bits.EchoCrc);
		uart_printf("(expected 0x%02X)\n", resp.bits.EchoCrc, crc);
	    }
	}

	if (!echo_error) {
	    print_AdcStatus(adc_num, resp.bits.AdcStatus);
	}

	uart_putc('\n');
	return FAIL;
    }

    // 6. check response data CRC
    uint8_t res[4] = { 0 };
    uint8_t crc;

    if (cmdop == CMDOP_RDATA) {
	res[0] = resp.bits.AdcStatus;
	res[1] = resp.bits.AdcMsbData;
	res[2] = resp.bits.AdcMidData;
	res[3] = resp.bits.AdcLsbData;
	crc = CalcCRC(res, 4);
	if (crc != resp.bits.AdcCrcResp) {
	    err = YES;
	    uart_printf("\nVerification CRC for '%02X %02X %02X %02X' => 0x%02X\n",
			res[0], res[1], res[2], res[3], crc);
	    uart_printf("ERROR: CRC mismatch (0x%02X != 0x%02X)\n", crc,
			resp.bits.AdcCrcResp);
	}

	if (err || cfg.debug_level > 0) {
	    print_AdcResp(valueHi, valueLo);
	}
    } else if (cmdop & CMDOP_RREG) {
	res[0] = resp.bits.AdcLsbData;
	crc = CalcCRC(res, 1);
	if (crc != resp.bits.AdcCrcResp) {
	    print_AdcResp(valueHi, valueLo);
	    err = YES;
	    uart_printf("\nVerification CRC for 'Lsb: %02X' => 0x%02X\n",
			res[0], crc);
	    uart_printf("ERROR: CRC mismatch (0x%02X != Out CRC:0x%02X)\n",
			crc, resp.bits.AdcCrcResp);
	}
    } else if (cmdop & CMDOP_WREG) {
	if (resp.bits.EchoCrc != CRC) {
	    print_AdcResp(valueHi, valueLo);
	    err = YES;
	    uart_printf("\nERROR: CRC mismatch (0x%02X != 0x%02X)\n",
			resp.bits.EchoData, CRC);
	}
    } else {
	if (cfg.debug_level > 0) {
	    print_AdcResp(valueHi, valueLo);
	}
    }

    if (err) {
	uart_putc('\n');
	return FAIL;
    }

    // 7. Return response
    if (adc_resp) {
	*adc_resp = resp;
    }

    //if (cfg.debug_level > 0) uart_putc('\n');
    return OK;
}

//---------------------------------------------------------
DevStat_t ReadDevStat()
{
    DevCmd_t cmd;
    cmd.value = 0;
    cmd.bits.CmdOpCode = CmdReadDevStat;
    cmd.bits.AdcChannels = channel_to_onehot(selectedADC);
    cmd.bits.CmdParam = 0;
    if (cfg.debug_level >= 2) {
	uart_printf("\nget stat: DevCmd:: %p <= 0x%08X\n", DevCtrlAdr, cmd.value);
    }
    *DevCtrlAdr = cmd;

    DevStat_t stat = *DevStatAdr;
    if (cfg.debug_level >= 2) {
	uart_printf("get stat: DevStat::%p => 0x%08X\n", DevCtrlAdr, stat.value);
    }
    return stat;
}

//---------------------------------------------------------
void DevReadADCOutputRegs()
{
    volatile uint32_t valueLo = *AdcRegAdrLo;
    volatile uint32_t valueHi = *AdcRegAdrHi; // this reset FPGA State Machine.
    uart_printf("ADC Data (L/H): %08X %08X\n", valueLo, valueHi);
}

//---------------------------------------------------------
void SetADCAutoscanPeriod(int adc_num, uint32_t ticks)
{
    // adc_num - 1..4
    DevCmd_t cmd;
    cmd.value = 0;
    cmd.bits.CmdOpCode = CmdSetAdcTmr;
    cmd.bits.AdcChannels = channel_to_onehot(adc_num);
    cmd.bits.CmdParam = ticks;
    if (cfg.debug_level > 0) {
	uart_printf("%p <= 0x%08X\n", DevCtrlAdr, cmd.value);
    }
    *DevCtrlAdr = cmd;
}

//---------------------------------------------------------
bool AutoScan_Start(int adc_num)
{
    // adc_num - 1..4
    if (adc_num < 1 || adc_num > 4) {
	uart_printf("\nERROR: %d is invalid ADC number (1..4)\n", adc_num);
	return FAIL;
    }

    DevCmd_t cmd;
    cmd.value = 0;
    cmd.bits.CmdOpCode = CmdSetAdcWorkMode;
    cmd.bits.AdcChannels = channel_to_onehot(adc_num);
    cmd.bits.CmdParam = channel_to_onehot(adc_num);

    if (cfg.debug_level > 0) {
	uart_printf("DevCmd::%p <= 0x%08X\n", DevCtrlAdr, cmd.value);
    }

    *DevCtrlAdr = cmd;
    return OK;
}

//---------------------------------------------------------
bool AutoScan_Stop(int adc_num)
{
    // adc_num - 1..4
    if (adc_num < 1 || adc_num > 4) {
	uart_printf("\nERROR: %d is invalid ADC number (1..4)\n", adc_num);
	return FAIL;
    }

    DevStat_t stat = ReadDevStat();

    DevCmd_t cmd;
    cmd.value = 0;
    cmd.bits.CmdOpCode = CmdSetAdcWorkMode;
    cmd.bits.AdcChannels = channel_to_onehot(adc_num);
    cmd.bits.CmdParam = stat.bits.AdcMode & (~channel_to_onehot(adc_num));

    if (cfg.debug_level > 0) {
	uart_printf("DevCmd::%p <= 0x%08X\n", DevCtrlAdr, cmd.value);
    }

    *DevCtrlAdr = cmd;
    if (!WaitADCReady(adc_num)) {
	uart_printf("\nINFO: ADC-%d not ready\n", adc_num);
    }
    uint32_t valueLo = *AdcRegAdrLo;
    uint32_t valueHi = *AdcRegAdrHi; // to reset FPGA State Machine.
    uart_printf("ADC-%d Data (L/H): %08X %08X\n", adc_num, valueLo, valueHi);
    return OK;
}

//---------------------------------------------------------
/**
 * Converts channel number to hex-representation for device
 * @param channel - channel number (1..4)
 * @return hex-representation
 * @return (001 010 011 100)
 */
uint8_t channel_to_onehot(int channel)
{
    // channel - 1..4
    if (channel < 1 || channel > 4)
	channel = 1;
    return (1 << (channel - 1));
}

//---------------------------------------------------------
bool autoscan_mode(int adc_num) {
    // adc_num - 1..4
    DevStat_t stat;
    DevCmd_t cmd;
    cmd.value = 0;
    cmd.bits.CmdOpCode = CmdReadDevStat;
    cmd.bits.AdcChannels = channel_to_onehot(adc_num);
    cmd.bits.CmdParam = 0;
    if (cfg.debug_level > 0) {
	uart_printf("DevCmd.value: 0x%08X\n", cmd.value );
    }

    *DevCtrlAdr = cmd;
    stat = *DevStatAdr;
    int autoscan = (stat.bits.AdcMode >> (adc_num-1));
    if (cfg.debug_level > 0) {
	uart_printf("DevStat.value: 0x%08X\n", stat.value );
    }
    autoscan &= 0x01;
    if (autoscan) {
	return OK;
    }
    return FAIL;
}

//---------------------------------------------------------
const char *print_response_details(uint8_t reg_num, uint8_t value)
{
    static char buf[128];

    if (reg_num == RegID) {
	snprintf(buf, sizeof(buf), "%-7s", reg_name(reg_num) );

    } else if (reg_num == RegStatus0) {
	uint8_t CRC1 = (value & 0x40) >> 6;
	uint8_t STAT12 = (value & 0x10) >> 4;
	uint8_t REFALM = (value & 0x08) >> 4;
	uint8_t DRDY = (value & 0x04) >> 2;
	uint8_t CLOCK = (value & 0x02) >> 1;
	uint8_t RESET = (value & 0x01);
	snprintf(buf, sizeof(buf), "%-7s - CRC1:%u, STAT12:%u, REFALM:%u, DRDY:%u, CLOCK:%u, RESET:%u",
		 reg_name(reg_num), CRC1, STAT12, REFALM, DRDY, CLOCK, RESET);

    } else if (reg_num == RegStatus1) {
	snprintf(buf, sizeof(buf), "%-7s", reg_name(reg_num));

    } else if (reg_num == RegStatus2) {
	uint8_t CRC2 = (value & 0x10) >> 4;
	uint8_t ID2 = (value & 0x0F);
	snprintf(buf, sizeof(buf), "%-7s - CRC2:%u, REV_ID2:%Xh",
		 reg_name(reg_num), CRC2, ID2);

    } else if (reg_num == RegMode0) {
	uint8_t DRT = ((value >> 3) & 0x1F);
	uint8_t FLT = (value & 0x07);
	snprintf(buf, sizeof(buf), "%-7s - Data Rate: 0x%02X, Filter: 0x%02X",
		 reg_name(reg_num), DRT, FLT);

    } else if (reg_num == RegMode1) {
	uint8_t CNV = ((value >> 4) & 0x01);
	uint8_t DLY = (value & 0x0F);
	snprintf(buf, sizeof(buf), "%-7s - Conversion Mode:%u (%s), Delay: 0x%02X",
		 reg_name(reg_num), CNV, (CNV == 1 ? "Pulse" : "Conts"), DLY);

    } else if (reg_num == RegMode3) {
	uint8_t STATENB = (value >> 6) & 0x01;
	snprintf(buf, sizeof(buf), "%-7s - STATENB:%u", reg_name(reg_num), STATENB);

    } else if (reg_num == RegMode4) {
	uint8_t MUX = (value >> 4) & 0x07;
	uint8_t GAIN = (value & 0x0F);
	snprintf(buf, sizeof(buf), "%-7s - MUX: 0x%02X (Input %s%s), Gain: 0x%02X (%0.4f)",
		 reg_name(reg_num), MUX, (MUX == 0x00 ? "External" : ""),
		 (MUX == 0x05 ? "Internal" : ""), GAIN, gain(GAIN) );

    } else if (reg_num == RegRef) {
	uint8_t RMUX = (value & 0x0F);
	snprintf(buf, sizeof(buf), "%-7s - RMUX: 0x%02X (Reference %s%s)",
		 reg_name(reg_num), RMUX,
		 (RMUX == 0x05 ? "AVDD" : ""), (RMUX == 0x0A ? "External" : ""));

    } else if (reg_num == RegOFCAL0) {
	snprintf(buf, sizeof(buf), "%-7s", reg_name(reg_num));

    } else if (reg_num == RegOFCAL1) {
	snprintf(buf, sizeof(buf), "%-7s", reg_name(reg_num));

    } else if (reg_num == RegOFCAL2) {
	snprintf(buf, sizeof(buf), "%-7s", reg_name(reg_num));

    } else if (reg_num == RegFSCAL0) {
	snprintf(buf, sizeof(buf), "%-7s", reg_name(reg_num));

    } else if (reg_num == RegFSCAL1) {
	snprintf(buf, sizeof(buf), "%-7s", reg_name(reg_num));

    } else if (reg_num == RegFSCAL2) {
	snprintf(buf, sizeof(buf), "%-7s", reg_name(reg_num));

    } else {
	snprintf(buf, sizeof(buf), "reserved");
    }

    return buf;
}

//---------------------------------------------------------
const char *CmdOpCodeName(cmd_op_t cmdcode)
{
    if (cmdcode == CmdReadDevStat)
	return "CmdReadDevStat";
    if (cmdcode == CmdSetAdcWorkMode)
	return "CmdSetAdcWorkMode";
    if (cmdcode == CmdSetAdcTmr)
	return "CmdSetAdcTmr";
    if (cmdcode == CmdSetDirectCommand)
	return "CmdSetDirectCommand";
    return "UNKN";
}

//---------------------------------------------------------
void print_available_adc_registers()
{
    uart_puts("0h  - ID\n");
    uart_puts("01h - STATUS0\n");
    uart_puts("02h - MODE0\n");
    uart_puts("03h - MODE1\n");
    uart_puts("04h - RESERVED\n");
    uart_puts("05h - MODE3\n");
    uart_puts("06h - REF\n");
    uart_puts("07h - OFCAL0\n");
    uart_puts("08h - OFCAL1\n");
    uart_puts("09h - OFCAL2\n");
    uart_puts("0Ah - FSCAL0\n");
    uart_puts("0Bh - FSCAL1\n");
    uart_puts("0Ch - FSCAL2\n");
    uart_puts("0Dh - RESERVED\n");
    uart_puts("0Eh - RESERVED\n");
    uart_puts("0Fh - RESERVED\n");
    uart_puts("10h - MODE4\n");
    uart_puts("11h - STATUS1\n");
    uart_puts("12h - STATUS2\n");
}

//---------------------------------------------------------
const char *reg_name(uint8_t reg_num)
{
    if (reg_num == 0x00)
	return "ID";
    if (reg_num == 0x01)
	return "STATUS0";
    if (reg_num == 0x02)
	return "MODE0";
    if (reg_num == 0x03)
	return "MODE1";
    if (reg_num == 0x04)
	return "RESERVED";
    if (reg_num == 0x05)
	return "MODE3";
    if (reg_num == 0x06)
	return "REF";
    if (reg_num == 0x07)
	return "OFCAL0";
    if (reg_num == 0x08)
	return "OFCAL1";
    if (reg_num == 0x09)
	return "OFCAL2";
    if (reg_num == 0x0a)
	return "FSCAL0";
    if (reg_num == 0x0b)
	return "FSCAL1";
    if (reg_num == 0x0c)
	return "FSCAL2";
    if (reg_num == 0x0d)
	return "RESERVED";
    if (reg_num == 0x0e)
	return "RESERVED";
    if (reg_num == 0x0f)
	return "RESERVED";
    if (reg_num == 0x10)
	return "MODE4";
    if (reg_num == 0x11)
	return "STATUS1";
    if (reg_num == 0x12)
	return "STATUS2";
    return "UNKN";
}

//---------------------------------------------------------
const char *cmd_name(uint8_t cmdcode)
{
    if (cmdcode ==    0) return "NOP";
    if (cmdcode == 0x06) return "RESET";
    if (cmdcode == 0x08) return "START";
    if (cmdcode == 0x0A) return "STOP";
    if (cmdcode == 0x12) return "RDATA";
    if (cmdcode == 0x16) return "OFSCAL";
    if (cmdcode == 0x17) return "GANCAL";

    if (cmdcode >= 0x20 && cmdcode <= 0x20 + 0x12) {
	return reg_name(cmdcode - 0x20);
    }

    if (cmdcode >= 0x40 && cmdcode <= 0x40 + 0x12) {
	return reg_name(cmdcode - 0x40);
    }

    return "UNKN";
}

//---------------------------------------------------------
double gain(uint8_t gain_code) {
    if (gain_code == 0) return 0.125;
    if (gain_code == 1) return 0.1875;
    if (gain_code == 2) return 0.25;
    if (gain_code == 3) return 0.5;
    if (gain_code == 4) return 1;
    if (gain_code == 5) return 2;
    if (gain_code == 6) return 4;
    if (gain_code == 7) return 8;
    if (gain_code == 8) return 16;
    if (gain_code == 9) return 32;
    if (gain_code == 10) return 64;
    if (gain_code == 11) return 128;
    return 0.0;
}

//-----------------------------------------------
void print_dev_status(DevStat_t *stat)
{
    uart_printf("Dev Status: 0x%04X - AdcMode: 0x%X, AdcBusy: %u %u %u %u, AdcErr:0x%X, DevErr:0x%X\n",
		stat->value,
		stat->bits.AdcMode,
		(stat->bits.AdcBusy >> 3) & 0x01,
		(stat->bits.AdcBusy >> 2) & 0x01,
		(stat->bits.AdcBusy >> 1) & 0x01,
		(stat->bits.AdcBusy >> 0) & 0x01,
		stat->bits.AdcErr,
		stat->bits.DevErr);
}

//---------------------------------------------------------
void print_AdcResp(uint32_t valueHi, uint32_t valueLo)
{
    uart_printf("ADC Data (L/H): %08X %08X\n", valueLo, valueHi);

    AdcResp_t resp;
    resp.value = ((uint64_t) valueHi << 32) | valueLo;

    uart_printf("AdcResp: Echo cmd: 0x%02X, data: 0x%02X CRC: 0x%02X ",
		resp.bits.EchoCmd, resp.bits.EchoData, resp.bits.EchoCrc);
    uart_printf(" Status: 0x%02X, Msb: 0x%02X, Mid: 0x%02X, Lsb: 0x%02X, Out CRC: 0x%02X\n",
		resp.bits.AdcStatus, resp.bits.AdcMsbData, resp.bits.AdcMidData,
		resp.bits.AdcLsbData, resp.bits.AdcCrcResp);
}

//---------------------------------------------------------
void print_AdcStatus(int adc_num, uint8_t status0)
{
    bool ok;
    uint8_t status1 = 0;
    uint8_t status2 = 0;

    if (status0 == 0) return;
    uart_putc('\n');

    if (cfg.debug_level > 0 || (status0 & 0x40 || status0 & 0x10)) {
	print_status0(status0, 0);
    }

    if (status0 & 0x40 || status0 & 0x10) {
	// CRC1 STST12
	AdcResp_t response;
	ok = ADC_DirectCommand(adc_num, CMDOP_RREG + RegStatus1, new_arbitrary(), &response);
	if (ok) {
	    status1 = response.bits.AdcLsbData;
	    print_status1(status1);
	}

	ok = ADC_DirectCommand(adc_num, CMDOP_RREG + RegStatus2, new_arbitrary(), &response);
	if (ok) {
	    status2 = response.bits.AdcLsbData;
	    print_status2(status2);
	}
    }
}

//---------------------------------------------------------
void print_status0(uint8_t status0, int mode)
{
    uart_printf( "STATUS0: 0x%02X =>", status0);
    if (status0 & 0x40) uart_printf( " CRC1:1");
    if (status0 & 0x10) uart_printf( " STAT12:1");
    if (status0 & 0x08) uart_printf( " REFALM:1");
    if (status0 & 0x04) uart_printf( " DRDY:1");
    if (status0 & 0x02) uart_printf( " CLOCK:1");
    if (status0 & 0x01) uart_printf( " RESET:1");
    if (mode == 1) return;
    uart_putc('\n');

    if (status0 & 0x40) {
	uart_puts("CRC1: 1   - CRC error occurred during commands using CS1\n");
    }
    if (status0 & 0x10) {
	uart_puts("STAT12: 1 - Error logged in the STATUS1 or STATUS2 registers\n");
    }
    if (status0 & 0x08) {
	uart_puts("REFALM: 1 - Reference low alarm\n");
    }
    if (status0 & 0x04) {
	uart_puts("DRDY: 1   - Conversion data are new from the last data read\n");
    }
    if (status0 & 0x02) {
	uart_puts("CLOCK: 1  - ADC clock is external\n");
    }
    if (status0 & 0x01) {
	uart_puts("RESET: 1  - Indicates an ADC reset has occurred\n");
    }
}

//---------------------------------------------------------
void print_status1(uint8_t status1)
{
    uart_printf("STATUS1: 0x%02X", status1);
    if (status1 & 0x80) {
	uart_puts("  PGA_ONL:1 (PGA Output Negative Low Alarm)\n");
    }
    if (status1 & 0x40) {
	uart_puts("  PGA_ONH:1 (PGA Output Negative High Alarm)\n");
    }
    if (status1 & 0x20) {
	uart_puts("  PGA_OPL:1 (PGA Output Positive Low Alarm)\n");
    }
    if (status1 & 0x10) {
	uart_puts("  PGA_OPH:1 (PGA Output Positive High Alarm)\n");
    }
    if (status1 & 0x08) {
	uart_puts("  PGA_INL:1 (PGA Input Negative Low Alarm)\n");
    }
    if (status1 & 0x04) {
	uart_puts("  PGA_INH:1 (PGA Input Negative High Alarm)\n");
    }
    if (status1 & 0x02) {
	uart_puts("  PGA_IPL:1 (PGA Input Positive Low Alarm)\n");
    }
    if (status1 & 0x01) {
	uart_puts("  PGA_IPH:1 (PGA Input Positive High Alarm)\n");
    }
    uart_putc('\n');
}

//---------------------------------------------------------
void print_status2(uint8_t status2)
{
    uart_printf("STATUS2: 0x%02X", status2);
    if (status2 & 0x10) {
	uart_printf("  CRC2:   1 (CRC error occurred during commands with use of CS2)\n",
		    (status2 & 0x10) >> 4);
    }
}
