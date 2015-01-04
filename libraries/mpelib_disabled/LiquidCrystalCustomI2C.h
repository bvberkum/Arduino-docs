// 2013 <dev@dotmpe.com>

#ifndef LiquidCrystalCustomI2C_h
#define LiquidCrystalCustomI2C_h

#include "<PortsLCD.h>"


// @file
// LiquidCrystalCustomI2C,
//

/// Interface to character LCD's connected via 2 I/O pins using software I2C.
/// This class allows driving an LCD connected via I2C using an LCD Plug, which
/// is in turn based on an MCP23008 I2C I/O expander chip and some other parts.
/// The available functions include all those of the LiquidCrystal class.

class LiquidCrystalCustomI2C : public LiquidCrystal {
  DeviceI2C device;
public:
  LiquidCrystalCustomI2C (const PortI2C& p, byte addr =0x24);
  
  // this display can also turn the back-light on or off
  void backlight();
  void noBacklight();
protected:
  virtual void config();
  virtual void send(byte, byte);
  virtual void write4bits(byte);
};

#endif


