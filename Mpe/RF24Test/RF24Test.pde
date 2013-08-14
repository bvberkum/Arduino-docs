#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

//
// Hardware configuration
//

// Set up nRF24L01 radio on SPI bus plus pins 9 & 10 

//CSN: 8 (PB0)
//CE: 9 (PB1)

RF24 radio(9, 8); /* CE, CSN */

const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

void setup(void)
{
  Serial.begin(57600);
  //Serial.println("RF24Test");

  printf_begin();
  printf("\n\rRF24/examples/GettingStarted/\n\r");

  radio.begin();
  radio.openReadingPipe(1,pipes[1]);
  radio.startListening();

  radio.printDetails();
}

void loop() {
	
}

