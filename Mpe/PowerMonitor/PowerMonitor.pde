/**

  - Measure mains frequency.
  - TODO: read ACS712 sensors.

  Adapted from
http://jeelabs.org/2009/05/28/measuring-the-ac-line-frequency/#comments
  */
uint8_t transitions = 200; /* for 50 hz rectified, expect 200 interrupts */
volatile uint8_t count; /* interrupt counter */
long lastTime;

ISR(ANALOG_COMP_vect) {
	++count;
}

void setup()
{
	Serial.begin( 57600 );
	Serial.println( "PowerMonitor" );
	delay(2);

	// compare against 1.1V reference
	ACSR = _BV(ACBG) | _BV(ACI) | _BV(ACIE);

	// use MUX to select AIO port 1
	ADCSRA &= ~ bit(ADEN);
	ADCSRB |= bit(ACME);
	ADMUX = 0;
}

void loop()
{
	//Serial.println('.');
	//delay(2);

	cli();
	
	if (count >= transitions) {

		long now = micros();
		count -= transitions;
		sei();

		if (lastTime != 0) {
			long millihz = long(50e9 / (now - lastTime));
			Serial.print(millihz/1000);
			Serial.print(".");
			Serial.print((millihz/100) % 10, DEC);
			Serial.print((millihz/10) % 10, DEC);
			Serial.print((millihz) % 10, DEC);
			Serial.print(" Hz ");
			if ( millihz >= 50000) {
				Serial.print("+");
			}
			Serial.print((millihz - 50000) * 0.002);
			Serial.println(" %");
		}

		lastTime = now;

	} else {
		sei();
	}

}

