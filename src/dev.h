/**
 \file   dev.h
 \author Aleksey Lekhtin
 \date   April 5, 2024
 \brief  ESZ7 DEV API
 */

#ifndef DEV__H_
#define DEV__H_

#include <stdbool.h>

#include "adc_t.h"

extern int selectedADC; // Номер выбранной АЦП для работы

extern volatile DevCmd_t *DevCtrlAdr;
extern volatile DevStat_t *DevStatAdr;
extern volatile uint32_t *FwVerRegAdr;
extern volatile uint32_t *AdcRegAdrLo;
extern volatile uint32_t *AdcRegAdrHi;
extern volatile int32_t  *Enc0RegAdr;
extern volatile int32_t  *Enc0LoadAdr;
extern volatile int32_t  *Enc1RegAdr;
extern volatile int32_t  *Enc1LoadAdr;
extern volatile uint32_t *EncDebAdr;

bool ADC_DirectCommand(int adc_num, uint8_t cmdop, uint8_t value, AdcResp_t *adc_resp);

DevStat_t ReadDevStat();
void SetADCAutoscanPeriod(int adc_num, uint32_t ticks);
bool AutoScan_Start(int adc_num);
bool AutoScan_Stop(int adc_num);
void DevReadADCOutputRegs();
bool WaitADCReady(int adc_num);

uint8_t channel_to_onehot(int channel);
bool autoscan_mode(int adc_num);
const char *CmdOpCodeName(cmd_op_t opcode);
void set_arbitrary(uint8_t num);
uint8_t new_arbitrary();
const char *reg_name(uint8_t reg_num);
const char *cmd_name(uint8_t cmdcode);
double gain(uint8_t gain_code);
void print_available_adc_registers();

////void print_available_adc_registers();
void print_AdcResp(uint32_t valueHi, uint32_t valueLo);
void print_AdcStatus(int adc_num, uint8_t status0);
const char *print_response_details(uint8_t reg_num, uint8_t value);
void print_dev_status(DevStat_t *stat);
void print_status0(uint8_t status0, int mode);
void print_status1(uint8_t status1);
void print_status2(uint8_t status2);



#endif // DEV__H_
