/**
\file   funcs.h
\author Aleksey Lekhtin
\date   April 5, 2024
\brief  Application features
*/

#ifndef FUNCS__H_
#define FUNCS__H_

#include "dev.h"

void select_adc();

// Control commands
void adc_nop();
void adc_reset();
void adc_start();
void adc_stop();

//Read data command
void adc_rdata();
void adc_start_and_rdata();

// Register commands
void adc_read_register();
void adc_write_register();

// Miscellaneous
void adc_read_id();
void adc_lists_registers();
void adc_read_regs();
void clear_encoders();
void read_encoders();

bool ADCReadRegister(int adc_num, uint8_t reg_num, uint8_t *value);
bool ADCWriteRegister(int adc_num, uint8_t reg_num, uint8_t value);
bool ADCReadID(int adc_num, uint8_t *id);

void init();
void print_firmware_version();
//// void print_calibration_info(int adc_num);
void setup_stuff();
void calibration_stuff();
void AutoScan_control(int adc_num);
void AutoScan_control_all();
void enable_irq();

void set_debug_level();

#endif // FUNCS__H_
