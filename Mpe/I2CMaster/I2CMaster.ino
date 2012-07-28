/** 
 * Master
 */

#include <Wire.h>

void setup()
{
  Wire.begin();        // join i2c bus (address optional for master)
  Serial.begin(9600);  // start serial for output
  Serial.println("I2CMaster");
}

void loop()
{
    Serial.println("Data request");

  // Receive
  Wire.requestFrom(2, 6);    // request 6 bytes from slave device #2

  while(Wire.available())    // slave may send less than requested
  { 
    char c = Wire.read(); // receive a byte as character
    Serial.print(c);         // print the character
  }

  delay(500);

  // Send
  //Wire.beginTransmission(4); // transmit to device #4
  //Wire.write("x is ");        // sends five bytes
  //Wire.write(x);              // sends one byte  
  //Wire.endTransmission();    // stop transmitting

    Serial.println(".");

  //x++;
  //delay(500);
  
}

