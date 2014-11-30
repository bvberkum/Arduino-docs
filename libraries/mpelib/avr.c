#include <avr/io.h>

#include "avr.h"


int freeRam (void) {
	extern int __heap_start, *__brkval; 
	int v;
	return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

int usedRam (void) {
	return SRAM_SIZE - freeRam();
}



