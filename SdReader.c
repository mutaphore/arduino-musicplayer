#include <avr/io.h>
#include <util/delay.h>
#include "globals.h"
#include "SdReader.h"
#include "WavePinDefs.h"

uint32_t block_;
uint8_t errorCode_=0;
uint8_t errorData_=0;
uint8_t inBlock_=0;
uint16_t offset_;
uint8_t partialBlockRead_=0;
uint8_t response_;
uint8_t type_=0;

//------------------------------------------------------------------------------
// inline SPI functions
/** Send a byte to the card */
inline void spiSend(uint8_t b) {SPDR = b; while(!(SPSR & (1 << SPIF)));}
/** Receive a byte from the card */
inline uint8_t spiRec(void) {spiSend(0XFF); return SPDR;}
/** Set Slave Select high */
inline void spiSSHigh(void) {
   //digitalWrite(SS, HIGH);
   sbi(PORTB, SS);

   // insure SD data out is high Z
   spiSend(0XFF);
}
/** Set Slave Select low */
inline void spiSSLow(void) {
   //digitalWrite(SS, LOW);
   cbi(PORTB, SS);
}

//------------------------------------------------------------------------------
// card status
/** status for card in the ready state */
#define R1_READY_STATE 0
/** status for card in the idle state */
#define R1_IDLE_STATE  1
/** start data token for read or write */
#define DATA_START_BLOCK      0XFE
/** mask for data response tokens after a write block operation */
#define DATA_RES_MASK         0X1F
/** write data accepted token */
#define DATA_RES_ACCEPTED     0X05
/** write data crc error token */
#define DATA_RES_CRC_ERROR    0X0B
/** write data programming error token */
#define DATA_RES_WRITE_ERROR  0X0D

void error1(uint8_t code) {errorCode_ = code;}
void error2(uint8_t code, uint8_t data) {errorCode_ = code; errorData_ = data;}

/**
 * Enable or disable partial block reads.
 * 
 * Enabling partial block reads improves performance by allowing a block
 * to be read over the SPI bus as several sub-blocks.  Errors will occur
 * if the time between reads is too long since the SD card will timeout.
 *
 * Use this for applications like the Adafruit Wave Shield.
 *
 * \param[in] value The value TRUE (non-zero) or FALSE (zero).)   
 */     
void sdPartialBlockRead(uint8_t value) {
   sdReadEnd(); 
   partialBlockRead_ = value;
}

/**
 * Read a 512 byte block from a SD card device.
 *
 * \param[in] block Logical block to be read.
 * \param[out] dst Pointer to the location that will receive the data. 
 *
 * \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.      
 */ 
uint8_t sdReadBlock(uint32_t block, uint8_t *dst) {
   return sdReadData(block, 0, dst, 512);
}

/** Return the card type: SD V1, SD V2 or SDHC */
uint8_t sdType(void) {return type_;}
void sdSetType(uint8_t t) {type_ = t;}

//------------------------------------------------------------------------------
// send command to card
uint8_t sdCardCommand(uint8_t cmd, uint32_t arg) {
   uint8_t r1;
   uint8_t retry;
   signed char s;

   // end read if in partialBlockRead mode
   sdReadEnd();

   // select card
   spiSSLow();

   // wait up to 300 ms if busy
   sdWaitNotBusy(300);

   // send command
   spiSend(cmd | 0x40);

   // send argument
   for (s = 24; s >= 0; s -= 8) spiSend(arg >> s);

   // send CRC
   uint8_t crc = 0XFF;
   if (cmd == CMD0) crc = 0X95; // correct crc for CMD0 with arg 0
   if (cmd == CMD8) crc = 0X87; // correct crc for CMD8 with arg 0X1AA
   spiSend(crc);

   // wait for response
   for (retry = 0; ((r1 = spiRec()) & 0X80) && retry != 0XFF; retry++);

   return r1;
}

//------------------------------------------------------------------------------
/**
 * Determine the size of an SD flash memory card.
 * \return The number of 512 byte data blocks in the card
 */ 
uint32_t sdCardSize(void) {
   union csd_t csd;
   if (!sdReadCSD(&csd)) return 0;
   if (csd.v1.csd_ver == 0) {
      uint8_t read_bl_len = csd.v1.read_bl_len;
      uint16_t c_size = (csd.v1.c_size_high << 10)
         | (csd.v1.c_size_mid << 2) | csd.v1.c_size_low;
      uint8_t c_size_mult = (csd.v1.c_size_mult_high << 1)
         | csd.v1.c_size_mult_low;
      return (uint32_t)(c_size + 1) << (c_size_mult + read_bl_len - 7);
   }
   else if (csd.v2.csd_ver == 1) {
      uint32_t c_size = ((uint32_t)csd.v2.c_size_high << 16)
         | (csd.v2.c_size_mid << 8) | csd.v2.c_size_low;
      return (c_size + 1) << 10;
   }
   else {
      error1(SD_CARD_ERROR_BAD_CSD);
      return 0;
   }
}

//------------------------------------------------------------------------------
/**
 * Initialize a SD flash memory card.
 *
 * \param[in] slow If \a slow is false (zero) the SPI bus will
 * be initialize at a speed of 8 Mhz.  If \a slow is true (nonzero)
 * the SPI bus will be initialize a speed of 4 Mhz. This may be helpful
 * for some SD cards with Version 1.0 of the Adafruit Wave Shield.
 *
 * \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure. 
 *
 */  
uint8_t sdInit(uint8_t slow) {
   uint8_t r;
   uint16_t i;
   uint8_t retry;
   uint32_t t0=0;

   //pinMode(SS, OUTPUT);
   DDRB |= _BV(SS);

   //digitalWrite(SS, HIGH);
   PORTB |= _BV(SS);

   //pinMode(MOSI, OUTPUT);
   DDRB |= _BV(MOSI);

   //pinMode(MISO_PIN, INPUT);

   //pinMode(SCK, OUTPUT);
   DDRB |= _BV(SCK);

#if SPI_INIT_SLOW
   // Enable SPI, Master, clock rate f_osc/128
   SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR1) | (1 << SPR0);
#else  // SPI_INIT_SLOW
   // Enable SPI, Master, clock rate f_osc/64
   SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR1);
#endif  // SPI_INIT_SLOW

   // must supply min of 74 clock cycles with CS high.
   for (i = 0; i < 10; i++) spiSend(0XFF);

   // next two lines prevent re-init hang by cards that were in partial read
   spiSSLow();
   for (i = 0; i <= 512; i++) spiRec();

   // command to go idle in SPI mode
   for (retry = 0; ; retry++) {
      if ((r = sdCardCommand(CMD0, 0)) ==  R1_IDLE_STATE) break;
      if (retry == 10) {
         error2(SD_CARD_ERROR_CMD0, r);
         return 0;
      }
   }

   // check SD version
   r = sdCardCommand(CMD8, 0x1AA);
   if (r == R1_IDLE_STATE) {
      for(i = 0; i < 4; i++) {
         r = spiRec();
      }
      if (r != 0XAA) {
         error2(SD_CARD_ERROR_CMD8_ECHO, r);
         return 0;
      }
      sdSetType(SD_CARD_TYPE_SD2);
   }
   else if (r & R1_ILLEGAL_COMMAND) {
      sdSetType(SD_CARD_TYPE_SD1);
   }
   else {
      error2(SD_CARD_ERROR_CMD8, r);
   }

   // initialize card and send host supports SDHC if SD2
   t0=0;
   for (;;) {
      _delay_us(2);
      t0++;
      sdCardCommand(CMD55, 0);
      r = sdCardCommand(ACMD41, sdType() == SD_CARD_TYPE_SD2 ? 0X40000000 : 0);
      if (r == R1_READY_STATE) break;

      // timeout after 2 seconds
      if ((t0/1000) > 2000) {
         error1(SD_CARD_ERROR_ACMD41);
         return 0;
      }
   }

   // if SD2 read OCR register to check for SDHC card
   if (sdType() == SD_CARD_TYPE_SD2) {
      if(sdCardCommand(CMD58, 0)) {
         error1(SD_CARD_ERROR_CMD58);
         return 0;
      }
      if ((spiRec() & 0XC0) == 0XC0) sdSetType(SD_CARD_TYPE_SDHC);

      // discard rest of ocr
      for (i = 0; i < 3; i++) spiRec();
   }

   // use max SPI frequency unless slow is true
   SPCR &= ~((1 << SPR1) | (1 << SPR0)); // f_OSC/4

   if (!slow) SPSR |= (1 << SPI2X); // Doubled Clock Frequency: f_OSC/2
   spiSSHigh();
   return 1;
}

//------------------------------------------------------------------------------
/**
 * Read part of a 512 byte block from a SD card.
 *  
 * \param[in] block Logical block to be read.
 * \param[in] offset Number of bytes to skip at start of block
 * \param[out] dst Pointer to the location that will receive the data. 
 * \param[in] count Number of bytes to read
 * \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.      
 */
uint8_t sdReadData(uint32_t block,
      uint16_t offset, uint8_t *dst, uint16_t count) {
   uint16_t i;

   if (count == 0) return 1;
   if ((count + offset) > 512) {
      return 0;
   }
   if (!inBlock_ || block != block_ || offset < offset_) {
      block_ = block;

      // use address if not SDHC card
      if (sdType()!= SD_CARD_TYPE_SDHC) block <<= 9;
      if (sdCardCommand(CMD17, block)) {
         error1(SD_CARD_ERROR_CMD17);
         return 0;
      }
      if (!sdWaitStartBlock()) {
         return 0;
      }
      offset_ = 0;
      inBlock_ = 1;
   }

   // start first SPI transfer
   SPDR = 0XFF;

   // skip data before offset
   for (;offset_ < offset; offset_++) {
      while(!(SPSR & (1 << SPIF)));
      SPDR = 0XFF;
   }

   // transfer data
   uint16_t n = count - 1;
   for (i = 0; i < n; i++) {
      while(!(SPSR & (1 << SPIF)));
      dst[i] = SPDR;
      SPDR = 0XFF;
   }

   // wait for last byte
   while(!(SPSR & (1 << SPIF)));
   dst[n] = SPDR;
   offset_ += count;
   if (!partialBlockRead_ || offset_ >= 512) sdReadEnd();
   return 1;
}

//------------------------------------------------------------------------------
/** Skip remaining data in a block when in partial block read mode. */
void sdReadEnd(void) {
   if (inBlock_) {
      // skip data and crc
      SPDR = 0XFF;
      while (offset_++ < 513) {
         while(!(SPSR & (1 << SPIF)));
         SPDR = 0XFF;
      }
      // wait for last crc byte
      while(!(SPSR & (1 << SPIF)));
      spiSSHigh();
      inBlock_ = 0;
   }
}

//------------------------------------------------------------------------------
/** read CID or CSR register */
uint8_t sdReadRegister(uint8_t cmd, uint8_t *dst) {
   uint16_t i;
   if (sdCardCommand(cmd, 0)) {
      error1(SD_CARD_ERROR_READ_REG);
      return 0;
   }
   if(!sdWaitStartBlock()) return 0;

   //transfer data
   for (i = 0; i < 16; i++) dst[i] = spiRec();

   spiRec();// get first crc byte
   spiRec();// get second crc byte

   spiSSHigh();
   return 1;
}

//------------------------------------------------------------------------------
// wait for card to go not busy
uint8_t sdWaitNotBusy(uint16_t timeoutMillis) {
   uint32_t t0 = 0;
   while (spiRec() != 0XFF) {
      t0++;
      _delay_us(2);
      if ((t0/1000) > timeoutMillis) return 0;
   }
   return 1;
}

//------------------------------------------------------------------------------
/** Wait for start block token */
uint8_t sdWaitStartBlock(void) {
   uint8_t r;
   uint32_t t0 = 0;
   while ((r = spiRec()) == 0XFF) {
      t0++;
      _delay_us(2);
      if ((t0/1000) > SD_READ_TIMEOUT) {
         error1(SD_CARD_ERROR_READ_TIMEOUT);
         return 0;
      }
   }
   if (r == DATA_START_BLOCK) return 1;
   error2(SD_CARD_ERROR_READ, r);
   return 0;
}

/** 
 * Read a cards CSD register. The CSD contains Card-Specific Data that
 * provides information regarding access to the card contents. */
uint8_t sdReadCSD(union csd_t* csd) {
   return sdReadRegister(CMD9, (uint8_t *)csd);
}

/** 
 * Read a cards CID register. The CID contains card identification information
 * such as Manufacturer ID, Product name, Product serial number and
 * Manufacturing date. */
uint8_t sdReadCID(cid_t* cid) {
   return sdReadRegister(CMD10, (uint8_t *)cid);
}

