#ifndef mpelib_generic_h
#define mpelib_generic_h

#include <Arduino.h>


#ifndef MAXLENLINE
#define MAXLENLINE      79
#endif

#ifndef SMOOTH
#define SMOOTH          5   // smoothing factor used for running averages
#endif


int tick = 0;
int pos = 0;

/* *** Generic routines *** {{{ */

void serialFlush () {
#if SERIAL
#if ARDUINO >= 100
	Serial.flush();
#endif
	delay(2); // make sure tx buf is empty before going back to sleep
#endif
}

void blink(int led, int count, int length, int length_off=-1) {
	int i;
	for (i=0;i<count;i++) {
		digitalWrite (led, HIGH);
		delay(length);
		digitalWrite (led, LOW);
		(length_off > -1) ? delay(length_off) : delay(length);
	}
}

void debug_ticks(void)
{
#if SERIAL && DEBUG
	tick++;
	if ((tick % 20) == 0) {
		Serial.print('.');
		pos++;
	}
	if (pos > MAXLENLINE) {
		pos = 0;
		Serial.println();
	}
	serialFlush();
#endif
}

void debugline(char* msg) {
#if DEBUG
	Serial.println(msg);
#endif
}

// utility code to perform simple smoothing as a running average
int smoothedAverage(int prev, int next, byte firstTime) {
	if (firstTime)
		return next;
	return ((SMOOTH - 1) * prev + next + SMOOTH / 2) / SMOOTH;
}

/* }}} *** */


#endif
