int CoSensorPower = 5; // digital power feed/indicator pin
int MthSensorPower = 6; 
int CoSensorData = 0; // measure pin (analog) for CO sensor
int MthSensorData = 1; 

int CoSensorVoltage = ;

const int PREHEAT_PERIOD = 600;
const int HEATING_PERIOD = 900; 

void setup() {
}

void loop () {


	Serial.println("PREHEAT");
	digitalWrite(CoSensorPower, HIGH);

	Serial.println("HEAT");
	analogWrite(CoSensorPower, 0x10);

	digitalWrite(CoSensorPower, LOW);

	Serial.print(analogRead(CoSensorVoltage));
	//Serial.print(analogRead(MthSensorData));
	Serial.print(' ');
	Serial.print(analogRead(CoSensorData));

}
