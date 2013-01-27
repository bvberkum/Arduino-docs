/**
 * LiquidCrystalCustomI2C: a HD44780 to I2C bridge using a PCF8574*
 * I/O expander. The HD44780 is found on many or most 16pin
 * Liquid-Crystal Character Displays.
 */

#include "LiquidCrystalCustomI2C.h"

// CONSTANT  definitions
// ---------------------------------------------------------------------------

// flags for backlight control
/*
 @abstract   LCD_NOBACKLIGHT
 @discussion NO BACKLIGHT MASK
 */
#define LCD_NOBACKLIGHT 0x00

/*
 @abstract   LCD_BACKLIGHT
 @discussion BACKLIGHT MASK used when backlight is on
 */
#define LCD_BACKLIGHT   0xFF


// Default library configuration parameters used by class constructor with
// only the I2C address field.
// ---------------------------------------------------------------------------
/*!
 @defined 
 @abstract   Enable bit of the LCD
 @discussion Defines the IO of the expander connected to the LCD Enable
 */
#define EN 6  // Enable bit

/*!
 @defined 
 @abstract   Read/Write bit of the LCD
 @discussion Defines the IO of the expander connected to the LCD Rw pin
 */
#define RW 5  // Read/Write bit

/*!
 @defined 
 @abstract   Register bit of the LCD
 @discussion Defines the IO of the expander connected to the LCD Register select pin
 */
#define RS 4  // Register select bit

/*!
 @defined 
 @abstract   LCD dataline allocation this library only supports 4 bit LCD control
 mode.
 @discussion D4, D5, D6, D7 LCD data lines pin mapping of the extender module
 */
#define D4 0
#define D5 1
#define D6 2
#define D7 3


// CONSTRUCTORS
// ---------------------------------------------------------------------------
LiquidCrystalCustomI2C::LiquidCrystalCustomI2C( const PortI2C& p, uint8_t addr )
    : device (p, addr) {

  _displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;
  config(addr, EN, RW, RS, D4, D5, D6, D7);
  begin(16, 2); 
}

// XXX:
LiquidCrystalCustomI2C::LiquidCrystalCustomI2C(uint8_t lcd_Addr, uint8_t backlighPin, 
                                     t_backlighPol pol = POSITIVE)
{
   config(lcd_Addr, EN, RW, RS, D4, D5, D6, D7);
   setBacklightPin(backlighPin, pol);
}

LiquidCrystalCustomI2C::LiquidCrystalCustomI2C(uint8_t lcd_Addr, uint8_t En, uint8_t Rw,
                                     uint8_t Rs)
{
   config(lcd_Addr, En, Rw, Rs, D4, D5, D6, D7);
}

LiquidCrystalCustomI2C::LiquidCrystalCustomI2C(uint8_t lcd_Addr, uint8_t En, uint8_t Rw,
                                     uint8_t Rs, uint8_t backlighPin, 
                                     t_backlighPol pol = POSITIVE)
{
   config(lcd_Addr, En, Rw, Rs, D4, D5, D6, D7);
   setBacklightPin(backlighPin, pol);
}

LiquidCrystalCustomI2C::LiquidCrystalCustomI2C(uint8_t lcd_Addr, uint8_t En, uint8_t Rw,
                                     uint8_t Rs, uint8_t d4, uint8_t d5,
                                     uint8_t d6, uint8_t d7 )
{
   config(lcd_Addr, En, Rw, Rs, d4, d5, d6, d7);
}

LiquidCrystalCustomI2C::LiquidCrystalCustomI2C(uint8_t lcd_Addr, uint8_t En, uint8_t Rw,
                                     uint8_t Rs, uint8_t d4, uint8_t d5,
                                     uint8_t d6, uint8_t d7, uint8_t backlighPin, 
                                     t_backlighPol pol = POSITIVE )
{
   config(lcd_Addr, En, Rw, Rs, d4, d5, d6, d7);
   setBacklightPin(backlighPin, pol);
}
// /XXX


void LiquidCrystalCustomI2C::config (uint8_t lcd_Addr, 
								uint8_t En, uint8_t Rw, uint8_t Rs, 
                                uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7 )
{
   _Addr = lcd_Addr;
   
   _backlightPinMask = 0;
   _backlightStsMask = LCD_NOBACKLIGHT;
   _polarity = POSITIVE;
   
   _En = ( 1 << En );
   _Rw = ( 1 << Rw );
   _Rs = ( 1 << Rs );
   
   // Initialise pin mapping
   _data_pins[0] = ( 1 << d4 );
   _data_pins[1] = ( 1 << d5 );
   _data_pins[2] = ( 1 << d6 );
   _data_pins[3] = ( 1 << d7 );   
}

void LiquidCrystalCustomI2C::backlight() {
   //setBacklight(BACKLIGHT_ON);
}

void LiquidCrystalCustomI2C::noBacklight() {
   //setBacklight(BACKLIGHT_OFF);
}

void LiquidCrystalCustomI2C::send(byte value, byte mode) {
  if (mode != 0)
    mode = MCP_REGSEL;
  write4bits((value >> 4) | mode);
  write4bits((value & 0x0F) | mode);
}

void LiquidCrystalCustomI2C::write4bits(byte value) {
  value |= MCP_BACKLIGHT | MCP_ENABLE;
  device.send();
  device.write(MCP_GPIO);
  device.write(value);
  device.write(value ^ MCP_ENABLE);
  device.write(value);
  device.stop();
}



// low level data pushing commands
//----------------------------------------------------------------------------

//
// send - write either command or data
void LiquidCrystalCustomI2C::send(uint8_t value, uint8_t mode) 
{
   // No need to use the delay routines since the time taken to write takes
   // longer that what is needed both for toggling and enable pin an to execute
   // the command.
   
   if ( mode == FOUR_BITS )
   {
      write4bits( (value & 0x0F), COMMAND );
   }
   else 
   {
      write4bits( (value >> 4), mode );
      write4bits( (value & 0x0F), mode);
   }
}

//
// write4bits
void LiquidCrystalCustomI2C::write4bits ( uint8_t value, uint8_t mode ) 
{
   uint8_t pinMapValue = 0;
   
   // Map the value to LCD pin mapping
   // --------------------------------
   for ( uint8_t i = 0; i < 4; i++ )
   {
      if ( ( value & 0x1 ) == 1 )
      {
         pinMapValue |= _data_pins[i];
      }
      value = ( value >> 1 );
   }
   
   // Is it a command or data
   // -----------------------
   if ( mode == DATA )
   {
      mode = _Rs;
   }
   
   pinMapValue |= mode | _backlightStsMask;
   pulseEnable ( pinMapValue );
}

//
// pulseEnable
void LiquidCrystalCustomI2C::pulseEnable (uint8_t data)
{
   _i2cio.write (data | _En);   // En HIGH
   _i2cio.write (data & ~_En);  // En LOW
}

