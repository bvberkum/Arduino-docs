/* nRF24L01+ testing
 */
#include <SPI.h>
#include "nRF24L01.h"
#include <RF24.h>

#include "printf.h"


#define DEBUG       1
#define SERIAL      1

/** Generic routines */

#if SERIAL 
static void serialFlush () {
#if ARDUINO >= 100
  Serial.flush();
#endif  
  delay(2); // make sure tx buf is empty before going back to sleep
}
#endif

/* Roles supported by this sketch */
typedef enum { 
  role_reporter = 1,  /* start enum at 1, 0 is reserved for invalid */
  role_logger
} Role;

const char* role_friendly_name[] = { 
  "invalid", 
  "Reporter", 
  "Logger"
};

Role role;// = role_logger;

/* nRF24L01+: 2.4Ghz radio  */
const uint64_t pipes[2] = { 
  0xF0F0F0F0E1LL, /* local id */
  0xF0F0F0F0D2LL /* node id */
};
RF24 radio(12, 13); /* CE, CS */

void radio_init()
{
  /* Setup and configure rf radio */
  radio.begin();
  radio.setRetries(15,15); /* delay, number */
  /* Start radio */
  radio.openWritingPipe(pipes[0]);
  radio.openReadingPipe(1,pipes[1]);
  radio.startListening();
#if SERIAL && DEBUG
  radio.printDetails();
  serialFlush();
#endif
}

void radio_run()
{
  if (radio.available()) {
	unsigned long got_time;
	bool done = false;
	while (!done)
	{
	  done = radio.read( &got_time, sizeof(unsigned long) );
#if SERIAL && DEBUG	  
	  printf("Got payload %lu...", got_time);
#endif
	  delay(20);
	  radio.stopListening();
	  radio.write( &got_time, sizeof(unsigned long) );
#if SERIAL && DEBUG	  
	  printf("Sent response.\n\r");
#endif
	  radio.startListening();
	}
  }
}


/* Main */

void setup(void)
{
#if SERIAL
  Serial.begin(57600);
#endif
#if SERIAL && DEBUG
  printf_begin();
  Serial.print("\nRF24Logger");
  serialFlush();
#endif

  radio_init();
}

void loop(void)
{
#if SERIAL && DEBUG
  Serial.println('.');
#endif
  radio_run();
  delay(500);
}

// vim:cin:ai:noet:sts=2 sw=2 ft=cpp
