/* Arduino WaveHC Library
 * Copyright (C) 2008 by William Greiman
 *  
 * This file is part of the Arduino FAT16 Library
 *  
 * This Library is free software: you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 
 * You should have received a copy of the GNU General Public License
 * along with the Arduino Fat16 Library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef SdReader_h
#define SdReader_h
#include "SdInfo.h"

/**
 * Some SD card are very sensitive to the SPI bus speed for initialization.
 * Try setting SPI_INIT_SLOW nonzero if you have initialization problems.
 *
 * Set SPI_INIT_SLOW nonzero to reduce the SPI bus speed for SD initaizaton
 * to F_CPU/128.  F_CPU/64 is used if
 */
#define SPI_INIT_SLOW 1
/**
 * Default card SPI speed. Change to true for Wave Shield V1.0
 * The SPI speed is 4 Mhz for 'true' and 8 Mhz for 'false'.
 */
#define SPI_DEFAULT_HALF_SPEED false

/** read timeout ms */
#define SD_READ_TIMEOUT    300

// SD card errors
/** timeout error for command CMD0 */
#define SD_CARD_ERROR_CMD0  0X1
/** CMD8 was not accepted - not a valid SD card*/
#define SD_CARD_ERROR_CMD8  0X2
/** card returned an error response for CMD17 (read block) */
#define SD_CARD_ERROR_CMD17 0X3
/** card returned an error response for CMD24 (write block) */
#define SD_CARD_ERROR_CMD24 0X4
/** card returned an error response for CMD58 (read OCR) */
#define SD_CARD_ERROR_CMD58 0X5
/** card's ACMD41 initialization process timeout */
#define SD_CARD_ERROR_ACMD41 0X6
/** card returned a bad CSR version field */
#define SD_CARD_ERROR_BAD_CSD 0X7
/** read CID or CSD failed */
#define SD_CARD_ERROR_READ_REG 0X8
/** bad response echo from CMD8 */
#define SD_CARD_ERROR_CMD8_ECHO 0X09
/** timeout while waiting for start of read data */
#define SD_CARD_ERROR_READ_TIMEOUT 0XD
/** card returned an error token instead of read data */
#define SD_CARD_ERROR_READ 0X10
//
// card types
/** Standard capacity V1 SD card */
#define SD_CARD_TYPE_SD1 1
/** Standard capacity V2 SD card */
#define SD_CARD_TYPE_SD2 2
/** High Capacity SD card */
#define SD_CARD_TYPE_SDHC 3
//------------------------------------------------------------------------------

uint32_t cardSize(void);
uint8_t sdInit(uint8_t slow);
uint8_t sdWaitNotBusy(uint16_t timeoutMillis);
uint8_t sdReadData(uint32_t block, uint16_t offset, uint8_t *dst, uint16_t count);
void sdPartialBlockRead(uint8_t value);
uint8_t sdReadBlock(uint32_t block, uint8_t *dst);
uint8_t sdReadCID(cid_t* cid);
uint8_t sdReadCSD(union csd_t* csd);
void sdReadEnd(void);
uint8_t sdWaitStartBlock(void);
void error(uint8_t code, uint8_t data);
uint8_t sdType(void);
void sdSetType(uint8_t t);
uint8_t sdCardCommand(uint8_t cmd, uint32_t arg);
uint32_t sdCardSize(void);
uint8_t sdReadRegister(uint8_t cmd, uint8_t *dst);
#endif //SdReader_h
