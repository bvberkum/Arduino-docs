#ifndef mpelib_avr_h
#define mpelib_avr_h


#if defined(__AVR_ATtiny84__) || defined(__AVR_ATtiny85__)
#define SRAM_SIZE       512
#define EEPROM_SIZE     512
#elif defined(__AVR_ATmega168__) || defined(__AVR_ATmega8__)
#define SRAM_SIZE       1024
#define EEPROM_SIZE     512
#elif defined(__AVR_ATmega328__) || defined(__AVR_ATmega328P__)
#define SRAM_SIZE       2048
#define EEPROM_SIZE     1024
#else
#error Chip not supported, plase fill in SRAM_SIZE/EEPROM_SIZE
#endif

#include <avr/io.h>



static int freeRam (void) {
	extern int __heap_start, *__brkval; 
	int v;
	return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

static int usedRam (void) {
	return SRAM_SIZE - freeRam();
}

#endif

