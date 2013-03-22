void setup() {
	pinMode(4, OUTPUT);
	pinMode(14, OUTPUT);
	pinMode(7, OUTPUT);
	pinMode(17, OUTPUT);
}

static void setLatch (bool on) {
  if (on) {
    //bitSet(PORTD, 5); // D.5 = DIO2
    //bitSet(PORTC, 1); // A.1 = AIO2
    bitSet(PORTD, 4); // D.4 = DIO1
    bitSet(PORTC, 0); // A.0 = AIO1
  } else {
    bitSet(PORTD, 7); // D.7 = DIO4
    bitSet(PORTC, 3); // A.3 = AIO4
  }
 
  delay(1000);
 
  //bitClear(PORTD, 5); // D.5 = DIO2
  //bitClear(PORTC, 1); // A.1 = AIO2
  bitClear(PORTD, 4); // D.4 = DIO1
  bitClear(PORTC, 0); // A.0 = AIO1
  bitClear(PORTD, 7); // D.7 = DIO4
  bitClear(PORTC, 3); // A.3 = AIO4
}

static void setLatch_cap(bool on) {
	if (on) {
		bitSet(PORTD, 4); // D.4 = DIO1
		bitSet(PORTC, 0); // A.0 = AIO1
	} else {
		bitClear(PORTD, 4); // D.4 = DIO1
		bitClear(PORTC, 0); // A.0 = AIO1
	}
}
/* slower Arduino routine:
*/
/* 
static void setLatch (bool on) {
  pinMode(4, on ? INPUT : OUTPUT);  // DIO1
  pinMode(14, on ? INPUT : OUTPUT);  // AIO1

  pinMode(7, on ? OUTPUT : INPUT);  // DIO4
  pinMode(17, on ? OUTPUT : INPUT);  // AIO4
 
  digitalWrite(4, on ? 0 : 1);
  digitalWrite(14, on ? 0 : 1);
  digitalWrite(7, on ? 1 : 0);
  digitalWrite(17, on ? 1 : 0);
 
  delay(25);
   
  pinMode(4, INPUT);
  pinMode(14, INPUT);
  pinMode(7, INPUT);
  pinMode(17, INPUT);
}
*/
void loop() {
	/*
	digitalWrite(14, HIGH);
	delay(6);
	digitalWrite(14, LOW);
	delay(2500);

	digitalWrite(4, HIGH);
	delay(6);
	digitalWrite(4, LOW);
	delay(2500);
	*/

	setLatch(true);
	//delay(1000);
	setLatch(false);
	//delay(1000);

//setLatch_cap(true);
//  delay(2500);
//  setLatch_cap(false);
//  delay(2500);
}
