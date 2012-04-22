/*
   PS2 Keyboard emulator for controlling system with PS/2 keyboard port.

*/

#include "ps2dev.h"

//PS2dev keyboard(3,2); // 2:data 3:clock
PS2dev keyboard(11,6); // 6:data,green (JP1D) 11:clock,white (JP2D)
int enabled = 0; // pseudo var for keyboard state
int initialized = 0;
int inByte = 0;

char inData[20];
char inChar = -1;
int index = 0;

void ack() {
	Serial.println("ack");
	while(keyboard.write(0xFA));
}

int keyboardcommand(int command) {
	unsigned char val;
	switch (command) {
		case 0xFF: //reset
			ack();
			Serial.println("Entering reset");
			//the while loop lets us wait for the host to be ready
			while(keyboard.write(0xAA)!=0);
			Serial.println("Reset done");
			break;
		case 0xFE: //resend
			ack();
			break;
		case 0xF6: //set defaults
			//enter stream mode
			ack();
			break;
		case 0xF5: //disable data reporting
			//FM
			enabled = 0;
			Serial.println("Disable");
			ack();
			break;
		case 0xF4: //enable data reporting
			//FM
			enabled = 1;
			Serial.println("Enable");
			ack();
			break;
		case 0xF3: //set typematic rate
			ack();
			keyboard.read(&val); //do nothing with the rate
			ack();
			break;
		case 0xF2: //get device id
			ack();
			keyboard.write(0xAB);
			keyboard.write(0x83);
			break;
		case 0xF0: //set scan code set
			ack();
			keyboard.read(&val); //do nothing with the rate
			ack();
			break;
		case 0xEE: //echo
			//ack();
			keyboard.write(0xEE);
			break;
		case 0xED: //set/reset LEDs
			ack();
			keyboard.read(&val); //do nothing with the rate
			ack();
			break;
	}
}

void setup() {
	Serial.begin(9600);
	Serial.println("JeePS2KB");
//	if (initialized == 0) {
//		Serial.println("Waiting for keyboard setup");
//		// send the keyboard start up
//		while (keyboard.write(0xAA)!=0);
//		delay(10);
//		initialized = 1;
//		Serial.println("Keyboard setup done");
//	}
}

void loop() 
{
	unsigned char c;
	// if host device wants to send a command:
//	if( (digitalRead(3) == LOW) || (digitalRead(2) == LOW)) {
//		Serial.print('.');
//		while(keyboard.read(&c)) ;
//		keyboardcommand(c);
//	}
//	else 
//	{ 
		//send keypresses accordingly using scancodes
		while (Serial.available() > 0) {
			if (index < 19) {
				inChar = Serial.read();
				inData[index] = inChar;
				index++;
				inData[index] = '\0';
			}
			else {
				Serial.println("SerialCommandOverflow");
				while (Serial.available() != 0) {
					Serial.read(); }
			}
			delay(10);
		}
		if (inData[0] != 0) {
			Serial.println(inData);
			for (int i;i<19;i++) {
				inData[i]=0;
			}
			index = 0;
			Serial.println("SerialACK");
		}
//		// secancodes: http://www.computer-engineering.org/ps2keyboard/scancodes2.html
//		keyboard.write(0x1C); // \
//		keyboard.write(0xF0); //  |- send 'a'
//		keyboard.write(0x1C); // /
		delay (1000); // wait 1 second
//	}
}

