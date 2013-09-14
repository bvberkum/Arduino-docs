/**
  - Based on RF12demo
 */
#include <JeeLib.h>
#include <util/crc16.h>
#include <util/parity.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include "EmBencode.h"

// ATtiny's only support outbound serial @ 38400 baud, and no DataFlash logging

#if defined(__AVR_ATtiny84__) ||defined(__AVR_ATtiny44__)
#define SERIAL_BAUD 38400
#else
#define SERIAL_BAUD 57600

#define DATAFLASH 1 // check for presence of DataFlash memory on JeeLink
#define FLASH_MBIT  16  // support for various dataflash sizes: 4/8/16 Mbit

#define LED_PIN   9 // activity LED, comment out to disable

#endif

#define COLLECT 0x20 // collect mode, i.e. pass incoming without sending acks

char embuf [200];
EmBdecode decoder (embuf, sizeof embuf);

static unsigned long now () {
	// FIXME 49-day overflow
	return millis() / 1000;
}

static void activityLed (byte on) {
#ifdef LED_PIN
	pinMode(LED_PIN, OUTPUT);
	digitalWrite(LED_PIN, !on);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// RF12 configuration setup code

typedef struct {
	byte nodeId;
	byte group;
	char msg[RF12_EEPROM_SIZE-4];
	word crc;
} RF12Config;

static RF12Config config;

static char cmd;
static byte value, stack[RF12_MAXDATA+4], top, sendLen, dest, quiet;
static byte testbuf[RF12_MAXDATA], testCounter, useHex;

static void showNibble (byte nibble) {
	char c = '0' + (nibble & 0x0F);
	if (c > '9')
		c += 7;
	Serial.print(c);
}

static void showByte (byte value) {
	if (useHex) {
		showNibble(value >> 4);
		showNibble(value);
	} else {
		Serial.print((int) value);
	}
}

static void addCh (char* msg, char c) {
	byte n = strlen(msg);
	msg[n] = c;
}

static void addInt (char* msg, word v) {
	if (v >= 10)
		addInt(msg, v / 10);
	addCh(msg, '0' + v % 10);
}

static void saveConfig () {
	// set up a nice config string to be shown on startup
	memset(config.msg, 0, sizeof config.msg);
	strcpy(config.msg, " ");

	byte id = config.nodeId & 0x1F;
	addCh(config.msg, '@' + id);
	strcat(config.msg, " i");
	addInt(config.msg, id);
	if (config.nodeId & COLLECT)
		addCh(config.msg, '*');

	strcat(config.msg, " g");
	addInt(config.msg, config.group);

	strcat(config.msg, " @ ");
	static word bands[4] = { 315, 433, 868, 915 };
	word band = config.nodeId >> 6;
	addInt(config.msg, bands[band]);
	strcat(config.msg, " MHz ");

	config.crc = ~0;
	for (byte i = 0; i < sizeof config - 2; ++i)
		config.crc = _crc16_update(config.crc, ((byte*) &config)[i]);

	// save to EEPROM
	for (byte i = 0; i < sizeof config; ++i) {
		byte b = ((byte*) &config)[i];
		eeprom_write_byte(RF12_EEPROM_ADDR + i, b);
	}

	if (!rf12_config())
		Serial.println("config save failed");
}

static byte bandToFreq (byte band) {
	return band == 8 ? RF12_868MHZ : band == 9 ? RF12_915MHZ : RF12_433MHZ;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// OOK transmit code

// Turn transmitter on or off, but also apply asymmetric correction and account
// for 25 us SPI overhead to end up with the proper on-the-air pulse widths.
// With thanks to JGJ Veken for his help in getting these values right.
static void ookPulse(int on, int off) {
	rf12_onOff(1);
	delayMicroseconds(on + 150);
	rf12_onOff(0);
	delayMicroseconds(off - 200);
}

static void fs20sendBits(word data, byte bits) {
	if (bits == 8) {
		++bits;
		data = (data << 1) | parity_even_bit(data);
	}
	for (word mask = bit(bits-1); mask != 0; mask >>= 1) {
		int width = data & mask ? 600 : 400;
		ookPulse(width, width);
	}
}

static void fs20cmd(word house, byte addr, byte cmd) {
	byte sum = 6 + (house >> 8) + house + addr + cmd;
	for (byte i = 0; i < 3; ++i) {
		fs20sendBits(1, 13);
		fs20sendBits(house >> 8, 8);
		fs20sendBits(house, 8);
		fs20sendBits(addr, 8);
		fs20sendBits(cmd, 8);
		fs20sendBits(sum, 8);
		fs20sendBits(0, 1);
		delay(10);
	}
}

static void kakuSend(char addr, byte device, byte on) {
	int cmd = 0x600 | ((device - 1) << 4) | ((addr - 1) & 0xF);
	if (on)
		cmd |= 0x800;
	for (byte i = 0; i < 4; ++i) {
		for (byte bit = 0; bit < 12; ++bit) {
			ookPulse(375, 1125);
			int on = bitRead(cmd, bit) ? 1125 : 375;
			ookPulse(on, 1500 - on);
		}
		ookPulse(375, 375);
		delay(11); // approximate
	}
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

const char helpText1[] PROGMEM = 
"\n"
"Available commands:" "\n"
"  <nn> i     - set node ID (standard node ids are 1..30)" "\n"
"  <n> b      - set MHz band (4 = 433, 8 = 868, 9 = 915)" "\n"
"  <nnn> g    - set network group (RFM12 only allows 212, 0 = any)" "\n"
"  <n> c      - set collect mode (advanced, normally 0)" "\n"
"  t          - broadcast max-size test packet, request ack" "\n"
"  ...,<nn> a - send data packet to node <nn>, request ack" "\n"
"  ...,<nn> s - send data packet to node <nn>, no ack" "\n"
"  <n> l      - turn activity LED on PB1 on or off" "\n"
"  <n> q      - set quiet mode (1 = don't report bad packets)" "\n"
"  <n> x      - set reporting format (0 = decimal, 1 = hex)" "\n"
"  123 z      - total power down, needs a reset to start up again" "\n"
"Remote control commands:" "\n"
"  <hchi>,<hclo>,<addr>,<cmd> f     - FS20 command (868 MHz)" "\n"
"  <addr>,<dev>,<on> k              - KAKU command (433 MHz)" "\n"
;

static void showString (PGM_P s) {
	for (;;) {
		char c = pgm_read_byte(s++);
		if (c == 0)
			break;
		if (c == '\n')
			Serial.print('\r');
		Serial.print(c);
	}
}

static void showHelp () {
	showString(helpText1);
	Serial.println("Current configuration:");
	rf12_config();
}

static void handleInput (char c) {
	if ('0' <= c && c <= '9')
		value = 10 * value + c - '0';
	else if (c == ',') {
		if (top < sizeof stack)
			stack[top++] = value;
		value = 0;
	} else if ('a' <= c && c <='z') {
		Serial.print("> ");
		for (byte i = 0; i < top; ++i) {
			Serial.print((int) stack[i]);
			Serial.print(',');
		}
		Serial.print((int) value);
		Serial.println(c);
		switch (c) {
			default:
				showHelp();
				break;
			case 'i': // set node id
				config.nodeId = (config.nodeId & 0xE0) + (value & 0x1F);
				saveConfig();
				break;
			case 'b': // set band: 4 = 433, 8 = 868, 9 = 915
				if (value)
					config.nodeId = (bandToFreq(value) << 6) + (config.nodeId & 0x3F);
				saveConfig();
				break;
			case 'g': // set network group
				config.group = value;
				saveConfig();
				break;
			case 'c': // set collect mode (off = 0, on = 1)
				if (value)
					config.nodeId |= COLLECT;
				else
					config.nodeId &= ~COLLECT;
				saveConfig();
				break;
			case 't': // broadcast a maximum size test packet, request an ack
				cmd = 'a';
				sendLen = RF12_MAXDATA;
				dest = 0;
				for (byte i = 0; i < RF12_MAXDATA; ++i)
					testbuf[i] = i + testCounter;
				Serial.print("test ");
				Serial.println((int) testCounter); // first byte in test buffer
				++testCounter;
				break;
			case 'a': // send packet to node ID N, request an ack
			case 's': // send packet to node ID N, no ack
				cmd = c;
				sendLen = top;
				dest = value;
				memcpy(testbuf, stack, top);
				break;
			case 'l': // turn activity LED on or off
				activityLed(value);
				break;
			case 'f': // send FS20 command: <hchi>,<hclo>,<addr>,<cmd>f
				rf12_initialize(0, RF12_868MHZ);
				activityLed(1);
				fs20cmd(256 * stack[0] + stack[1], stack[2], value);
				activityLed(0);
				rf12_config(0); // restore normal packet listening mode
				break;
			case 'k': // send KAKU command: <addr>,<dev>,<on>k
				rf12_initialize(0, RF12_433MHZ);
				activityLed(1);
				kakuSend(stack[0], stack[1], value);
				activityLed(0);
				rf12_config(0); // restore normal packet listening mode
				break;
			case 'q': // turn quiet mode on or off (don't report bad packets)
				quiet = value;
				break;
			case 'z': // put the ATmega in ultra-low power mode (reset needed)
				if (value == 123) {
					delay(10);
					rf12_sleep(RF12_SLEEP);
					cli();
					Sleepy::powerDown();
				}
				break;
			case 'x': // set reporting mode to hex (1) or decimal (0)
				useHex = value;
				break;
		}
		value = top = 0;
		memset(stack, 0, sizeof stack);
	} else if (c == '>') {
		// special case, send to specific band and group, and don't echo cmd
		// input: band,group,node,header,data...
		stack[top++] = value;
		rf12_initialize(stack[2], bandToFreq(stack[0]), stack[1]);
		rf12_sendNow(stack[3], stack + 4, top - 4);
		rf12_sendWait(2);
		rf12_config(0); // restore original band, etc
		value = top = 0;
		memset(stack, 0, sizeof stack);
	} else if (' ' < c && c < 'A')
		showHelp();
}

void setup() {
	Serial.begin(SERIAL_BAUD);
	Serial.print("\n[RadioLink]");
	activityLed(0);

	if (rf12_config()) {
		config.nodeId = eeprom_read_byte(RF12_EEPROM_ADDR);
		config.group = eeprom_read_byte(RF12_EEPROM_ADDR + 1);
	} else {
		config.nodeId = 0x81;
		config.group = 0x5; 
		saveConfig();
	}

	showHelp();
}

typedef struct {
	//	byte light :8;     // light sensor: 0..255
	//	byte moved :1;  // motion detector: 0..1
	//	int rhum   :10;  // rhumdity: 0..100
	//	int temp   :10; // temperature: -500..+500 (tenths)
	int ctemp  :10; // atmega temperature: -500..+500 (tenths)
	//	byte lobat :1;  // supply voltage dropped under 3.1V: 0..1
} Payload;

Payload measurement;

void loop() {
	if (Serial.available())
		handleInput(Serial.read());

	if (rf12_recvDone()) {
		byte n = rf12_len;
		if (rf12_crc == 0)
			Serial.print("OK");
		else {
			if (quiet)
				return;
			Serial.print(" ?");
			if (n > 20) // print at most 20 bytes if crc is wrong
				n = 20;
		}
		if (useHex)
			Serial.print('X');
		if (config.group == 0) {
			Serial.print(" G");
			showByte(rf12_grp);
		}
		Serial.print(' ');
		showByte(rf12_hdr);
		for (byte i = 0; i < n; ++i) {
			if (!useHex)
				Serial.print(' ');
			showByte(rf12_data[i]);
		}

		if (rf12_crc == 0) {
			activityLed(1);

			if (RF12_WANTS_ACK) {
				Serial.println(" wants ack");
			}
			if (RF12_WANTS_ACK && (config.nodeId & COLLECT) == 0) {
				// should ack
				Serial.println(" -> ack");
				rf12_sendStart(RF12_ACK_REPLY, 0, 0);
				/*
				*/
			}
			/*
				No payload decoding here, but in Python.

			   measurement = *(Payload*) rf12_data;
			   Serial.println();
			   Serial.print((int) measurement.light);
			   Serial.print(' ');
			   Serial.print((int) measurement.rhum);
			   Serial.print(' ');
			   Serial.print((int) measurement.temp);
			   Serial.println((int) measurement.ctemp);
			 */

			activityLed(0);
		}
		Serial.println();
	}

	if (cmd && rf12_canSend()) {
		activityLed(1);

		Serial.print(" -> ");
		Serial.print((int) sendLen);
		Serial.println(" b");
		byte header = cmd == 'a' ? RF12_HDR_ACK : 0;
		if (dest)
			header |= RF12_HDR_DST | dest;
		rf12_sendStart(header, testbuf, sendLen);
		cmd = 0;

		activityLed(0);
	}
}

