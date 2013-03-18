void setup() {
	pinMode(4, OUTPUT);
//	digitalWrite(4, HIGH);
}
/*
static void setLatch (bool on) {
  if (on) {
    //bitSet(PORTD, 5); // D.5 = DIO2
    //bitSet(PORTC, 1); // A.1 = AIO2
    bitSet(PORTD, 4); // D.4 = DIO1
    bitSet(PORTC, 0); // A.0 = AIO1
  } else {
    bitSet(PORTD, 6); // D.6 = DIO3
    bitSet(PORTC, 2); // A.2 = AIO3
  }
 
  delay(3000);
 
  //bitClear(PORTD, 5); // D.5 = DIO2
  //bitClear(PORTC, 1); // A.1 = AIO2
  bitClear(PORTD, 4); // D.4 = DIO1
  bitClear(PORTC, 0); // A.0 = AIO1
  bitClear(PORTD, 6); // D.6 = DIO3
  bitClear(PORTC, 2); // A.2 = AIO3
}
*/
/* slower Arduino routine:
*/
 
static void setLatch (bool on) {
  pinMode(7, on ? OUTPUT : OUTPUT);  // DIO1
  //pinMode(14, OUTPUT); // AIO1
  pinMode(4, on ? INPUT : OUTPUT);  // DIO4
  //pinMode(16, OUTPUT); // AIO3
 
  digitalWrite(4, on ? 0 : 1);
  //digitalWrite(14, on ? 0 : 1);
  digitalWrite(7, on ? 1 : 0);
  //digitalWrite(16, on ? 1 : 0);
 
  delay(2208);
   
  pinMode(4, INPUT);
  //pinMode(14, INPUT);
  pinMode(7, INPUT);
  //pinMode(16, INPUT);
}
void loop() {
  setLatch(true);
  delay(500);
  setLatch(false);
  delay(500);
}
