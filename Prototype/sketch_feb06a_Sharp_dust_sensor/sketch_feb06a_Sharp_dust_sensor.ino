int dustVal = 0;

int dustPin = 2;
int ledPin = 6;

//int delayTime = 280;
int delayTime2 = 40;

int offTime = 9680;
int cnt = 0;

unsigned long lastMeasure;
unsigned long delayTime;

void setup(){
  Serial.begin(57600);
  pinMode(ledPin,OUTPUT);
}

void measure() {/*
  delayTime = offTime;
  if (lastMeasure) {
    delayTime -= (millis()*10) - lastMeasure;
  }
  delayMicroseconds(delayTime);*/
  
  // ledPower is any digital pin on the arduino connected to Pin 3 on the sensor
  digitalWrite(ledPin, LOW); // power on the LEDœ
  delayMicroseconds(delayTime);
  dustVal=analogRead(dustPin); // read the dust value via pin 5 on the sensor
  delayMicroseconds(delayTime2);
  digitalWrite(ledPin, HIGH); // turn the LED off
  //lastMeasure = millis() * 10;
  delayMicroseconds(offTime);
}

void loop() {
  measure();
  //cnt += 1;
  //if (cnt % 25 == 0)
  Serial.println(dustVal);
  //delay(2);
  //delay(1000);
}

