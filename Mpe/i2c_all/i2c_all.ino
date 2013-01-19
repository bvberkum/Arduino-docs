/*

2012-1-19
  Made LiquidCrystalTmp_I2C copy to prevent collision with JeeLib.

*/
#include <Wire.h>
#include <JeeLib.h>

#include <LiquidCrystalTmp_I2C.h>


PortI2C myBus (1);
//PortI2C myBus (1 /*, PortI2C::KHZ400 */);

//DimmerPlug dimmer (myBus, 0x47); // my dimmer plug has all the addrees jumpers right sides soldered
//DeviceI2C expander (myBus, 0x20); // also works with output plug if 0x26/0x27

/** LCD */
#define I2C_ADDR    0x20
#define BACKLIGHT_PIN     7
#define En_pin  4
#define Rw_pin  5
#define Rs_pin  6
#define D4_pin  0
#define D5_pin  1
#define D6_pin  2
#define D7_pin  3
#define LED_OFF 0
#define LED_ON  1
LiquidCrystalTmp_I2C  lcd(I2C_ADDR, En_pin,Rw_pin,Rs_pin,D4_pin,D5_pin,D6_pin,D7_pin);

/** Lux Plug */
LuxPlug lux (myBus, 0x39);
byte highGain;
int level = 0x1FFF;


#if defined(ARDUINO) && ARDUINO >= 100
#define printByte(args)  write(args);
#else
#define printByte(args)  print(args,BYTE);
#endif
/*
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
 
 void cycle_expander()
 {
  // running light
  for (byte i = 0; i < 8; ++i) {
    exp_write(1 << i);
    delay(100);
    Serial.print('.');
  }
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
 
void cycle_dimmer()
{
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
}
 */

void i2c_lcd_init()
{
  lcd.begin(16,2);
  lcd.setBacklightPin(BACKLIGHT_PIN,NEGATIVE);
  lcd.setBacklight(LED_ON);
/*
  lcd.createChar(0, bell);
  lcd.createChar(1, note);
  lcd.createChar(2, clock);
  lcd.createChar(3, heart);
  lcd.createChar(4, duck);
  lcd.createChar(5, check);
  lcd.createChar(6, cross);
  lcd.createChar(7, retarrow);*/
  lcd.home();
}

void lux_init()
{
  lux.begin();
  lux.setGain(1);
}

void cycle_lux()
{
  const word* photoDiodes = lux.getData();
  int lux_value = lux.calcLux();
 
  Serial.print("LUX ");
  Serial.print(photoDiodes[0]);
  Serial.print(' ');
  Serial.print(photoDiodes[1]);
  Serial.print(' ');
  Serial.print(lux.calcLux());
  Serial.print(' ');
  Serial.println(highGain);
  
  lcd.setCursor(0,1);
  //lcd.clear();
  /*
  lcd.print("D0 / D1 / lux:");
  lcd.setCursor(0,1);
  lcd.print(photoDiodes[0]);
  lcd.print(' ');
  lcd.print(photoDiodes[1]);
  lcd.print(' ');*/
  lcd.print(lux_value);
  lcd.print(" lux      ");
  //lcd.print(highGain)
}


void setup()
{
  Serial.begin(57600);
  Serial.println("I2C_ALL");

  lux_init();
  i2c_lcd_init();
  //exp_setup();
  

  lcd.setCursor(0,0);
  lcd.print("I2C All");
}

void loop()
{
  Serial.println('.');

  //cycle_dimmer();
  //cycle_expander();
  cycle_lux();
  
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

  /*
   // need to wait after changing the gain
   //  see http://talk.jeelabs.net/topic/608
   highGain = ! highGain;
   lux.setGain(highGain);
   */

  delay(1000);
}

