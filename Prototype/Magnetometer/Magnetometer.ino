#include <DotmpeLib.h>
#include <JeeLib.h>
#include <Wire.h>
#include <mpelib.h>


/** Globals and sketch configuration  */
#define DEBUG           1 /* Enable trace statements */
#define SERIAL          1 /* Enable serial */
#define DEBUG_MEASURE   0

#define MEASURE_PERIOD  50  // how often to measure, in tenths of seconds
#define REPORT_EVERY    5   // report every N measurement cycles
#define SMOOTH          5   // smoothing factor used for running averages

#define MAXLENLINE      79
#define SRAM_SIZE       0x800 // atmega328, for debugging
// set the sync mode to 2 if the fuses are still the Arduino default
// mode 3 (full powerdown) can only be used with 258 CK startup fuses
#define RADIO_SYNC_MODE 2

#define _HMC5883L       1


String sketch = "Magnetometer";
String version = "0";

String node_id = "magn-1";

int tick = 0;
int pos = 0;

MpeSerial mpeser (57600);

/* Scheduled tasks */
enum { MEASURE, REPORT, END };

static word schedbuf[END];
Scheduler scheduler (schedbuf, END);

// has to be defined because we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }


#if _HMC5883L
/* Digital magnetometer I2C module */

/* The I2C address of the module */
#define HMC5803L_Address 0x1E

/* Register address for the X Y and Z data */
#define X 3
#define Y 7
#define Z 5

#endif //_HMC5883L

/* Report variables */

static byte reportCount;    // count up until next report, i.e. packet send

// This defines the structure of the packets which get sent out by wireless:
struct {
#if _HMC5883L
	int magnx;
	int magny;
	int magnz;
#endif //_HMC5883L
} payload;



#if _HMC5883L
/* Digital magnetometer I2C module */

/* This function will initialise the module and only needs to be run once
   after the module is first powered up or reset */
void Init_HMC5803L(void)
{
	/* Set the module to 8x averaging and 15Hz measurement rate */
	Wire.beginTransmission(HMC5803L_Address);
	Wire.write(0x00);
	Wire.write(0x70);

	/* Set a gain of 5 */
	Wire.write(0x01);
	Wire.write(0xA0);
	Wire.endTransmission();
}


/* This function will read once from one of the 3 axis data registers
   and return the 16 bit signed result. */
int HMC5803L_Read(byte Axis)
{
	int Result;

	/* Initiate a single measurement */
	Wire.beginTransmission(HMC5803L_Address);
	Wire.write(0x02);
	Wire.write(0x01);
	Wire.endTransmission();
	delay(6);

	/* Move modules the resiger pointer to one of the axis data registers */
	Wire.beginTransmission(HMC5803L_Address);
	Wire.write(Axis);
	Wire.endTransmission();

	/* Read the data from registers (there are two 8 bit registers for each axis) */
	Wire.requestFrom(HMC5803L_Address, 2);
	Result = Wire.read() << 8;
	Result |= Wire.read();

	return Result;
}
#endif //_HMC5883L


/* *** Initialization routines *** {{{ */

void doConfig(void)
{
}

void initLibs()
{
#if _HMC5883L
	/* Initialise the Wire library */
	Wire.begin();

	/* Initialise the module */
	Init_HMC5803L();
#endif //_HMC5883L
}

/* Run-time handlers */

void doReset(void)
{
	tick = 0;
	reportCount = REPORT_EVERY;     // report right away for easy debugging
	scheduler.timer(MEASURE, 0);    // start the measurement loop going
}

void doAnnounce()
{
#if SERIAL
	Serial.print(node_id);
#if _HMC5883L
	Serial.print(' ');
	Serial.print((int) payload.magnx);
	Serial.print(' ');
	Serial.print((int) payload.magny);
	Serial.print(' ');
	Serial.print((int) payload.magnz);
#endif
	Serial.println();
#endif //SERIAL
}

// readout all the sensors and other values
void doMeasure()
{
	byte firstTime = payload.magnx == 0; // special case to init running avg

#if _HMC5883L
	payload.magnx = smoothedAverage(payload.magnx, HMC5803L_Read(X), firstTime);
	payload.magny = smoothedAverage(payload.magny, HMC5803L_Read(Y), firstTime);
	payload.magnz = smoothedAverage(payload.magnz, HMC5803L_Read(Z), firstTime);
#endif //_HMC5883L
}

// periodic report, i.e. send out a packet and optionally report on serial port
void doReport(void)
{
#if _RFM12B
	rf12_sleep(RF12_WAKEUP);
	rf12_sendNow(0, &payload, sizeof payload);
	rf12_sendWait(RADIO_SYNC_MODE);
	rf12_sleep(RF12_SLEEP);
#endif

#if SERIAL
	/* Report over serial, same fields and order as announced */
	Serial.print(node_id);
	Serial.print(" ");
#if _HMC5883L
	Serial.print(' ');
	Serial.print(payload.magnx);
	Serial.print(' ');
	Serial.print(payload.magny);
	Serial.print(' ');
	Serial.print(payload.magnz);
#endif //_HMC5883L
	Serial.println();
#endif //SERIAL
}

void runScheduler(char task)
{
	switch (task) {

		case MEASURE:
			// reschedule these measurements periodically
			debug("MEASURE");
			scheduler.timer(MEASURE, MEASURE_PERIOD);
			doMeasure();
			// every so often, a report needs to be sent out
			if (++reportCount >= REPORT_EVERY) {
				reportCount = 0;
				scheduler.timer(REPORT, 0);
				debug("scheduled report");
			}
			serialFlush();
			break;

		case REPORT:
			debug("REPORT");
			doReport();
			serialFlush();
			break;

#if DEBUG
		default:
			Serial.print("0x");
			Serial.print(task, HEX);
			Serial.println(" ?");
			serialFlush();
			break;
#endif

	}
}


/* Main */

void setup(void)
{
#if SERIAL
	mpeser.begin();
	mpeser.startAnnounce(sketch, version);
	serialFlush();
#endif

	initLibs();

	doReset();
}

void loop(void)
{
	doMeasure();
	doReport();
	delay(300);
	return;

	debug_ticks();

	char task = scheduler.pollWaiting();
	//if (task == 0xFF) {} // -1
	//else if (task == 0xFE) {} // -2
	//else
	runScheduler(task);
}

