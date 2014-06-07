/**
 * \file
 * Pin definitions
 */
//#include <ArduinoPins.h>
#include <avr/io.h>
#ifndef WavePinDefs_h
#define WavePinDefs_h

//SPI pin definitions

/** SPI slave select pin. Warning: SS may be redefined as another pin
 but the hardware SS_PIN must be set to output mode before any calls to
 WaveHC functions. The SS_PIN can then be used as a general output pin */
#define SS   PB2

/** SPI master output, slave input pin. */
#define MOSI PB3

/** SPI master input, slave output pin. */
#define MISO PB4

/** SPI serial clock pin. */
#define SCK  PB5

#endif // WavePinDefs_h
