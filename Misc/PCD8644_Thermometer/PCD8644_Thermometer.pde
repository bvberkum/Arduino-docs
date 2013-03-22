/*
 * Thermometer - read temperature using an DHT11 sensor and display it on a PCD8544 LCD.
 *
 * Copyright (c) 2013 B. van Berkum
 * Copyright (c) 2010 Carlos Rodrigues <cefrodrigues@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


#include <PCD8544.h>
#include <DHT.h>
#include <JeeLib.h>
#include <avr/sleep.h>


// has to be defined because we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }


static const byte sensorPin = A3;
static const byte ledPin = 13;

// The dimensions of the LCD (in pixels)...
static const byte LCD_WIDTH = 84;
static const byte LCD_HEIGHT = 48;

// The number of lines for the temperature chart...
static const byte CHART_HEIGHT = 3;

// A custom "degrees" symbol...
static const byte DEGREES_CHAR = 1;
static const byte degrees_glyph[] = { 0x00, 0x07, 0x05, 0x07, 0x00 };

// A bitmap graphic (10x2) of a thermometer...
//static const byte THERMO_WIDTH = 10;
//static const byte THERMO_HEIGHT = 2;
//static const byte thermometer[] = { 0x00, 0x00, 0x48, 0xfe, 0x01, 0xfe, 0x00, 0x02, 0x05, 0x02,
//	0x00, 0x00, 0x62, 0xff, 0xfe, 0xff, 0x60, 0x00, 0x00, 0x00};

static PCD8544 lcd(3, 4, 5, 6, 7); /* SCLK, SDIN, DC, RESET, SCE */
static DHT dht(sensorPin, DHT11);

void setup() {
	lcd.begin(LCD_WIDTH, LCD_HEIGHT);

	// Register the custom symbol...
	lcd.createChar(DEGREES_CHAR, degrees_glyph);

	pinMode(ledPin, OUTPUT);
	pinMode(sensorPin, INPUT);

	dht.begin();
}


void loop() {
	// Start beyond the edge of the screen...
	static byte xChart = LCD_WIDTH;

	//lcd.begin();

	digitalWrite(ledPin, HIGH);

	// Read the temperature (in celsius)...
	float temp = dht.readTemperature();
	float hum = dht.readHumidity();

	// Draw the thermometer bitmap at the bottom left corner...
	//lcd.setCursor(0, LCD_HEIGHT/8 - THERMO_HEIGHT);
	//lcd.drawBitmap(thermometer, THERMO_WIDTH, THERMO_HEIGHT);

	// Wrap the chart's current position...
	if (xChart >= LCD_WIDTH) {
		xChart = 0;//THERMO_WIDTH + 2;
	}

	// Update the temperature chart...  
	lcd.setCursor(xChart, 0);
	lcd.drawColumn(CHART_HEIGHT, map(temp, 0, 35, 0, CHART_HEIGHT*8));  // ...clipped to the 0-35C range.
	lcd.drawColumn(CHART_HEIGHT, 0);         // ...with a clear marker to see the current chart position.

	lcd.setCursor(xChart, 3);
	lcd.drawColumn(CHART_HEIGHT, map(hum, 0, 100, 0, CHART_HEIGHT*8));
	//lcd.drawColumn(CHART_HEIGHT, map(hum, 0, 100, CHART_HEIGHT*8, 0) - CHART_HEIGHT*8 );  // ...clipped to the 0-35C range.
	lcd.drawColumn(CHART_HEIGHT, 0);         // ...with a clear marker to see the current chart position.

	// Print the temperature (using the custom "degrees" symbol)...
	lcd.setCursor(0, 0);
	lcd.print(temp, 1);
	lcd.print("\001C ");
	lcd.setCursor(0, 3);
	lcd.print(hum, 1);
	lcd.print("%");

	xChart++;

	delay(300);
	//lcd.stop();
	//digitalWrite(ledPin, LOW);  
	byte minutes = 5; 
	while (minutes-- > 0)
		Sleepy::loseSomeTime(60000);
}

/* EOF - Thermometer.ino  */
