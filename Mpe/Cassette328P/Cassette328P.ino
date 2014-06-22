/*
Cassette328P

An atmega328p TQFP chip with various peripherals:

- Leds (green, yellow, red) at 5, 6 and 7 (PD on the m328p, 5 and 6 have PWM)
- Nordic nRF24L01+ is at ISP plus D8 and D9 (PB0 and PB1), RF24 IRW is not used?
- LCD at hardware I2C port with address 0x20
- RTC at I2C 0x68
- DHT11 at A1 (PC1)

* Voltage is 3.3v and speed 16Mhz, external ceramic resonator.
  Hope this will keep working as such.
* Arduino library is used while I'm learning from chip datasheets,
	perhaps where needed there is time for more efficient and low-power
	implementation.

Plans
  - Hookup a low-pass filter and play with PWM, sine generation at D10 (m328p: PB2).

Research
  - Perhaps enable DEBUG/SERIAL modes internally, wonder what it does with the sketch size, no experience there.
    Could use free ADC pin to measure level for verbosity, grey encoding would be cool witht he proper visuals.
  - Optionally enable LCD when present, also, make control panel to go with display, using attiny85 as I2C slave.
    Some buttons for menu. Perhaps touchscreen controller, remote reset.

Free pins
  - D3  PD3 PWM
  - D4  PD4
  - D10 PB2 PWM
  - D20 A6 ADC6
*/

#include <avr/io.h>
#include <avr/sleep.h>
#include <util/atomic.h>

#include <DHT.h> // Adafruit DHT
#include <JeeLib.h>
#include <Wire.h>
#include <SPI.h>
#include <EEPROM.h>
//include <RF24.h>
//include <LiquidCrystalTmp_I2C.h>
#include <SimpleFIFO.h> //code.google.com/p/alexanderbrevig/downloads/list

#include <printf.h>


/** Globals and sketch configuration  */
#define DEBUG           1
#define SERIAL          1
							
#define LED_RED         5
#define LED_YELLOW      6
#define LED_GREEN       7

#define REPORT_EVERY    5   // report every N measurement cycles
#define SMOOTH          5   // smoothing factor used for running averages
#define MEASURE_PERIOD  50  // how often to measure, in tenths of seconds
#define DEBUG_MEASURE   1
							
#define RETRY_PERIOD    10  // how soon to retry if ACK didn't come in
#define RETRY_LIMIT     5   // maximum number of times to retry
#define ACK_TIME        10  // number of milliseconds to wait for an ack
							
#define RADIO_SYNC_MODE 2
							
#define _MEM        1
#define _BAT        1

//#define LDR_PORT    0   // defined if LDR is connected to a port's AIO pin
#define _DHT        1   // defined if _DHTxx data is connected to a DIO pin
#define DHT_PIN     14
#define _RTC        0
//#define LCD
#define _RF24       0


static String sketch = "Cassette328P";
static String node = "";
static String vesion = "0";

/**
 * See http://www.wormfood.net/avrbaudcalc.php for baurdates. Here used 38400 
 * gives an UBRR of 25 with 0.2% error rate, according to that sheet.
 */
//#define USART_BAUDRATE 38400
//#define BAUD_PRESCALE ((( F_CPU / ( USART_BAUDRATE * 16UL ))) - 1 )

#define CONFIG_VERSION "ls1"
#define CONFIG_START 32

struct Config {
	char version[4]; bool saved;
	bool radio; bool display; bool dht; bool rtc;
	uint64_t rf24id;
	uint64_t rf24routes[1];
} static_config = {
	/* default values */
	CONFIG_VERSION, false,
	false, true, true, true,
	0xF0F0F0F0D2LL, { 0xF0F0F0F0E1LL }
};

Config config;

static byte node;       // node ID used for this unit

enum { ANNOUNCE, DISCOVERY, MEASURE, REPORT, TASK_END };
static word schedbuf[TASK_END];
Scheduler scheduler (schedbuf, TASK_END);
static byte reportCount;    // count up until next report, i.e. packet send

/* Allow invokation of commands with up to 7 args */
SimpleFIFO<uint8_t,8> cmdseq; //store 10 ints

typedef enum { 
	cmd_wait_for_command = 0x01,  
	cmd_stand_by = 0x20,
	cmd_report_all = 0x10,
	cmd_read_time = 0x11,
	cmd_service_mode = 0x21, // '!' in minicom
	cmd_print_rf12info = 0x72, // 'r'
	cmd_print_settings  = 0x40, // '@'
	cmd_reset_settings = 0x23, // '#'
	cmd_help = 0x3F, // '?'
	cmd_report= 0x24, // '$'
	cmd_4 = 0x25, // '%'
	cmd_5 = 0x26, // '&'
	cmd_6 = 0x28, // '('
	cmd_7 = 0x29, // ')'
} Command;

// milis in serviceMode
int service_mode = 0;

/* Support for Sleepy::loseSomeTime() */
//EMPTY_INTERRUPT(WDT_vect);

// has to be defined because we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

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

/* Data reported by this sketch */
struct {
#if LDR_PORT
	byte light      :8;     // light sensor: 0..255
#endif
#if PIR_PORT
	byte moved      :1;  // motion detector: 0..1
#endif
#if _DHT
	int rhum        :7;  // rhumdity: 0..100 (4 or 5% resolution?)
	int temp        :10; // temperature: -500..+500 (tenths, .5 resolution)
#endif
	int ctemp       :10; // atmega temperature: -500..+500 (tenths)
#if _MEM
	int memfree     :16;
#endif
#if _BAT
	byte lobat      :1;  // supply voltage dropped under 3.1V: 0..1
#endif
//#if _RTC
// XXX: datetime?
//#endif
} payload;

#if LDR_PORT
Port ldr (LDR_PORT);
#endif

#if _DHT
/* DHTxx: Digital temperature/humidity (Adafruit) */
#define DHTTYPE   DHT11   // DHT 11 / DHT 22 (AM2302) / DHT 21 (AM2301)
DHT dht(DHT_PIN, DHTTYPE);
/* DHTxx (jeelib) */
//DHTxx dht (A1); // connected to ADC1
#endif

#if _RTC
/* DS1307: Real Time Clock over I2C */
#define DS1307_I2C_ADDRESS 0x68
#endif

int freeRam () {
	extern int __heap_start, *__brkval; 
	int v; 
	return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

double internalTemp(void)
{
	unsigned int wADC;
	double t;

	// The internal temperature has to be used
	// with the internal reference of 1.1V.
	// Channel 8 can not be selected with
	// the analogRead function yet.

	// Set the internal reference and mux.
	ADMUX = (_BV(REFS1) | _BV(REFS0) | _BV(MUX3));
	ADCSRA |= _BV(ADEN);  // enable the ADC

	delay(20);            // wait for voltages to become stable.

	ADCSRA |= _BV(ADSC);  // Start the ADC

	// Detect end-of-conversion
	while (bit_is_set(ADCSRA,ADSC));

	// Reading register "ADCW" takes care of how to read ADCL and ADCH.
	wADC = ADCW;

	// The offset of 324.31 could be wrong. It is just an indication.
	t = (wADC - 311 ) / 1.22;

	// The returned temperature is in degrees Celcius.
	return (t);
}

// utility code to perform simple smoothing as a running average
static int smoothedAverage(int prev, int next, byte firstTime =0) {
	if (firstTime)
		return next;
	return ((SMOOTH - 1) * prev + next + SMOOTH / 2) / SMOOTH;
}


#if _RF24
/* nRF24L01+: 2.4Ghz radio. Addresses are 40 bit, ie. < 0x10000000000L */
RF24 radio(12,13); /* CE, CS */

void radio_init()
{
	radio.begin();
	radio.setRetries(15, 15); /* delay, number */
	//radio.setPayloadSize(8);
	radio.powerDown();
}

void radio_start()
{
#if DEBUG
	radio.printDetails();
#endif
	/* Start radio */
  radio.openWritingPipe(config.rf24routes[0]);
  radio.openReadingPipe(1, config.rf24id);
  radio.startListening();
}

void radio_run()
{
	if (role == role_logger)
	{
		if (radio.available()) {
			unsigned long got_time;
			bool done = false;
			while (!done)
			{
				// Fetch the payload, and see if this was the last one.
				done = radio.read( &got_time, sizeof(unsigned long) );
				// Spew it
				printf("Got payload %lu...", got_time);
				// Delay just a little bit to let the other unit
				// make the transition to receiver
				delay(20);

				/** XXX: echo time during debugging */
				// First, stop listening so we can talk
				radio.stopListening();

				// Send the final one back.
				radio.write( &got_time, sizeof(unsigned long) );
				printf("Sent response.\n\r");

				// Now, resume listening so we catch the next packets.
				radio.startListening();
			}
		}
	}
	if (role == role_reporter)
	{
		// First, stop listening so we can talk.
		radio.stopListening();
		// Take the time, and send it.  This will block until complete
		unsigned long time = millis();
		printf("Now sending %lu...",time);
		bool ok = radio.write( &time, sizeof(unsigned long) );

		if (ok)
			printf("ok...");
		else
			printf("failed.\n\r");

		// Now, continue listening
		radio.startListening();

		/* XXX: while debugging wait for echo and display time delay */

		// Wait here until we get a response, or timeout (250ms)
		unsigned long started_waiting_at = millis();
		bool timeout = false;
		while ( ! radio.available() && ! timeout )
			if (millis() - started_waiting_at > 200 )
				timeout = true;

		// Describe the results
		if ( timeout )
		{
			printf("Failed, response timed out.\n\r");
		}
		else
		{
			// Grab the response, compare, and send to debugging spew
			unsigned long got_time;
			radio.read( &got_time, sizeof(unsigned long) );

			// Spew it
			printf("Got response %lu, round-trip delay: %lu\n\r",got_time,millis()-got_time);
		}

		// Try again 1s later
		delay(1000);
	}
}

void saveRF24Config(Config &c) {
	for (unsigned int t=0; t<sizeof(c); t++)
	{
		EEPROM.write(CONFIG_START + t, *((char*)&c + t));
	}
}
#endif

/** Generic routines */

static void serialFlush () {
#if SERIAL
#if ARDUINO >= 100
	Serial.flush();
#endif
	delay(2); // make sure tx buf is empty before going back to sleep
#endif
}

void blink(int led, int count, int length, int length_off=0) {
  for (int i=0;i<count;i++) {
    digitalWrite (led, HIGH);
    delay(length);
    digitalWrite (led, LOW);
    delay(length);
    (length_off > 0) ? delay(length_off) : delay(length);
  }
}

#if defined(ARDUINO) && ARDUINO >= 100
#define printByte(args)  write(args);
#else
#define printByte(args)  print(args,BYTE);
#endif

#if LCD
//uint8_t bell[8]  = {0x4,0xe,0xe,0xe,0x1f,0x0,0x4};

/* I2C LCD 1602 parameters */
#define I2C_ADDR    0x20  // Define I2C Address where the PCF8574A is

#define BACKLIGHT_PIN     7
#define En_pin  4
#define Rw_pin  5
#define Rs_pin  6
#define D4_pin  0
#define D5_pin  1
#define D6_pin  2
#define D7_pin  3

#define  LED_OFF  0
#define  LED_ON  1


LiquidCrystalTmp_I2C  lcd(I2C_ADDR,En_pin,Rw_pin,Rs_pin,D4_pin,D5_pin,D6_pin,D7_pin);

void i2c_lcd_init()
{
	lcd.begin(16,2);
	lcd.setBacklightPin(BACKLIGHT_PIN, NEGATIVE);
	//lcd.noDisplay();
	//lcd.noBacklight();
	lcd.off();
	//lcd.setBacklight(LED_ON);
	lcd.leftToRight();	
	lcd.noBlink();
	lcd.noCursor();
//lcd.createChar(0, bell);
}

void i2c_lcd_start()
{
	lcd.on();
	lcd.home();
}
#endif

// Convert normal decimal numbers to binary coded decimal
byte decToBcd(byte val)
{
	return ( (val/10*16) + (val%10) );
}

// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val)
{
	return ( (val/16*10) + (val%16) );
}

#if _RTC
// Stops the DS1307, but it has the side effect of setting seconds to 0
// Probably only want to use this for testing
/*void stopDs1307()
  {
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.send(0);
  Wire.send(0x80);
  Wire.endTransmission();
  }*/

// 1) Sets the date and time on the ds1307
// 2) Starts the clock
// 3) Sets hour mode to 24 hour clock
// Assumes you're passing in valid numbers
void setDateDs1307(
		byte second,        // 0-59
		byte minute,        // 0-59
		byte hour,          // 1-23
		byte dayOfWeek,     // 1-7
		byte dayOfMonth,    // 1-28/29/30/31
		byte month,         // 1-12
		byte year)          // 0-99
{
	Wire.beginTransmission(DS1307_I2C_ADDRESS);
	Wire.write(0);
	Wire.write(decToBcd(second));    // 0 to bit 7 starts the clock
	Wire.write(decToBcd(minute));
	Wire.write(decToBcd(hour));      // If you want 12 hour am/pm you need to set
	// bit 6 (also need to change readDateDs1307)
	Wire.write(decToBcd(dayOfWeek));
	Wire.write(decToBcd(dayOfMonth));
	Wire.write(decToBcd(month));
	Wire.write(decToBcd(year));
	Wire.endTransmission();
}

// Gets the date and time from the ds1307
void getDateDs1307(
		byte *second,
		byte *minute,
		byte *hour,
		byte *dayOfWeek,
		byte *dayOfMonth,
		byte *month,
		byte *year)
{
	// Reset the register pointer
	Wire.beginTransmission(DS1307_I2C_ADDRESS);
	Wire.write(0);
	Wire.endTransmission();

	Wire.requestFrom(DS1307_I2C_ADDRESS, 7);

	// A few of these need masks because certain bits are control bits
	*second     = bcdToDec(Wire.read() & 0x7f);
	*minute     = bcdToDec(Wire.read());
	*hour       = bcdToDec(Wire.read() & 0x3f);  // Need to change this if 12 hour am/pm
	*dayOfWeek  = bcdToDec(Wire.read());
	*dayOfMonth = bcdToDec(Wire.read());
	*month      = bcdToDec(Wire.read());
	*year       = bcdToDec(Wire.read());
}

void rtc_init()
{
	byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;

#if SERIAL || DEBUG
	Serial.println("WaimanTinyRTC");
#endif
	Wire.begin();
	getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);

	if (second == 0 && minute == 0 && hour == 0 && month == 0 && year == 0) {
#if SERIAL || DEBUG
		Serial.println("Warning: time reset");
#endif
		second = 0;
		minute = 31;
		hour = 5;
		dayOfWeek = 4;
		dayOfMonth = 18;
		month = 5;
		year = 12;
		setDateDs1307(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
	}
}

void rtc_run(void)
{
	byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
	getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);

#if LCD
	if (config.display) {
		lcd.setCursor(0, 1);
		lcd.print(year, DEC);
		lcd.print("-");
		lcd.print(month, DEC);
		lcd.print("-");
		lcd.print(dayOfMonth, DEC);
		lcd.print(" ");
		lcd.print(hour, DEC);
		lcd.print(":");
		lcd.print(minute, DEC);
		lcd.print(":");
		lcd.print(second, DEC);
	}
#endif
}
#endif

uint8_t cmd;

bool rx_overflow = false;

/* I guess this hooks into the RX ISR's. The serial buffer is 1 byte, which can
 * stay in the FIFO as long as the second byte is being received. */
void serialEvent()
{
	while (Serial.available()) {
		uint8_t b = Serial.read();
		if (!cmdseq.enqueue(b)) {
			rx_overflow = true;
		}
	}
}

/**
 * Using this to wake up chip by hardware jumper.
 * Need to look for an interrupt.
 */
int serviceMode(void) 
{
	pinMode(LED_RED, INPUT);
	digitalWrite( LED_RED, LOW);
	int d = digitalRead( LED_RED ) ;
	bool active = d == HIGH;
	if (!active) {
		pinMode( LED_RED, OUTPUT );
	}
	return active;
}

/* Configuration is persisted to EEPROM */
static void doConfig(void)
{
}

bool loadConfig(Config &c) 
{
	if (EEPROM.read(CONFIG_START + 0) == CONFIG_VERSION[0] &&
			EEPROM.read(CONFIG_START + 1) == CONFIG_VERSION[1] &&
			EEPROM.read(CONFIG_START + 2) == CONFIG_VERSION[2])
	{
		for (unsigned int t=0; t<sizeof(c); t++)
		{
			*((char*)&c + t) = EEPROM.read(CONFIG_START + t);
		}
		return true;
	} else {
		return false;
#if DEBUG
		Serial.println("No valid data in eeprom");
#endif
	}
}


void setup_peripherals()
{
#if _RTC
	rtc_init();
#endif
#if _RF24
	radio_init();
#endif
#if _RF12
	//radio_init();
#endif
#if _LCD
	i2c_lcd_init();
	i2c_lcd_start();
	lcd.print(F("Cassette328P"));
#endif
#if _DHT
 	dht.begin();
#if DEBUG 
	Serial.println("Initialized DHT");
	float t = dht.readTemperature();
	Serial.println(t);
#endif
#endif
	blink(LED_GREEN, 4, 100);
}

static bool doAnnounce() 
{
}

static void doMeasure() 
{
	byte firstTime = payload.ctemp == 0; // special case to init running avg

	int ctemp = internalTemp();
	payload.ctemp = smoothedAverage(payload.ctemp, ctemp, firstTime);
#if SERIAL && DEBUG_MEASURE
	Serial.print("AVR T new/avg ");
	Serial.print(ctemp);
	Serial.print(' ');
	Serial.println(payload.ctemp);
#endif

#if _BAT
	payload.lobat = rf12_lowbat();
#endif
#if SERIAL && DEBUG_MEASURE
	if (payload.lobat) {
		Serial.println("Low battery");
	}
#endif

#if _DHT
	float h = dht.readHumidity();
	float t = dht.readTemperature();
	if (isnan(h)) {
#if SERIAL | DEBUG
		Serial.println(F("Failed to read DHT11 humidity"));
#endif
	} else {
		int rh = h * 10;
		payload.rhum = smoothedAverage(payload.rhum, rh, firstTime);
#if SERIAL && DEBUG_MEASURE
		Serial.print(F("DHT RH new/avg "));
		Serial.print(rh);
		Serial.print(' ');
		Serial.println(payload.rhum);
#endif
	}
	if (isnan(t)) {
#if SERIAL | DEBUG
		Serial.println(F("Failed to read DHT11 temperature"));
#endif
	} else {
		payload.temp = smoothedAverage(payload.temp, t * 10, firstTime);
#if SERIAL && DEBUG_MEASURE
		Serial.print(F("DHT T new/avg "));
		Serial.print(t);
		Serial.print(' ');
		Serial.println(payload.temp);
#endif
	}
#endif // _DHT

#if LDR_PORT
	ldr.digiWrite2(1);  // enable AIO pull-up
	byte light = ~ ldr.anaRead() >> 2;
	ldr.digiWrite2(0);  // disable pull-up to reduce current draw
	payload.light = smoothedAverage(payload.light, light, firstTime);
#if SERIAL && DEBUG_MEASURE
	Serial.print(F("LDR new/avg "));
	Serial.print(light);
	Serial.print(' ');
	Serial.println(payload.light);
#endif
#endif

#if _MEM
	payload.memfree = freeRam();
#if SERIAL && DEBUG_MEASURE
	Serial.print("MEM free ");
	Serial.println(payload.memfree);
#endif
#endif

	serialFlush();
}

// periodic report, i.e. send out a packet and optionally report on serial port
static void doReport(void)
{
	rf12_sleep(RF12_WAKEUP);
	rf12_sendNow(0, &payload, sizeof payload);
	rf12_sendWait(RADIO_SYNC_MODE);
	rf12_sleep(RF12_SLEEP);
#if RTC
	blink(LED_YELLOW, 2, 25);
	rtc_run();
#endif
#if _RF24
	blink(LED_YELLOW, 2, 25);
	radio_run();
#endif
#if SERIAL
	Serial.print(node);
	Serial.print(" ");
#if LDR_PORT
	Serial.print((int) payload.light);
	Serial.print(' ');
#endif
//  Serial.print((int) payload.moved);
//  Serial.print(' ');
#if _DHT
	Serial.print((int) payload.rhum);
	Serial.print(' ');
	Serial.print((int) payload.temp);
	Serial.print(' ');
#endif
	Serial.print((int) payload.ctemp);
	Serial.print(' ');
#if _MEM
	Serial.print((int) payload.memfree);
	Serial.print(' ');
#endif
	Serial.print((int) payload.lobat);
	Serial.println();
	serialFlush();
#endif//SERIAL
}

static void runCommand()
{
/*
	blink(LED_YELLOW, 20, 1 );
	uint8_t cmd = cmdseq.dequeue();
	switch (cmd) {

		case cmd_reset_settings:
			saveRF24Config(static_config);
			config = static_config;
			setup_peripherals();
			break;

		case cmd_print_settings:
//					Serial.print("rf24id=");
//					Serial.println(config.rf24id, HEX);
//					Serial.print("rf24routes[0]=");
//					Serial.println(config.rf24routes[0], HEX);
			Serial.print("display=");
			Serial.println(config.display, BIN);
			Serial.print("radio=");
			Serial.println(config.radio, BIN);
			Serial.print("dht=");
			Serial.println(config.dht, BIN);
			Serial.print("rtc=");
			Serial.println(config.rtc, BIN);
			break;

		case cmd_help:
			Serial.println("Commands:");
			Serial.println(" 0x21 '!'  Enter service mode");
			Serial.println(" 0x23 '#'  Reset config");
			Serial.println(" 0x24 '$'  Do report");
			Serial.println(" 0x40 '@'  Print config");
			Serial.println(" 0x72 'r'  Radio info");
			break;

		case cmd_report:
			doReport();
			break;

		case cmd_print_rf12info:
			if (config.radio) {
				radio.printDetails();
			}
			Serial.println("r ACK");
			break;

		case cmd_service_mode:
			//service_mode = millis();
			break;

		default:
			Serial.print("0x");
			Serial.print(cmd, HEX);
			Serial.println("?");
			blink(LED_YELLOW, 5, 25 );
	}
	cmdseq.flush();
	Serial.println(".");
	blink(LED_GREEN, 2, 75 );
*/
}

/* Main */

void setup(void)
{
#if SERIAL
	Serial.begin(57600);
	Serial.println();
	Serial.print("[");
	Serial.print(sketch);
	Serial.print(".");
	Serial.print(version);
	Serial.print("]");
#if DEBUG
	Serial.print(F("Free RAM: "));
	Serial.println(freeRam());
	byte rf12_show = 1;
#else
	byte rf12_show = 0;
#endif
	//node= 4;
	rf12_initialize(node, RF12_868MHZ, 5);
	rf12_control(0x9485); // Receiver Control: Max LNA, 200kHz RX bw, DRSSI -73dB
	rf12_control(0x9850); // Transmission Control: Pos, 90kHz
	rf12_control(0xC606); // Data Rate 6
	rf12_sleep(RF12_SLEEP); // power down

	/* Read config from memory, or initialize with defaults */
//	if (!loadConfig(config)) 
//	{
//		saveRF24Config(static_config);
//		config = static_config;
//	}
	pinMode( LED_GREEN, OUTPUT );
	pinMode( LED_YELLOW, OUTPUT );
	pinMode( LED_RED, OUTPUT );

	//blink(LED_YELLOW, 1, 100);
	digitalWrite( LED_RED, HIGH );
	digitalWrite( LED_YELLOW, HIGH );


	digitalWrite( LED_RED, LOW );

	setup_peripherals();

	digitalWrite( LED_YELLOW, LOW );

	serialFlush();
	reportCount = REPORT_EVERY;     // report right away for easy debugging
	scheduler.timer(MEASURE, 0);    // start the measurement loop going
}

int col = 0;

void loop(void)
{
#if DEBUG
	col++;
	Serial.print('.');
	if (col > 79) {
		col = 0;
		Serial.println();
	}
	serialFlush();
#endif

	if (rx_overflow) {
		blink(LED_RED, 1, 75 );
		rx_overflow = false;
	}
	//if (cmdseq.count() > 0) {
	//	runCommand();
	//}
	/* Do nothing but count uptime while LED_RED is bypassed to HIGH */
	//if (serviceMode()) {
	//	if (service_mode == NULL) {
	//		Serial.println("servicemode");
	//	}
	//	service_mode = millis();
	//	if (service_mode % 100) {
	//		blink(LED_YELLOW, 1, 150);
	//	}
	//	delay(20);
	//	return;
	//} else {
	//	service_mode = NULL;
	//}
	switch (scheduler.pollWaiting()) {

		case DISCOVERY:
			break;

		case ANNOUNCE:
			break;

		case MEASURE:
			// reschedule these measurements periodically
			scheduler.timer(MEASURE, MEASURE_PERIOD);
			doMeasure();
			// every so often, a report needs to be sent out
			if (++reportCount >= REPORT_EVERY) {
				reportCount = 0;
				scheduler.timer(REPORT, 0);
			}
			break;

		case REPORT:
			doReport();
			break;

	}
}
