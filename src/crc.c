/**
* \file   crc.c
* \author Aleksey Lekhtin
* \date   Aug 13, 2024
* \brief  ADC125H02 CRC
* \copyright Copyright (C) 2019 Texas Instruments Incorporated - http://www.ti.com/
*/

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * \fn uint8_t calculateCRC(uint8_t *dataByte, uint8_t numBytes)
 * \brief Calculates the 8-bit CRC for data words, up to 4 bytes
 *
 * NOTE: This calculation is shown as an example and is not optimized for speed.
 * On a TM4C1294NCPDT, operating with a 120 MHz system clock, this function's
 * execution time is between 5-10 us.
 *
 *
 * --- CRC Calculation Description ---
 * The CRC shift register is initialized to 0xFF and all data is shifted in MSB first, as shown in the
 * diagram below. 'Bit 7' of the CRC shifted register is XOR'd with the next data bit and the result
 * is placed into 'Bit 0' of shift register and also used in additional XOR operations. 'Bit 1' takes
 * the value of 'Bit 0' XOR'd with the result of the first XOR operation. Similarly, 'Bit 2' takes
 * the value of 'Bit 1' XOR'd with the result of the first XOR operation. All other bits in the CRC
 * shift register are shifted, such that 'Bit N' takes on the previous value of 'Bit N-1'.
 *
 * NOTE: If the first XOR operation results in a '0', all other XOR operations retain the value of the
 * previous bit; and as such, can be ignored.
 *
 *                       Bit    Bit    Bit    Bit    Bit    Bit           Bit           Bit
 * CRC Shift register:   |7| << |6| << |5| << |4| << |3| << |2| << XOR << |1| << XOR << |0|
 *                        |                                         ^             ^      ^
 *                        V                                         |             |      |
 * Data IN (MSB 1st) ->> XOR -------------------------------------------------------------
 *
 *
 * \param *dataByte pointer to first element in the data byte array
 * \param numBytes number of bytes (between 1 and 4) used in CRC calculation
 * \return calculated CRC byte
 */

#if 0

#if 0
uint8_t calculateCRC(const uint8_t dataBytes[], uint8_t numBytes)
#else
uint8_t CalcCRC(const uint8_t byte1, const uint8_t byte2)
#endif

{
#if 0
	/* Check that "numBytes" is between 1 and 4 */
	assert((numBytes >= 1) && (numBytes <= 4));

	/* Check that "dataBytes" is not a null pointer */
	assert(dataBytes != NULL);
#else
	uint8_t numBytes = 2;
#endif

    /* NOTE:
     * Using "uint_fast8_t" types here instead of "uint8_t" to avoid unnecessary
     * implicit type conversions. Reference this E2E thread for additional info:
     * https://e2e.ti.com/support/microcontrollers/msp430/f/166/t/679200
     */
    uint_fast8_t crc        = 0xFFu;        /* Initial value of crc register     */
    bool crcMSb;                            /* Most significant bit of crc byte  */
    uint_fast8_t poly       = 0x07u;        /* CRC polynomial byte               */
    uint_fast8_t shift_by   = 0u;           /* Intermediate variable             */
    uint32_t data           = 0u;           /* Data storage variable             */
    uint32_t msbMask        = 0x80000000u;  /* Points to the next data bit       */
    bool dataMSb;                           /* Most significant bit of data int  */

#if 0
    /* Construct data word from data bytes */
    uint_fast8_t i;
    for (i = 0; i < numBytes; i++)
    {
    	shift_by = 8 * (numBytes - i - 1);
    	data |= (((uint32_t) dataBytes[i]) << shift_by);
    }
#else
    data = (uint32_t)(byte1 << 8) | (uint32_t)byte2;
#endif

    /* Determine the location of the first data byte */
    shift_by = 8 * (4 - numBytes);
    msbMask >>= shift_by;

    /* CRC algorithm */
    while (msbMask > 0)
    {
        // Check MSB's of data and crc
        dataMSb = (bool) (data & msbMask);
        crcMSb  = (bool) (crc & 0x80u);

        // Shift crc byte
        crc <<= 1;

        // Check if XOR operation of MSbs results in additional XOR operation
        if (dataMSb ^ crcMSb)   { crc ^= poly; }

        /* Shift MSb pointer */
        msbMask >>= 1;
    }

    return (uint8_t) (crc & 0xFF);
}
#endif


uint8_t CalcCRC(const uint8_t dataBytes[], uint8_t numBytes)
{
	/* Check that "numBytes" is between 1 and 4 */
	assert((numBytes >= 1) && (numBytes <= 4));

	/* Check that "dataBytes" is not a null pointer */
	assert(dataBytes != 0);

    /* NOTE:
     * Using "uint_fast8_t" types here instead of "uint8_t" to avoid unnecessary
     * implicit type conversions. Reference this E2E thread for additional info:
     * https://e2e.ti.com/support/microcontrollers/msp430/f/166/t/679200
     */
    uint_fast8_t crc        = 0xFFu;        /* Initial value of crc register     */
    bool crcMSb;                            /* Most significant bit of crc byte  */
    uint_fast8_t poly       = 0x07u;        /* CRC polynomial byte               */
    uint_fast8_t shift_by   = 0u;           /* Intermediate variable             */
    uint32_t data           = 0u;           /* Data storage variable             */
    uint32_t msbMask        = 0x80000000u;  /* Points to the next data bit       */
    bool dataMSb;                           /* Most significant bit of data int  */

    /* Construct data word from data bytes */
    uint_fast8_t i;
    for (i = 0; i < numBytes; i++)
    {
    	shift_by = 8 * (numBytes - i - 1);
    	data |= (((uint32_t) dataBytes[i]) << shift_by);
    }

    /* Determine the location of the first data byte */
    shift_by = 8 * (4 - numBytes);
    msbMask >>= shift_by;

    /* CRC algorithm */
    while (msbMask > 0)
    {
        // Check MSB's of data and crc
        dataMSb = (bool) (data & msbMask);
        crcMSb  = (bool) (crc & 0x80u);

        // Shift crc byte
        crc <<= 1;

        // Check if XOR operation of MSbs results in additional XOR operation
        if (dataMSb ^ crcMSb)   { crc ^= poly; }

        /* Shift MSb pointer */
        msbMask >>= 1;
    }

    return (uint8_t) (crc & 0xFF);
}
