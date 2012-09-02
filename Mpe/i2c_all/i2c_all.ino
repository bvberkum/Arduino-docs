#include <Wire.h>
#include <JeeLib.h>

#include <LiquidCrystal_I2C.h>

PortI2C myBus (1);
//PortI2C myBus (1 /*, PortI2C::KHZ400 */);

DimmerPlug dimmer (myBus, 0x47); // my dimmer plug has all the addrees jumpers right sides soldered
DeviceI2C expander (myBus, 0x20); // also works with output plug if 0x26/0x27
//LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display    
LuxPlug lux (myBus, 0x39);
byte highGain;

int level = 0x1FFF;


enum {
  MCP_IODIR, MCP_IPOL, MCP_GPINTEN, MCP_DEFVAL, MCP_INTCON, MCP_IOCON,
  MCP_GPPU, MCP_INTF, MCP_INTCAP, MCP_GPIO, MCP_OLAT
};

static void exp_setup () {
  expander.send();
  expander.write(MCP_IODIR);
  expander.write(0); // all outputs
  expander.stop();
}

static void exp_write (byte value) {
  expander.send();
  expander.write(MCP_GPIO);
  expander.write(value);
  expander.stop();
}

static  void   dimmer_init() {
  dimmer.begin();
  // set each channel to max brightness
  dimmer.setMulti(dimmer.PWM0, 255, 255, 255, 255,
  255, 255, 255, 255,
  255, 255, 255, 255,
  255, 255, 255, 255, -1);
  // set up for group blinking
  dimmer.setReg(dimmer.MODE2, 0x34);
  // blink rate: 0 = very fast, 255 = 10s
  dimmer.setReg(dimmer.GRPFREQ, 50);
  // blink duty cycle: 0 = full on, 255 = full off
  dimmer.setReg(dimmer.GRPPWM, 240);
  // let the chip do its thing for a while
  delay(10000);
  // set up for group dimming
  dimmer.setReg(dimmer.MODE2, 0x14);
  // gradually decrease brightness to minimum
  for (byte i = 100; i < 255; ++i) {
    dimmer.setReg(dimmer.GRPPWM, i);
    delay(100);
  }
}


void setup()
{
  Serial.begin(38400);
  Serial.println("testing ");

  //pinMode(4, OUTPUT);

  //Wire.begin();
  lux.begin();
  lux.setGain(1);
  //lcd.init();                      // initialize the lcd 
  //lcd.backlight();
  exp_setup();
}


void loop()
{
  Serial.print('.');
  //delay(200);
  //digitalWrite(4, HIGH);
  //delay(200);
  //digitalWrite(4, LOW);

  // running light
  for (byte i = 0; i < 8; ++i) {
    exp_write(1 << i);
    delay(100);
  Serial.print('.');
  }
  //Wire.beginTransmission(address);
  //Wire.send(REGISTER_CONFIG);
  //  Connect to device and send two bytes
  //Wire.send(0xff & dir);  //  low byte
  //Wire.send(dir >> 8);    //  high byte
  //Wire.endTransmission();

  /*
  if (Serial.available()) {
   // wait a bit for the entire message to arrive
   delay(100);
   // clear the screen
   lcd.clear();
   // read all the available characters
   while (Serial.available() > 0) {
   // display each character to the LCD
   uint8_t s=Serial.read();
   Serial.println('ACK ');
   Serial.println(s);
   lcd.write(s);
   }
   }*/

  //delay(1000);


  Serial.print('.');

// Handle Dimmer plug
  byte brightness = ++level;
  if (level & 0x100)
    brightness = ~ brightness;
  byte r = level & 0x0200 ? brightness : 0;
  byte g = level & 0x0400 ? brightness : 0;
  byte b = level & 0x0800 ? brightness : 0;
  byte w = level & 0x1000 ? brightness : 0;
  // set all 16 registers in one sweep
  dimmer.setMulti(dimmer.PWM0, w, b, g, r,
  w, b, g, r,
  w, b, g, r,
  w, b, g, r, -1);

  Serial.println('.');

  const word* photoDiodes = lux.getData();
  Serial.print("LUX ");
  Serial.print(photoDiodes[0]);
  Serial.print(' ');
  Serial.print(photoDiodes[1]);
  Serial.print(' ');
  Serial.print(lux.calcLux());
  Serial.print(' ');
  Serial.println(highGain);
  /*
   // need to wait after changing the gain
   //  see http://talk.jeelabs.net/topic/608
   highGain = ! highGain;
   lux.setGain(highGain);
   */



}

