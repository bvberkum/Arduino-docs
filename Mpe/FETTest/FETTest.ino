#include <Ports.h>
#include <PortsSHT11.h>
#include <RF12.h>
#include <avr/sleep.h>


Port port1 (1); 

Port loop_indicator (4); // loop led indicator at JP4A

static byte myNodeID;       // node ID used for this unit

void blinkLed(byte count, int interval=5)
{
    for (byte i = 0; i < count; ++i) {
    	if (i > 0)
			delay(interval);
		loop_indicator.digiWrite2(1);
		delay(interval);
		loop_indicator.digiWrite2(0);
	}
}

void setup () {
	Serial.begin(9600);
    Serial.print("FETTest");
    
    loop_indicator.mode2(OUTPUT);
	loop_indicator.digiWrite2(0);

	port1.mode2(OUTPUT);
	port1.digiWrite2(0);

	myNodeID = rf12_config();
    
    rf12_sleep(RF12_SLEEP); // power down
	
	blinkLed(3, 100);
}

void loop () {
	Serial.print('.');
	blinkLed(1,100);
	h
	delay(1000);
}
