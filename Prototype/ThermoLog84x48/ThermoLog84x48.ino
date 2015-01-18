/*

	ThermoLog84x48
	based on Lcd84x48

 - Experimenting with Scheduler vs. MilliTimer.
 - Running key-scan code, interrupts.	
 */


/* *** Globals and sketch configuration *** */
#define DEBUG           1 /* Enable trace statements */
#define SERIAL          1 /* Enable serial */
							
#define _MEM            1   // Report free memory 
#define _DHT            1
#define DHT_HIGH        1   // enable for DHT22/AM2302, low for DHT11
#define _LCD84x48       1
							
#define REPORT_EVERY    5   // report every N measurement cycles
#define SMOOTH          5   // smoothing factor used for running averages
#define MEASURE_PERIOD  600  // how often to measure, in tenths of seconds
#define UI_IDLE         2000  // tenths of seconds idle time, ...
#define UI_STDBY        4000  // ms
#define MAXLENLINE      79
#define ATMEGA_CTEMP_ADJ 311
#define BL_INVERTED     0xFF
							

// Definition of interrupt names
#include <avr/io.h>
// ISR interrupt service routine
#include <avr/interrupt.h>

#include <JeeLib.h>
#include <OneWire.h>
#include <PCD8544.h>
// Adafruit DHT
#include <DHT.h>
#include <DotmpeLib.h>
#include <mpelib.h>


const String sketch = "X-ThermoLog84x48";
const int version = 0;

//char node[]
static String node = "templg";

// determined upon handshake 
char node_id[7];

volatile bool ui_irq;

static bool ui;

/* IO pins */
//             INT0     2     UI IRQ
#       define SCLK     3  // LCD84x48
#       define SDIN     4  // LCD84x48
#       define DC       5  // LCD84x48
#       define RESET    6  // LCD84x48
#       define SCE      7  // LCD84x48
#       define _DEBUG_LED 13
#if _DHT
#       define DHT_PIN = A3;
#endif

MpeSerial mpeser (57600);

MilliTimer idle, stdby;

/* *** Report variables *** {{{ */


static byte reportCount;    // count up until next report, i.e. packet send

struct {
#if _DHT
	int rhum    :7;  // 0..100 (avg'd)
#if DHT_HIGH
/* DHT22/AM2302: 20% to 100% @ 2% rhum, -40C to 80C @ ~0.5 temp */
	int temp    :10; // -500..+500 (int value of tenths avg)
#else
/* DHT11: 20% to 80% @ 5% rhum, 0C to 50C @ ~2C temp */
	int temp    :10; // -500..+500 (tenths, .5 resolution)
#endif
#endif
	int ctemp       :10; // atmega temperature: -500..+500 (tenths)
#if _MEM
	int memfree     :16;
#endif
} payload;


/* *** /Report variables *** }}} */

/* *** Scheduled tasks *** {{{ */

enum {
	MEASURE,
	REPORT,
	TASK_END
};
// Scheduler.pollWaiting returns -1 or -2
static const char WAITING = 0xFF; // -1: waiting to run
static const char IDLE = 0xFE; // -2: no tasks running

static word schedbuf[TASK_END];
Scheduler scheduler (schedbuf, TASK_END);

// has to be defined because we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }


/* *** /Scheduled tasks *** }}} */

/* *** Peripheral devices *** {{{ */

#if LDR_PORT
#endif

#if _DHT
/* DHT temp/rh sensor */
#if DHT_HIGH
DHT dht (DHT_PIN, DHT22);
#else
DHT dht (DHT_PIN, DHT11);
#endif
#endif // DHT

#if _LCD84x48
/* Nokkia 5110 display */

// The dimensions of the LCD (in pixels)...
static const byte LCD_WIDTH = 84;
static const byte LCD_HEIGHT = 48;

// The number of lines for the temperature chart...
static const byte CHART_HEIGHT = 3;

// A custom "degrees" symbol...
static const byte DEGREES_CHAR = 1;
static const byte degrees_glyph[] = { 0x00, 0x07, 0x05, 0x07, 0x00 };

// A bitmap graphic (10x2) of a thermometer...
static const byte THERMO_WIDTH = 10;
static const byte THERMO_HEIGHT = 2;
static const byte thermometer[] = { 0x00, 0x00, 0x48, 0xfe, 0x01, 0xfe, 0x00, 0x02, 0x05, 0x02,
	0x00, 0x00, 0x62, 0xff, 0xfe, 0xff, 0x60, 0x00, 0x00, 0x00};

static PCD8544 lcd84x48(SCLK, SDIN, DC, RESET, SCE);


#endif // LCD84x48

#if _DS
/* Dallas OneWire bus with registration for DS18B20 temperature sensors */



#endif // DS

#if _NRF24
/* nRF24L01+: nordic 2.4Ghz digital radio  */

#endif // NRF24

#if _RTC
#endif //_RTC

#if _HMC5883L
/* Digital magnetometer I2C module */

#endif // HMC5883L


/* *** /Peripheral devices }}} *** */

/* *** EEPROM config *** {{{ */

/* *** /EEPROM config *** }}} */


//ISR(INT0_vect) 
void irq0()
{
	ui_irq = true;
	//Sleepy::watchdogInterrupts(0);
}

/* Button detect using PCINT for ADC port */
void buttons_start()
{
	pinMode(A0, INPUT);	   // Pin A0 is input to which a switch is connected
	digitalWrite(A0, LOW);   // Configure internal pull-up resistor
	pinMode(A1, INPUT);	   // Pin A1 is input to which a switch is connected
	digitalWrite(A1, LOW);   // Configure internal pull-up resistor
	pinMode(A2, INPUT);	   // Pin A2 is input to which a switch is connected
	digitalWrite(A2, LOW);   // Configure internal pull-up resistor

	cli();		// switch interrupts off while messing with their settings  
	PCICR = 0x02;          // Enable PCINT1 interrupt
	PCMSK1 = 0b00000111;
	sei();		// turn interrupts back on
}

ISR(PCINT1_vect) {
	// Interrupt service routine. Every single PCINT8..14 (=ADC0..5) change
	// will generate an interrupt: but this will always be the
	// same interrupt routine
	if (digitalRead(A0) == 1)  Serial.println("A0");
	else if (digitalRead(A1) == 1)  Serial.println("A1");
	else if (digitalRead(A2) == 1)  Serial.println("A2");
}

/* *** Peripheral hardware routines *** {{{ */

#if LDR_PORT
#endif


/* *** PIR support *** {{{ */
#if PIR_PORT
#endif
/* *** /PIR support *** }}} */


#if _DHT
/* DHT temp/rh sensor 
 - AdafruitDHT
*/

#endif // DHT

#if _LCD84x48


void lcd_start()
{
	lcd84x48.begin(LCD_WIDTH, LCD_HEIGHT);

	// Register the custom symbol...
	lcd84x48.createChar(DEGREES_CHAR, degrees_glyph);

	lcd84x48.send( LOW, 0x21 );  // LCD Extended Commands on
	lcd84x48.send( LOW, 0x07 );  // Set Temp coefficent. //0x04--0x07
    lcd84x48.send( LOW, 0x12 );  // LCD bias mode 1:48. //0x13
    lcd84x48.send( LOW, 0x0C );  // LCD in normal mode.
	lcd84x48.send( LOW, 0x20 );  // LCD Extended Commands toggle off
}

void lcd_printWelcome(void)
{
	lcd84x48.setCursor(6, 0);
	lcd84x48.print(node);
	lcd84x48.setCursor(0, 5);
	lcd84x48.print(sketch);
}

void lcd_printDHT(void)
{
	lcd84x48.setCursor(12, 2);
	lcd84x48.print((float)payload.temp/10, 1);
	lcd84x48.print("\001C ");
	lcd84x48.setCursor(LCD_WIDTH/1.5, 2);
	lcd84x48.print(payload.rhum, 1);
	lcd84x48.print("%");
}
#endif // LCD84x48

#if _DS
/* Dallas OneWire bus with registration for DS18B20 temperature sensors */


#endif // DS

#if _NRF24
/* Nordic nRF24L01+ radio routines */

#endif // NRF24 funcs

#if _RTC
#endif //_RTC

#if _HMC5883L
/* Digital magnetometer I2C routines */
#endif //_HMC5883L


/* *** /Peripheral hardware routines }}} *** */

/* *** Initialization routines *** {{{ */

void doConfig(void)
{
}

void initLibs()
{
#if _DHT
	dht.begin();
#endif

	buttons_start();
}


/* *** /Initialization routines *** }}} */

/* *** Run-time handlers *** {{{ */

void doReset(void)
{
#if _DEBUG_LED
	pinMode(_DEBUG_LED, OUTPUT);
#endif
	pinMode(BL, OUTPUT);
	digitalWrite(BL, LOW ^ BL_INVERTED);
	attachInterrupt(INT0, irq0, RISING);
	ui_irq = true;
	tick = 0;
	reportCount = REPORT_EVERY;     // report right away for easy debugging
	scheduler.timer(MEASURE, 0);    // get the measurement loop going
}

bool doAnnounce()
{
	return false;
}

void doMeasure()
{
	byte firstTime = payload.ctemp == 0; // special case to init running avg

	int ctemp = internalTemp();
	payload.ctemp = smoothedAverage(payload.ctemp, ctemp, firstTime);

#if _DHT
	float h = dht.readHumidity();
	float t = dht.readTemperature();
	if (isnan(h)) {
#if SERIAL && DEBUG
		Serial.println(F("Failed to read DHT11 humidity"));
#endif
	} else {
		payload.rhum = smoothedAverage(payload.rhum, round(h), firstTime);
#if SERIAL && DEBUG_MEASURE
		Serial.print(F("DHT RH new/avg "));
		Serial.print(h);
		Serial.print(' ');
		Serial.println(payload.rhum);
#endif
	}
	if (isnan(t)) {
#if SERIAL && DEBUG
		Serial.println(F("Failed to read DHT11 temperature"));
#endif
	} else {
		payload.temp = smoothedAverage(payload.temp, (int)(t * 10), firstTime);
#if SERIAL && DEBUG_MEASURE
		Serial.print(F("DHT T new/avg "));
		Serial.print(t);
		Serial.print(' ');
		Serial.println(payload.temp);
#endif
	}
#endif // _DHT

#if _MEM
	payload.memfree = freeRam();
#if SERIAL && DEBUG_MEASURE
	Serial.print("MEM free ");
	Serial.println(payload.memfree);
#endif
#endif
}

void doReport(void)
{
#if SERIAL
	Serial.print(node);
	Serial.print(" ");
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
	Serial.println();
#endif//SERIAL
}

void uiStart()
{
	idle.set(UI_IDLE);
	if (!ui) {
		ui = true;
		lcd_start();
		lcd_printWelcome();
	}
}

void runScheduler(char task)
{
	switch (task) {

		case MEASURE:
			debugline("MEASURE");
#ifdef _DEBUG_LED
			blink(_DEBUG_LED, 1, 15);
#endif
			// reschedule these measurements periodically
			scheduler.timer(MEASURE, MEASURE_PERIOD);
			doMeasure();
			lcd_printDHT();
			// every so often, a report needs to be sent out
			if (++reportCount >= REPORT_EVERY) {
				reportCount = 0;
				scheduler.timer(REPORT, 0);
			}
			serialFlush();
			break;

		case REPORT:
			debugline("REPORT");
			doReport();
			serialFlush();
			break;

		default:
			Serial.print("0x");
			Serial.print(task, HEX);
			Serial.println(" ?");
			serialFlush();
			break;

	}
}

void runCommand()
{
}


/* *** /Run-time handlers *** }}} */

/* *** InputParser handlers *** {{{ */

/* *** /InputParser handlers *** }}} */

/* *** Main *** {{{ */


void setup(void)
{
#if SERIAL
	mpeser.begin();
	mpeser.startAnnounce(sketch, version);
#if DEBUG || _MEM
	Serial.print("Free RAM: ");
	Serial.println(freeRam());
#endif
	serialFlush();
#endif

	initLibs();

	doReset();
}

void loop(void)
{
	//doMeasure();
	//doReport();
	//delay(15000);
	//return;
	if (ui_irq) {
		debugline("Irq");
		ui_irq = false;
		uiStart();
		analogWrite(BL, 0xAF ^ BL_INVERTED);
	}
	debug_ticks();
	char task = scheduler.poll();
	if (-1 < task && task < IDLE) {
		runScheduler(task);
	}
	if (ui) {
		if (idle.poll()) {
			debugline("Idle");
			analogWrite(BL, 0x1F ^ BL_INVERTED);
			stdby.set(UI_STDBY);
		} else if (stdby.poll()) {
			debugline("StdBy");
			digitalWrite(BL, LOW ^ BL_INVERTED);
			lcd84x48.clear();
			lcd84x48.stop();
			ui = false;
		}
	} else {
#ifdef _DEBUG_LED
		blink(_DEBUG_LED, 1, 15);
#endif
		debugline("Sleep");
		serialFlush();
		char task = scheduler.pollWaiting();
		debugline("Wakeup");
		if (-1 < task && task < IDLE) {
			runScheduler(task);
		}
	}
}

/* }}} *** */

