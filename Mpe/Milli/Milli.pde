
#if defined(__AVR_ATtiny84__) ||defined(__AVR_ATtiny44__)
#define SERIAL_BAUD 38400
#else
#define SERIAL_BAUD 57600
#endif

// XXX running for Cassette328P board
#define LED_RED   5
#define LED_YELLOW  6
#define LED_GREEN     7

#define REPORT_DELAY 1000


static unsigned long count;

static unsigned long now () {
	// FIXME 49-day overflow
	return millis() / 1000;
}

void blink(int led, int count, int length, int length_off=0) {
  for (int i=0;i<count;i++) {
    digitalWrite (led, HIGH);
    delay(length);
    digitalWrite (led, LOW);
    (length_off > 0) ? delay(length_off) : delay(length);
  }
}

static void printMillis() {
	blink(LED_YELLOW, 1, 6, 40);

	static unsigned long lasttime = 0;  // static vars remembers values over function calls without being global

	if (millis() - lasttime <= REPORT_DELAY) return;  // instead of delay(2) just ignore if called too fast 
	lasttime = millis();

	Serial.println();
	Serial.print(count);	
	Serial.print(", ");	
	Serial.print(now());	
	Serial.print(", ");	
	Serial.print(millis());	
	Serial.print(" ");	
	blink(LED_RED, 3, 20, 40);
	count ++;
}

int freeRam () {
	extern int __heap_start, *__brkval; 
	int v; 
	return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

void setup() {
	Serial.begin(SERIAL_BAUD);
	Serial.println();
	Serial.print(F("[Milli]"));
	Serial.println();
	Serial.print(F("[freeRAM: "));
	Serial.print(freeRam());
	Serial.print("] ");	
	count = 0;
	pinMode( LED_GREEN, OUTPUT );
	pinMode( LED_YELLOW, OUTPUT );
	pinMode( LED_RED, OUTPUT );

	blink(LED_GREEN, 2, 50);
}

void loop() {
	printMillis();
}
