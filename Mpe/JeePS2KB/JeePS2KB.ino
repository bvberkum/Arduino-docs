/*
   PS2 Keyboard emulator for controlling system with PS/2 keyboard port.

*/

#include "ps2dev.h"    // to emulate a PS/2 device

int KBD_CLK = 4; // digital 4
int KBD_DATA = 5; // digital 5

PS2dev keyboard(KBD_CLK, KBD_DATA);  // PS2dev object (clock, data)
//int kbd_initialized = 0;       // pseudo variable for state of "keyboard"
int kbd_enabled = 0;       // pseudo variable for state of "keyboard"
unsigned char kbd_c;  		//char stores data recieved from computer for KBD

// serial char buffer
char inData[20];
char inChar = -1;
int index = 0;

// Use the on-board LED as an activity display
int LED_RX = 13; // digital, SCK, PB5
int LED_TX = 12; // digital, MISO, PB4


void blink(int led, int count, int length) {
	for (int i=0;i<count;i++) {
		delay(length);
		digitalWrite (led, HIGH);
		delay(length);
		digitalWrite (led, LOW);
	}
}

void kbd_init()
{
	// send the keyboard start up
	while (keyboard.write(0xAA)!=0);
  //while(mouse.write(0xAA)!=0);  
    //while(keyboard.write(0x00)!=0);
	delay(10);
}

void kbd_write(int key) {
	Serial.print("Sending key 0x");
	Serial.println(key);

	keyboard.write(key); 
	delay(40);

	keyboard.write(0xF0); // release
	delay(10);
	keyboard.write(key);
	delay(10);
}

void kbd_write2(int key) {
	Serial.print("Sending key 0x");
	Serial.println(key);

	keyboard.write(0xE0);
	delay(10);
	keyboard.write(key); 
	delay(40);

	keyboard.write(0xE0);
	delay(10);
	keyboard.write(0xF0);
	delay(10);
	keyboard.write(key);
	delay(10);
}

void kbd_write_pause() {
	keyboard.write(0xE1);
	keyboard.write(0x14);
	keyboard.write(0x77);
	keyboard.write(0xE1);
	keyboard.write(0xF0);
	keyboard.write(0x14);
	keyboard.write(0xF0);
	keyboard.write(0x77);
}

void kbd_ack()
{
	//acknowledge commands
	while(keyboard.write(0xFA));
}

int keyboardcommand(int command)
{
	unsigned char val;
	switch (command)
	{
		case 0xFF: //reset
			kbd_ack();
			//the while loop lets us wait for the host to be ready
			Serial.println("waiting for host");
			while(keyboard.write(0xAA)!=0);
			break;
		case 0xFE: //resend
			kbd_ack();
			break;
		case 0xF6: //set defaults
			//enter stream mode
			kbd_ack();
			break;
		case 0xF5: //disable data reporting
			//F
			kbd_enabled = 0;
			kbd_ack();
			break;
		case 0xF4: //enable data reporting
			//FM
			kbd_enabled = 1;
			kbd_ack();
			break;
		case 0xF3: //set typematic rate
			kbd_ack();
			keyboard.read(&val); //do nothing with the rate
			Serial.println("rate "+ val);
			kbd_ack();
			break;
		case 0xF2: //get device id
			kbd_ack();
			keyboard.write(0xAB);
			keyboard.write(0x83);
			break;
		case 0xF0: //set scan code set
			kbd_ack();
			keyboard.read(&val); //do nothing with the rate
			Serial.println("scan "+ val);
			kbd_ack();
			break;
		case 0xEE: //echo
			//kbd_ack();
			keyboard.write(0xEE);
			break;
		case 0xED: //set/reset LEDs
			kbd_ack();
			keyboard.read(&val); //do nothing with the rate
			Serial.println("led "+ val);
			kbd_ack();
			break;
	}
}

void resetSerial() {
	for (int i;i<19;i++) {
		inData[i]=0;
	}
	index = 0;
}

void readSerial() {
	if (index < 19) {
		inChar = Serial.read();
		inData[index] = inChar;
		index++;
		inData[index] = '\0';
	}
	else {
		Serial.println("SerialCommandOverflow");
		while (Serial.available() != 0) {
			blink(LED_TX, 1, 80);
			Serial.read(); }
	}
	blink(LED_RX, 1, 40);
	delay(10);
}

void setup() {
	Serial.begin(9600);
	Serial.println("JeePS2KB");

	// Set up the activity display LED
	pinMode (LED_TX, OUTPUT);
	pinMode (LED_RX, OUTPUT);

	// Disable timer0 since it can mess with the USB timing. Note that
	// this means some functions such as delay() will no longer work.
	//  TIMSK0&=!(1<<TOIE0);

	digitalWrite(LED_TX, LOW);
	digitalWrite(LED_RX, LOW);
	blink(LED_RX,5,80);
	blink(LED_TX,5,80);

	Serial.println("Waiting for keyboard setup");
	kbd_init();
	Serial.println("Keyboard setup done");

	delay(100);

}

void loop() 
{
	// if host device wants to send a command:
	if ((digitalRead(KBD_CLK) == LOW) || (digitalRead(KBD_DATA) == LOW))
	{
		blink(LED_RX,2,80);
		//Serial.print("Reading from PS/2 host: 0x");
		while(keyboard.read(&kbd_c)) ;
		//Serial.println(kbd_c, HEX);
		keyboardcommand(kbd_c);
		delay(20);
	}

	/** Serial read and echo */
	while (Serial.available() > 0) {
		readSerial();
	}

	if (inData[0] != 0) {
		blink(LED_TX, 2, 40);
		if (strcmp(inData, "f1") == 0) {
			kbd_write(0x05);
		}
		else if (strcmp(inData, "f9") == 0) {
			kbd_write(0x01);
		}
		else if (strcmp(inData, "f10") == 0) {
			kbd_write(0x09);
		}
		else if (strcmp(inData, "esc") == 0) {
			kbd_write(0x76);
		}
		else if (strcmp(inData, "bsp") == 0) {
			kbd_write(0x66);
		}
		else if (strcmp(inData, "sp") == 0) {
			kbd_write(0x29);
		}
		else if (strcmp(inData, "tab") == 0) {
			kbd_write(0x0D);
		}
		else if (strcmp(inData, "pause") == 0) {
			kbd_write_pause();
		}
		else if (strcmp(inData, "ret") == 0) {
			kbd_write(0x5A);
		}
		else if (strcmp(inData, "-") == 0) {
			kbd_write(0x7B); 
		}
		else if (strcmp(inData, "+") == 0) {
			kbd_write(0x79); 
		}
		else if (strcmp(inData, "h") == 0) {
			kbd_write2(0x6B); // left
		}
		else if (strcmp(inData, "j") == 0) {
			kbd_write2(0x72); // down
		}
		else if (strcmp(inData, "k") == 0) {
			kbd_write2(0x75); // up
		}
		else if (strcmp(inData, "l") == 0) {
			kbd_write2(0x74); // right
		}
		else if (strcmp(inData, "reset") == 0) {

			Serial.println("Sending reset");
			// XXX: bvb: have not seen tis work
			keyboard.write(0x14); //ctrl
			keyboard.write(0x11); //alt
			keyboard.write(0xE0); //del leading
			keyboard.write(0x71); //del code

			delay(60); //release del
			keyboard.write(0xE0);
			keyboard.write(0xF0);
			keyboard.write(0x71);
			delay(20); //release ctrl
			keyboard.write(0xF0);
			keyboard.write(0x14);
			delay(20); //release alt
			keyboard.write(0xF0);
			keyboard.write(0x11);
		}
		else {
			Serial.println("?");
			resetSerial();
			return;
		}
		//		// secancodes: http://www.computer-engineering.org/ps2keyboard/scancodes2.html
		//		keyboard.write(0x1C); // \
		//		keyboard.write(0xF0); //  |- send 'a'
	//		keyboard.write(0x1C); // /

		Serial.println("SerialACK");
		resetSerial();
	}
}

