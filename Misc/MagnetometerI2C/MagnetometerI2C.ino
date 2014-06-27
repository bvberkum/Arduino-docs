/* FILE:    ARD_HMC5803L_GY273_Example
   DATE:    23/10/13
   VERSION: 0.1

This is an example of how to use the Hobby Components GY-273 module (HCMODU0036) which 
uses a Honeywell HMC5883L 3-Axis Digital Compass IC. The IC uses an I2C interface to 
communicate which is compatible with the standard Arduino Wire library.

This example demonstrates how to initialise and read the module in single shot 
measurement mode. It will continually trigger single measurements and output the 
results for the 3 axis to the serial port.


CONNECTIONS:

MODULE    ARDUINO
VCC       3.3V
GND       GND
SCL       A5
SDA       A4
DRDY      N/A


You may copy, alter and reuse this code in any way you like, but please leave
reference to HobbyComponents.com in your comments if you redistribute this code.
This software may not be used for the purpose of promoting or selling products 
that directly compete with Hobby Components Ltd's own range of products.

THIS SOFTWARE IS PROVIDED "AS IS". HOBBY COMPONENTS MAKES NO WARRANTIES, WHETHER
EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, ACCURACY OR LACK OF NEGLIGENCE.
HOBBY COMPONENTS SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR ANY DAMAGES,
INCLUDING, BUT NOT LIMITED TO, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY
REASON WHATSOEVER. */

/* Include the standard Wire library */
#include <Wire.h>

/* The I2C address of the module */
#define HMC5803L_Address 0x1E

/* Register address for the X Y and Z data */
#define X 3
#define Y 7
#define Z 5

void setup() 
{
  Serial.begin(57600);
  /* Initialise the Wire library */
  Wire.begin();
  
  /* Initialise the module */ 
  Init_HMC5803L();
}

void loop() 
{
  /* Read each sensor axis data and output to the serial port */
  Serial.print(HMC5803L_Read(X));
  Serial.print(" ");
  Serial.print(HMC5803L_Read(Y));
  Serial.print(" ");
  Serial.println(HMC5803L_Read(Z));
  
  /* Wait a little before reading again */
  delay(200);
}


/* This function will initialise the module and only needs to be run once
   after the module is first powered up or reset */
void Init_HMC5803L(void)
{
  /* Set the module to 8x averaging and 15Hz measurement rate */
  Wire.beginTransmission(HMC5803L_Address);
  Wire.write(0x00);
  Wire.write(0x70);
          
  /* Set a gain of 5 */
  Wire.write(0x01);
  Wire.write(0xA0);
  Wire.endTransmission();
}


/* This function will read once from one of the 3 axis data registers
and return the 16 bit signed result. */
int HMC5803L_Read(byte Axis)
{
  int Result;
  
  /* Initiate a single measurement */
  Wire.beginTransmission(HMC5803L_Address);
  Wire.write(0x02);
  Wire.write(0x01);
  Wire.endTransmission();
  delay(6);
  
  /* Move modules the resiger pointer to one of the axis data registers */
  Wire.beginTransmission(HMC5803L_Address);
  Wire.write(Axis);
  Wire.endTransmission();
   
  /* Read the data from registers (there are two 8 bit registers for each axis) */  
  Wire.requestFrom(HMC5803L_Address, 2);
  Result = Wire.read() << 8;
  Result |= Wire.read();

  return Result;
}
