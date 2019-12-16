/*
 * Thermometer - read temperature using an DHT11 sensor and display it on a
 * PCD8544 LCD. Adapted from 2011 script to read LM35, expanded with JeeLib
 * low-power code (Sleepy).
 *
 * - XXX: DHT11 is for 3.5-5.5V
 * - DHT22/AM2302 is 3.3-6v?
 *
 * Copyright (c) 2010 Carlos Rodrigues <cefrodrigues@gmail.com>
 * Copyright (c) 2013 B. van Berkum <dev@dotmpe.com>
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

// Adafruit DHT
#include <DHT.h>
#include <DotmpeLib.h>
#include <mpelib.h>
#include <JeeLib.h>
#include <PCD8544.h>


/** Globals and sketch configuration  */
#define DEBUG           1 /* Enable trace statements */
#define SERIAL          1 /* Enable serial */

#define DEBUG_MEASURE   1


const String sketch = "PCD8544_Thermometer";
const String node = "";
const int version = 0;

const byte backlight = 9;
const byte sensorPin = 11;
const byte ledPin = 10;


MpeSerial mpeser (57600);

// has to be defined because we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

DHT dht(sensorPin, DHT22);


// The dimensions of the LCD (in pixels)...
const byte LCD_WIDTH = 84;
const byte LCD_HEIGHT = 48;

// The number of lines for the temperature chart...
const byte CHART_HEIGHT = 3;

// A custom "degrees" symbol...
const byte DEGREES_CHAR = 1;
const byte degrees_glyph[] = { 0x00, 0x07, 0x05, 0x07, 0x00 };

// A bitmap graphic (10x2) of a thermometer...
const byte THERMO_WIDTH = 10;
const byte THERMO_HEIGHT = 2;
const byte thermometer[] = { 0x00, 0x00, 0x48, 0xfe, 0x01, 0xfe, 0x00, 0x02, 0x05, 0x02,
  0x00, 0x00, 0x62, 0xff, 0xfe, 0xff, 0x60, 0x00, 0x00, 0x00};

PCD8544 lcd(A0, 4, A1, A2, 7); /* SCLK, SDIN, DC, RESET, SCE */

// Result: 2k (m328) - 1561 free = 487 bytes SRAM used by this pogram
int memfree;

volatile bool ui_irq;


void doMeasure(void)
{
  memfree = freeRam();
#if SERIAL && DEBUG_MEASURE
  Serial.print(" MEM free ");
  Serial.println(memfree);
#endif
}

//ISR(INT0_vect)
void irq0()
{
  ui_irq = true;
  Sleepy::watchdogInterrupts(0);
}


/* Main */

void setup(void)
{
  mpeser.begin();
  mpeser.startAnnounce(sketch, String(version));
  serialFlush();
  //attachInterrupt(INT0, irq0, RISING); // D2

  lcd.begin(LCD_WIDTH, LCD_HEIGHT);

  // Register the custom symbol...
  lcd.createChar(DEGREES_CHAR, degrees_glyph);

  lcd.send( LOW, 0x21 );  // LCD Extended Commands on
  lcd.send( LOW, 0x07 );  // Set Temp coefficent. //0x04--0x07
  lcd.send( LOW, 0x12 );  // LCD bias 0x13=1:48
  lcd.send( LOW, 0x20 );  // LCD Extended Commands toggle off

  pinMode(backlight, OUTPUT);
  pinMode(sensorPin, INPUT);
  pinMode(ledPin, OUTPUT);

  dht.begin();

  analogWrite(backlight, 0xaf);
}

void loop(void)
{
  //digitalWrite(ledPin, HIGH);
  //doMeasure();
  //digitalWrite(ledPin, LOW);

  // Start beyond the edge of the screen
  static byte xChart = LCD_WIDTH;

  // Read the temperature (in celsius)
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  // Draw the thermometer bitmap at the bottom left corner
  //lcd.setCursor(0, LCD_HEIGHT/8 - THERMO_HEIGHT);
  //lcd.drawBitmap(thermometer, THERMO_WIDTH, THERMO_HEIGHT);

  // Wrap the chart's current position...
  if (xChart >= LCD_WIDTH) {
    xChart = 0;//THERMO_WIDTH + 2;
  }

  // Update the temperature chart...
  lcd.setCursor(xChart, 0);
  lcd.drawColumn(CHART_HEIGHT, map(temp, 13, 35, 0, CHART_HEIGHT*8)); // clip to the 13-35C range.
  lcd.drawColumn(CHART_HEIGHT, 0); // with a clear marker to see the current chart position.

  lcd.setCursor(xChart, 3);
  lcd.drawColumn(CHART_HEIGHT, map(hum, 0, 100, 0, CHART_HEIGHT*8));
  //lcd.drawColumn(CHART_HEIGHT, map(hum, 0, 100, CHART_HEIGHT*8, 0) - CHART_HEIGHT*8 );  // ...clipped to the 0-35C range.
  lcd.drawColumn(CHART_HEIGHT, 0); // with a clear marker to see the current chart position.

  // Print the temperature (using the custom "degrees" symbol)...
  lcd.setCursor(0, 0);
  lcd.print(temp, 1);
  lcd.print("\001C ");
  lcd.setCursor(0, 3);
  lcd.print(hum, 1);
  lcd.print("%");

  xChart++;

  delay(5000);
  //Sleepy::loseSomeTime(5000); // Sleep for approx. miliseconds

  //lcd.stop();
  //byte minutes = 5;
  //while (minutes-- > 0) {
  //  Sleepy::loseSomeTime(60000); // Sleep for approx. miliseconds
  //}
}

/* EOF - Thermometer.ino  */
