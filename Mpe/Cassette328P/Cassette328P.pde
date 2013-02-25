/*

- Leds (green, yellow, red) at 5, 6 and 7 (PD on the m328p, 5 and 6 have PWM)
- Nordic nRF24L01+ is at ISP plus D8 and D9 (PB0 and PB1), RF24 reset is not used?
- LCD at hardware I2C port with address 0x20
- RTC at I2C 0x68
- DHT11 at A1 (PC1)

Work in progress
  - Basic peripherals working through I2C, ISP.
  - Need to set up wireless mesh and test range.
  - Need to rewrite using interrupts, implement serial protocols. And look out for power use.

Plans
  - Hookup a low-pass filter and play with PWM, sine generation at D10 (m328p: PB2).

Research
  - Can DHT be used in higher precision mode? this looks like 5%RH/2*C which is a bit course.
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

#include <Wire.h>
#include <SPI.h>
#include <RF24.h>
#include <DHT.h>
#include <LiquidCrystalTmp_I2C.h>
//#include <JeeLib.h>


/** Globals and sketch configuration  */
// redifined?#define SERIAL      0   // set to 1 to also report readings on the serial port
#define DEBUG       1   // set to 1 to report each loop() run and trigger on serial, and enable serial protocol

#define LED_GREEN   5
#define LED_YELLOW  6
#define LED_RED     7

#define LDR_PORT    0   // defined if LDR is connected to a port's AIO pin

static byte myNodeID;     // node ID used for this unit
static byte readCount;    // count up until next report, i.e. packet send

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
//    byte light;     // light sensor: 0..255
//    byte moved :1;  // motion detector: 0..1
    byte humi  :7;  // humidity: 0..100
    int temp   :10; // temperature: -500..+500 (tenths)
//    byte lobat :1;  // supply voltage dropped under 3.1V: 0..1
// XXX: datetime	
} payload;

#if LDR_PORT
Port ldr (LDR_PORT);
#endif

/* DHTxx: Digital temperature/humidity (Adafruit) */
#define DHTPIN A1
#define DHTTYPE DHT11   // DHT 11 / DHT 22 (AM2302) / DHT 21 (AM2301)
DHT dht(DHTPIN, DHTTYPE);
/* DHTxx (jeelib) */
//DHTxx dht (A1); // connected to ADC1

/* DS1307: Real Time Clock over I2C */
#define DS1307_I2C_ADDRESS 0x68

/* nRF24L01+: 2.4Ghz radio  */
const uint64_t pipes[2] = { 
  0xF0F0F0F0E1LL, 
  0xF0F0F0F0D2LL 
};
RF24 radio(9,8);

static void mode_reporter(void)
{
  role = role_reporter;
  radio.stopListening();
  radio.openWritingPipe(pipes[1]);
  radio.openReadingPipe(1, pipes[0]);
  radio.startListening();
}

static void mode_logger(void)
{
  role = role_logger;
  radio.stopListening();
  radio.openWritingPipe(pipes[0]);
  radio.openReadingPipe(1, pipes[1]);
  radio.startListening();
}


/** Generic routines */

//#if SERIAL || DEBUG
static void serialFlush () {
    #if ARDUINO >= 100
        Serial.flush();
    #endif  
    delay(2); // make sure tx buf is empty before going back to sleep
}
//#endif

void blink(int led, int count, int length) {
  for (int i=0;i<count;i++) {
    digitalWrite (led, HIGH);
    delay(length);
    digitalWrite (led, LOW);
    delay(length);
  }
}

#if defined(ARDUINO) && ARDUINO >= 100
#define printByte(args)  write(args);
#else
#define printByte(args)  print(args,BYTE);
#endif

uint8_t bell[8]  = {0x4,0xe,0xe,0xe,0x1f,0x0,0x4};
uint8_t note[8]  = {0x2,0x3,0x2,0xe,0x1e,0xc,0x0};
uint8_t clock[8] = {0x0,0xe,0x15,0x17,0x11,0xe,0x0};
uint8_t heart[8] = {0x0,0xa,0x1f,0x1f,0xe,0x4,0x0};
uint8_t duck[8]  = {0x0,0xc,0x1d,0xf,0xf,0x6,0x0};
uint8_t check[8] = {0x0,0x1,0x3,0x16,0x1c,0x8,0x0};
uint8_t cross[8] = {0x0,0x1b,0xe,0x4,0xe,0x1b,0x0};
uint8_t retarrow[8] = {	0x1,0x1,0x5,0x9,0x1f,0x8,0x4};

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
	lcd.setBacklightPin(BACKLIGHT_PIN,NEGATIVE);
	lcd.setBacklight(LED_ON);

	lcd.createChar(0, bell);
	lcd.createChar(1, note);
	lcd.createChar(2, clock);
	lcd.createChar(3, heart);
	lcd.createChar(4, duck);
	lcd.createChar(5, check);
	lcd.createChar(6, cross);
	lcd.createChar(7, retarrow);
	lcd.home();
}

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

//#if SERIAL || DEBUG
  Serial.println("WaimanTinyRTC");
//#endif
  Wire.begin();
  getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);

  if (second == 0 && minute == 0 && hour == 0 && month == 0 && year == 0) {
//#if SERIAL || DEBUG
	Serial.println("Warning: time reset");
//#endif
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

void radio_init()
{
  /* Setup and configure rf radio */
  radio.begin();
  radio.setRetries(15,15); /* delay, number */
  //radio.setPayloadSize(8);
  // XXX: should I open r/w pipes here?
  /* Start radio */
  radio.openReadingPipe(1,pipes[1]);
  radio.startListening();
//#if SERIAL || DEBUG
//  radio.printDetails();//DEBUG
//#endif
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

void dht_run(void)
{
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // check if returns are valid, if they are NaN (not a number) then something went wrong!
  lcd.setCursor(0,0);
  if (isnan(t) || isnan(h)) {
//#if SERIAL || DEBUG
	Serial.println("Failed to read from DHT");
//#endif
	lcd.println("DHT error");
  } else {
//#if SERIAL || DEBUG
	Serial.print("Humidity: "); 
	Serial.print(h);
	Serial.println(" %RH");

	Serial.print("Temperature: "); 
	Serial.print(t);
	Serial.println(" *C");
//#endif
	lcd.print(h);
	lcd.print("%RH ");
	lcd.print(t);
	lcd.print((char)223);
	lcd.print("C ");
  }
}
// spend a little time in power down mode while the SHT11 does a measurement
//static void dht_delay() {
//    Sleepy::loseSomeTime(32); // must wait at least 20 ms
//}


void rtc_run(void)
{
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);

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


/* Main */

void setup(void)
{
//#if SERIAL || DEBUG
  Serial.begin(57600);
  Serial.print("\nCassette328P");
  //myNodeID = rf24_config();
  serialFlush();
  //#else
  //myNodeID = rf24_config(0); // don't report info on the serial port
//#endif

  pinMode( LED_RED, OUTPUT );
  pinMode( LED_YELLOW, OUTPUT );
  pinMode( LED_GREEN, OUTPUT );
	
  blink(LED_RED,1,75);

  radio_init();
  mode_reporter();
  i2c_lcd_init();
  lcd.print("Cassette328P");
  rtc_init();
  dht.begin();

  blink(LED_GREEN,4,75);
}

void loop(void)
{
  blink(LED_RED,2,75);

//#if SERIAL || DEBUG
  Serial.print('.');
  serialFlush();
  while ( Serial.available() > 0 ) {
  	Serial.print(Serial.read());
  	continue; // XXX

	char c = toupper(Serial.read());
	if ( c == 'R' && role == role_logger) {
	  //mode_reporter();
	} else if ( c == 'L' && role == role_reporter) {
	  //mode_logger();
	} else {
	  Serial.print(c);
	  Serial.println("?");
	}
  }
  Serial.println();
//#endif

  blink(LED_YELLOW,2,75);
  rtc_run();

  blink(LED_GREEN,2,75);
  radio_run();

  blink(LED_YELLOW,2,75);
  dht_run();

  blink(LED_GREEN,1,75);
  delay(500);
}

// vim:cin:ai:noet:sts=2 sw=2 ft=cpp
