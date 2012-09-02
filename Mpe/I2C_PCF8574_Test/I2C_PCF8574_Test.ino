#include <Wire.h>



void setup()
{
  Wire.begin();
  Serial.begin(38400);
  Serial.println("testing ");
}

void loop()
{
  Wire.beginTransmission(address);
  //Wire.send(REGISTER_CONFIG);

  //  Connect to device and send two bytes
  //Wire.send(0xff & dir);  //  low byte
  //Wire.send(dir >> 8);    //  high byte

  Wire.endTransmission();
}

