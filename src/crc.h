/**
* \file   dev.h
* \author Aleksey Lekhtin
* \date   April 5, 2024
* \brief  ESZ7 DEV API
* \copyright Copyright (C) 2019 Texas Instruments Incorporated - http://www.ti.com/
*/

#include <stdint.h>

////uint8_t CalcCRC(const uint8_t byte1, const uint8_t byte2);

uint8_t CalcCRC(const uint8_t dataBytes[], uint8_t numBytes);
