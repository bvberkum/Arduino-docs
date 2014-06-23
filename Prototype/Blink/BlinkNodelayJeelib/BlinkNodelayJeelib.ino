/*

BlinkNodelayJeelib - use MilliTimer for non-blocking blink

*/

#include <JeeLib.h>


//static const float BLINK_RATE = 0.25; // 4sec period
static const float BLINK_RATE = 0.5; // 2sec period
static const int BLINK_HALF_PERIOD = 1000 / BLINK_RATE / 2;
static const int LED_PIN = 13;

static bool ledState = false;

static MilliTimer blink1;


static void toggleBlinkLed()
{
	ledState = !ledState;
	digitalWrite(LED_PIN, ledState);
}

void reset()
{
	blink1.set(BLINK_HALF_PERIOD);
}

void setup(void) 
{
	pinMode(LED_PIN, OUTPUT);
	pinMode(LED_PIN, HIGH);
	reset();
}

void loop(void)
{
	if (blink1.poll()) {
		toggleBlinkLed();
		reset();
	}
}

