//Global defines

#ifndef GLOBALS_H
#define GLOBALS_H

//AVR constants
#define RED 31
#define GREEN 32
#define YELLOW 33
#define BLUE 34
#define MAGENTA 35
#define CYAN 36
#define WHITE 37

#define SAMPLE_RATE 11000

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>

void serial_init();
uint8_t byte_available();
uint8_t read_byte();
uint8_t write_byte(uint8_t b);
void write_int(uint32_t num);
void print_string(char *s);
void print_int(uint16_t i);
void print_int32(uint32_t i);
void print_hex(uint16_t i);
void print_hex32(uint32_t i);
void set_cursor(uint8_t row, uint8_t col);
void clear_screen(void);

#endif
