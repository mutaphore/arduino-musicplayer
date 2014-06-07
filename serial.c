#include "globals.h"

#define LEN_16 6
#define LEN_32 11

/*
 * Initialize the serial port.
 */
void serial_init() {
   uint16_t baud_setting;

   UCSR0A = _BV(U2X0);
   baud_setting = 16; //115200 baud

   // assign the baud_setting
   UBRR0H = baud_setting >> 8;
   UBRR0L = baud_setting;

   // enable transmit and receive
   UCSR0B |= (1 << TXEN0) | (1 << RXEN0);
}

/*
 * Return 1 if a character is available else return 0.
 */
uint8_t byte_available() {
   return (UCSR0A & (1 << RXC0)) ? 1 : 0;
}

/*
 * Unbuffered read
 * Return 255 if no character is available otherwise return available character.
 */
uint8_t read_byte() {
   if (UCSR0A & (1 << RXC0)) return UDR0;
   return 255;
}

/*
 * Unbuffered write
 *
 * b byte to write.
 */
uint8_t write_byte(uint8_t b) {
   //loop until the send buffer is empty
   while (((1 << UDRIE0) & UCSR0B) || !(UCSR0A & (1 << UDRE0))) {}

   //write out the byte
   UDR0 = b;
   return 1;
}

/*
 * Write string
 *
 * s string to write.
 */
void print_string(char *s) {
   while (*s)
      write_byte(*s++);
}

void print_int(uint16_t i) {
   char data[LEN_16];
   uint8_t pos = LEN_16 - 1;

   if (i == 0) {
      write_byte('0');
      return;
   }
   
   data[pos] = 0;
   while (i) {
      data[--pos] = '0' + (i % 10);
      i /= 10;
   }
   print_string(data + pos);
}

void print_int32(uint32_t i) {
   char data[LEN_32];
   uint8_t pos = LEN_32 - 1;
  
   if (i == 0) {
      write_byte('0');
      return;
   }

   data[pos] = 0;
   while (i) {
      data[--pos] = '0' + (i % 10);
      i /= 10;
   }
   print_string(data + pos);
}

void print_hex(uint16_t i) {
   char data[LEN_16];
   uint8_t pos = LEN_16 - 1;
   uint8_t tmp;

   if (i == 0) {
      write_byte('0');
      return;
   }

   data[pos] = 0;
   while (i) {
      tmp = i % 16;
      data[--pos] = (tmp < 10) ? ('0' + tmp) : ('A' + tmp - 10);
      i /= 16;
   }
   print_string(data + pos);
}

void print_hex32(uint32_t i) {
   char data[LEN_32];
   uint8_t pos = LEN_32 - 1;
   uint8_t tmp;

   if (i == 0) {
      write_byte('0');
      return;
   }

   data[pos] = 0;
   while (i) {
      tmp = i % 16;
      data[--pos] = (tmp < 10) ? ('0' + tmp) : ('A' + tmp - 10);
      i /= 16;
   }
   print_string(data + pos);
}

void set_cursor(uint8_t row, uint8_t col) {
   write_byte(27);
   write_byte('[');
   print_int(row);
   write_byte(';');
   print_int(col);
   write_byte('H');
}

void set_color(uint8_t color) {
   write_byte(27);
   write_byte('[');
   print_int(color);
   write_byte('m');
}

void clear_screen() {
   write_byte(27);
   print_string("[2J");
}
