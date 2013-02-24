/*

- Leds (green, yellow, red) at 5, 6 and 7 (port D on the m328p)
- Nordic nRF24L01+ 
- I2C with LCD
- I2C with RTC
- TWI for DHTxx

*/

#include <Wire.h>
#include <SPI.h>
#include <RF24.h>
#include <DHT.h>
#include <LiquidCrystalTmp_I2C.h>
//#include <JeeLib.h>


#define LED_GREEN 5
#define LED_YELLOW 6
#define LED_RED 7

/* DHTxx: Digital temperature/humidity (Adafruit library) */
#define DHTPIN A1
#define DHTTYPE DHT11   // DHT 11 
//#define DHTTYPE DHT22   // DHT 22  (AM2302)
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
/* DHTxx (jeelib) */
//DHTxx dht (A1); // connected to ADC1

DHT dht(DHTPIN, DHTTYPE);

/* DS1307: Real Time Clock over I2C */
#define DS1307_I2C_ADDRESS 0x68

const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };
typedef enum { role_ping_out = 1, role_pong_back } role_e;
const char* role_friendly_name[] = { "invalid", "Ping out", "Pong back"};

role_e role = role_pong_back;
RF24 radio(9,8);


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

	Serial.println("WaimanTinyRTC");
	Wire.begin();
	getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);

	if (second == 0 && minute == 0 && hour == 0 && month == 0 && year == 0) {
		Serial.println("Warning: time reset");
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
  radio.setPayloadSize(8);
  // XXX: should I open r/w pipes here?
  /* Start radio */
  radio.startListening();
  //radio.printDetails();//DEBUG
}

void radio_run()
{
  if (role == role_ping_out)
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

  //
  // Pong back role.  Receive each packet, dump it out, and send it back
  //

  if ( role == role_pong_back )
  {
    // if there is data ready
    if ( radio.available() )
    {
      // Dump the payloads until we've gotten everything
      unsigned long got_time;
      bool done = false;
      while (!done)
      {
        // Fetch the payload, and see if this was the last one.
        done = radio.read( &got_time, sizeof(unsigned long) );

        // Spew it
        printf("Got payload %lu...",got_time);

	// Delay just a little bit to let the other unit
	// make the transition to receiver
	delay(20);
      }

      // First, stop listening so we can talk
      radio.stopListening();

      // Send the final one back.
      radio.write( &got_time, sizeof(unsigned long) );
      printf("Sent response.\n\r");

      // Now, resume listening so we catch the next packets.
      radio.startListening();
    }
  }

  //
  // Change roles
  //

  if ( Serial.available() )
  {
    char c = toupper(Serial.read());
    if ( c == 'T' && role == role_pong_back )
    {
      printf("*** CHANGING TO TRANSMIT ROLE -- PRESS 'R' TO SWITCH BACK\n\r");

      // Become the primary transmitter (ping out)
      role = role_ping_out;
      radio.openWritingPipe(pipes[0]);
      radio.openReadingPipe(1,pipes[1]);
    }
    else if ( c == 'R' && role == role_ping_out )
    {
      printf("*** CHANGING TO RECEIVE ROLE -- PRESS 'T' TO SWITCH BACK\n\r");
      
      // Become the primary receiver (pong back)
      role = role_pong_back;
      radio.openWritingPipe(pipes[1]);
      radio.openReadingPipe(1,pipes[0]);
    }
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
	Serial.println("Failed to read from DHT");
	lcd.println("DHT error");
  } else {
	Serial.print("Humidity: "); 
	Serial.print(h);
	Serial.println(" %");

	Serial.print("Temperature: "); 
	Serial.print(t);
	Serial.println(" *C");

	lcd.print(h);
	lcd.print("% ");
	lcd.print(t);
	lcd.print("*C ");
  }
}

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

void setup(void)
{
  Serial.begin(57600);
  Serial.println("Cassette328P");

  pinMode( LED_RED, OUTPUT );
  pinMode( LED_YELLOW, OUTPUT );
  pinMode( LED_GREEN, OUTPUT );
	
  i2c_lcd_init();
  lcd.print("Cassette328P");
  radio_init();
  rtc_init();
  dht.begin();
}

void loop(void)
{
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

