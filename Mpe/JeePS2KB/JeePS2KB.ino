#include "ps2dev.h"

PS2dev keyboard(3,2); // 2:data 3:clock

void ack() {
	while(keyboard.write(0xFA));
}

int keyboardcommand(int command) {
	unsigned char val;
	switch (command) {
		case 0xFF: //reset
			ack();
			//the while loop lets us wait for the host to be ready
			while(keyboard.write(0xAA)!=0);
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
			ack();
			break;
		case 0xF4: //enable data reporting
			//FM
			enabled = 1;
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
	// send the keyboard start up
	while(keyboard.write(0xAA)!=0);
	delay(10);
}

void loop() {
	unsigned char c;
	// if host device wants to send a command:
	if( (digitalRead(3)==LOW) || (digitalRead(2) == LOW)) {
		while(keyboard.read(&c)) ;
		keyboardcommand(c);
	}
	else{ //send keypresses accordingly using scancodes
		// secancodes: http://www.computer-engineering.org/ps2keyboard/scancodes2.html
		keyboard.write(0x1C); // \
		keyboard.write(0xF0); //  |- send 'a'
		keyboard.write(0x1C); // /
		delay (1000); // wait 1 second
	}
}

