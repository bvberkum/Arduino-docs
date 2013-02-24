// 2013-01-19
//   This is working with JeeNode I2C port. I found pullups unecessary while only
//   the LCD is plugged in using jumper wires.
//   Incompatible with JeeLib as interface is not there yet

//DFRobot.com
//Compatible with the Arduino IDE 1.0
//Library version:1.1
#include <Wire.h>
#include <LiquidCrystalTmp_I2C.h>
//#include <JeeLib.h> // Cannot use JeeLib ports yet while my I2C port expander
// uses different LCD hookup.

// Renamed the LiquidCrystal_I2C library version that does allow to configure
// this to LiquidCrystalTmp_I2C.


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

void i2c_lcd_debug_displayKeyCodes(void) {
	uint8_t i = 0;
	while (1) {
		lcd.clear();
		lcd.print("Codes 0x"); lcd.print(i, HEX);
		lcd.print("-0x"); lcd.print(i+16, HEX);
		lcd.setCursor(0, 1);
		for (int j=0; j<16; j++) {
			lcd.printByte(i+j);
		}
		i+=16;

		delay(4000);
	}
}

// Main routines

void setup()
{
	Serial.begin(57600);
	Serial.println("I2C_LCD_CustomChars_mjkdz");
	i2c_lcd_init();

	lcd.print("Hello world...");
	lcd.setCursor(0, 1);
	lcd.print(" i ");
	lcd.printByte(3);
	lcd.print(" arduinos!");
	delay(5000);

	i2c_lcd_debug_displayKeyCodes();
}

void loop()
{
}
