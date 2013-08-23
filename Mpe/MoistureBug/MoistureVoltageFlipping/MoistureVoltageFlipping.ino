
/* The pin connected to the 100 ohm side */
#define voltageFlipPin1 9
/* The pin connected to the 50k-100k (measuring) side. */
#define voltageFlipPin2 10
/* The analog pin for measuring */
#define sensorPin 5


int flipTimer = 1000;


const int numReadings = 5;

int readings[numReadings];      // the readings from the analog input
int index = 0;                  // the index of the current reading
int total = 0;                  // the running total
int average = 0;                // the average

void setup()
{
  Serial.begin(9600);
  Serial.println("MoistureVoltageFlipping");
  pinMode(voltageFlipPin1, OUTPUT);
  pinMode(voltageFlipPin2, OUTPUT);
  pinMode(sensorPin, INPUT);
  for (int thisReading = 0; thisReading < numReadings; thisReading++)
    readings[thisReading] = 0;          
}

void setSensorPolarity(int flip)
{
  if (flip == -1) {
    digitalWrite(voltageFlipPin1, HIGH);
    digitalWrite(voltageFlipPin2, LOW);
  } 
  else if (flip == 1) {
    digitalWrite(voltageFlipPin1, LOW);
    digitalWrite(voltageFlipPin2, HIGH);  
  } 
  else {
    digitalWrite(voltageFlipPin1, LOW);
    digitalWrite(voltageFlipPin2, LOW);
  }
}

int readProbe() {

  setSensorPolarity(-1);
  delay(flipTimer);
  int val1 = analogRead(sensorPin);
  Serial.print("v1: ");Serial.println(val1);

  setSensorPolarity(1);
  delay(flipTimer);
  int val2 = 1023 - analogRead(sensorPin);
  Serial.print("v2: ");Serial.println(val2);
  
  setSensorPolarity(0);

  return ( val1 + val2 ) / 2;
}

void takeReading() {

    // subtract the last reading:
    total = total - readings[index];
    // read from the sensor:
    int reading = readProbe();
    readings[index] = reading;
    // add the reading to the total:
    total = total + readings[index];
    // advance to the next position in the array:
    index = index + 1;

    // if we're at the end of the array...
    if (index >= numReadings)
      // ...wrap around to the beginning: 
      index = 0;

    // calculate the average:
    average = total / numReadings;
    // send it to the computer as ASCII digits
    Serial.print("probe: ");
    Serial.print(reading);

    Serial.print(", avg: ");
    Serial.println(average);
    
    delay(10);
  
}

void loop() {

  takeReading();
  takeReading();
  takeReading();
  takeReading();
  
  delay(500);
}

