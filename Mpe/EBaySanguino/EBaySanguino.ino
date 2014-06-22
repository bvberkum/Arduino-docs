/**
one debug led at 0
*/


void setup(void) {
	pinMode(3, OUTPUT);
	pinMode(4, OUTPUT);

	pinMode(7, OUTPUT);
	pinMode(8, OUTPUT);
	pinMode(9, OUTPUT);
	digitalWrite(3, LOW);
	digitalWrite(4, LOW);
	digitalWrite(5, LOW);
	digitalWrite(7, LOW);
	digitalWrite(8, LOW);
	digitalWrite(9, LOW);
}

int latch_delay = 1500;
int switch_delay = 2500;

void loop(void) {


	digitalWrite(9, HIGH);
	delay(latch_delay);
	digitalWrite(9, LOW);
	delay(switch_delay);

//	digitalWrite(3, HIGH);
//	digitalWrite(4, HIGH);
//	digitalWrite(7, HIGH);

	digitalWrite(8, HIGH);
	delay(latch_delay);
	digitalWrite(8, LOW);
	delay(switch_delay);

}
