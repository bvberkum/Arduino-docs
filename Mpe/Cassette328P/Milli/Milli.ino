
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
#define RED_DELAY    6000


static unsigned long count;

static unsigned long now () {
	// FIXME 49-day overflow
	return millis() / 1000;
}

/* blocking blink */
void blink(int led, int count, int length, int length_off=0) {
  for (int i=0;i<count;i++) {
    digitalWrite (led, HIGH);
    delay(length);
    digitalWrite (led, LOW);
    (length_off > 0) ? delay(length_off) : delay(length);
  }
}

static const unsigned long maxBlinkLoops = 3; /* parallel blinks */
static unsigned long blinkSpec[maxBlinkLoops][3]; /* 
lednr -> blink-count, active-mili, inactive-mili */
unsigned long blinks[maxBlinkLoops]; /*
    lednr -> remaining periods */
long ledState [maxBlinkLoops]; /*
	lednr -> signed for inactive-mili started, unsigned for active-mili started */
static long led[maxBlinkLoops] = {
    LED_GREEN,
    LED_YELLOW,
    LED_RED
};

void debugLeds() {
    Serial.print("Blinks: ");
    for (int i=0;i<maxBlinkLoops;i++) {
        Serial.print(blinks[i]);
        Serial.print(' ');
    }
    Serial.println(' ');
    Serial.print("States: ");
    for (int i=0;i<maxBlinkLoops;i++) {
        Serial.print(ledState[i]);
        Serial.print(' ');
    }
    Serial.println(' ');
}

void blinkPrime(int i, long count, long high, long low) {
    blinkSpec[i][0] = count;
    blinkSpec[i][1] = high;
    blinkSpec[i][2] = low;
    blinks[i] = blinkSpec[i][0]-1;
    ledState[i] = millis()+blinkSpec[i][1];
    if (ledState[i] > 0) {
        digitalWrite(led[i], HIGH);
    }
}
void execLeds() {
    for (int i=0;i<maxBlinkLoops;i++) {
        if (ledState[i] > 0) {
            if (ledState[i] <= millis()) {
                /* end of active periodhalf, set for inactive half */
                ledState[i] = 0-(millis()+blinkSpec[i][2]);
                digitalWrite(led[i], LOW);
            }
        } else if (ledState[i] < 0) {
            if (0-ledState[i] <= millis()) {
                /* end of inactive periodhalf, set for next blink or end */
                if (blinks[i]) {
                    blinks[i]--;
                    ledState[i] = millis()+blinkSpec[i][1];
                    digitalWrite(led[i], HIGH);
                } else {
                    ledState[i] = 0;
                }
            }
        }
    }
}
static void printMillis() {

	static unsigned long lasttime = 0;  // static vars remembers values over function calls without being global

	if (millis() - lasttime <= REPORT_DELAY) return;  // instead of delay(2) just ignore if called too fast 

	lasttime = millis();
    blinkPrime( 0, 3, 30, 40);

	Serial.println();
	Serial.print(count);	
	Serial.print(", ");	
	Serial.print(now());	
	Serial.print(", ");	
	Serial.print(millis());	
	Serial.print(" ");	
    //debugLeds();
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

    blinkPrime( 0, 0, 0, 0 );
    digitalWrite( LED_GREEN, LOW );
    blinkPrime( 1, 100, 100, 900 );

    for (int i=0;i<maxBlinkLoops;i++) {
        Serial.print(blinkSpec[i][0]);
        Serial.print(' ');
        Serial.print(blinkSpec[i][1]);
        Serial.print(' ');
        Serial.print(blinkSpec[i][2]);
        Serial.println(' ');
    }
}

void loop() {
	static unsigned long lasttime = 0;  
	execLeds();
	printMillis();
	if (millis() - lasttime <= RED_DELAY) return;
	lasttime = millis();
    blinkPrime( 2, 5, 20, 20 );
}
