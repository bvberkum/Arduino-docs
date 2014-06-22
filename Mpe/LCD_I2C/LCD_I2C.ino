/**
 * Connect a 16x2 LCD by I2C bus (provided by MCP23008)
 * But I do not have this plug.
 */

// Demo sketch for an LCD connected to I2C port via MCP23008 I/O expander
// 2009-10-31 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

#include <JeeLib.h>
#include <PortsLCD.h>

PortI2C i2c_bus1(1);
PortI2C i2c_bus2(4);


/** LCD routines **/
LiquidCrystalI2C lcd (i2c_bus2, 0x22);

String msg = "Hi there!";
char* lcdLines[] = {};

void marquee(String msg) {
	int length = msg.length();
	for (int i=-length; i<17; i++) {
		lcd.clear();
		int p=0;
		int o=0;
		if (i >= 0) {
			p = i;
		}
		if (i <  0) {
			o = length - (i + length);
		}
		lcd.setCursor(p, 0);
		lcd.print(msg.substring(o));
		lcd.setCursor(0, 1);
		// print the number of seconds since reset:
		//lcd.print(millis()/1000);
		lcd.setCursor(15,1);
		delay(300);
	}
}

void lcd_init() {
	// set up the LCD's number of rows and columns: 
	lcd.begin(16, 2);
	// Print a message to the LCD.
	lcd.print("  .Mpe LCD I2C");
	lcd.setCursor(0,1);
	lcd.print("     v1 2012");
	lcd.setCursor(15,1);
	lcd.cursor();
	lcd.blink();
}

/** PCA9635 Dimmer */
DimmerPlug dimmer (i2c_bus2, 0x47);
byte brightness;

int level = 0x1FFF;
void dimmer_init() {
    dimmer.begin();
    // set each channel to max brightness
    dimmer.setMulti(dimmer.PWM0, 255, 255, 255, 255,
                                 255, 255, 255, 255,
                                 255, 255, 255, 255,
                                 255, 255, 255, 255, -1);
    // set up for group blinking
    dimmer.setReg(dimmer.MODE2, 0x34);
    // blink rate: 0 = very fast, 255 = 10s
    dimmer.setReg(dimmer.GRPFREQ, 0);
    // blink duty cycle: 0 = full on, 255 = full off
    dimmer.setReg(dimmer.GRPPWM, 40);
    // let the chip do its thing for a while
    delay(10000);

    // set up for group dimming
    dimmer.setReg(dimmer.MODE2, 0x14);
    // gradually decrease brightness to minimum
    for (byte i = 100; i < 255; ++i) {
        dimmer.setReg(dimmer.GRPPWM, i);
        delay(100);
    }
    // the rest of the code dims individual channels 
    delay(2000);
}

/** static entry */
void setup() {
	lcd_init();
	dimmer_init();
	delay(3000);
	lcd.noCursor();
	lcd.noBlink();

	dimmer.setReg(DimmerPlug::MODE1, 0x00); //normal
	dimmer.setReg(DimmerPlug::MODE2, 0x00); // link
	dimmer.setReg(DimmerPlug::GRPPWM, 0x40); // duty cycel 40%
	dimmer.setReg(DimmerPlug::GRPFREQ, 5); //
	dimmer.setReg(DimmerPlug::LEDOUT0, 0x03);

	brightness = 0;
}

void loop() {
	marquee(msg);
	// set the cursor to column 0, line 1
	// (note: line 1 is the second row, since counting begins with 0):
	// turn the backlight off, briefly
	//delay(500);
	//lcd.noBacklight();
	//delay(50);
	//lcd.backlight();
	//delay(500);
	dimmer.setReg(DimmerPlug::PWM0, ++brightness);
	delay(10);
	if (brightness == 255)
		brightness = 0;
}

