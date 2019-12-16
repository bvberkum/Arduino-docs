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

extern void serialFlush() {
#if SERIAL_EN
#if ARDUINO >= 100
	Serial.flush();
#endif
	delay(2); // make sure tx buf is empty before going back to sleep
#endif
}

extern bool debug_ticks(void)
{
  bool r = false;
#if SERIAL_EN && DEBUG
	tick++;
	if ((tick % 20) == 0) {
		Serial.print('.');
		pos++;
		r = true;
	}
	if (pos > MAXLENLINE) {
		pos = 0;
		Serial.println();
	}
	serialFlush();
#endif
  return r;
}

extern void debugline(char* msg) {
#if SERIAL_EN && DEBUG
	Serial.println(msg);
#endif
}

void blink(int led, int count, int length, int length_off=-1, bool reverse=false) {
	int i;
	for (i=0;i<count;i++) {
		digitalWrite (led, (reverse and LOW ) or HIGH);
		delay(length);
		digitalWrite (led, (reverse and HIGH ) or LOW);
		(length_off > -1) ? delay(length_off) : delay(length);
	}
}

// utility code to perform simple smoothing as a running average
int smoothedAverage(int prev, int next, byte firstTime) {
	if (firstTime)
		return next;
	return ((SMOOTH - 1) * prev + next + SMOOTH / 2) / SMOOTH;
}

/* *** }}} */

#endif
