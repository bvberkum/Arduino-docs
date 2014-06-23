/**
 *
 * ATTiny25/45/85
 *                                     t85
 *                                 +----v----+
 *         (RST/dW/ADC0) D5/A0 PB5 | 1     8 | VCC
 *   (ADC3/XTAL1/PCINT3) D3/A3 PB3 | 2     7 | PB2 D2/A1 (ADC1/SCK/SCL/PCINT2)
 *   (ADC2/XTAL2/PCINT4) D4/A2 PB4 | 3     6 | PB1 *D1   (MISO/DO/PCINT1)
 *                             GND | 4     5 | PB0 *D0   (MOSI/DI/SDA/AREF/PCINT0)
 *                                 +---------+
 */

/* http://www.avrfreaks.net/index.php?name=PNphpBB2&file=viewtopic&t=96992 */

/* 
t85 port/signal
  PB0: DI, PB1: DO, PB2: SCK 

Shift register hookup:
  SRSCL:10: GND
  SRSCK:11: SCK
  RCLK:12: DI?
  OE:13: VCC
  SER:14: DO?
  
SR Plug
  SCK - SER
  MISO/DO - OE
  MOSI/DI - RCLK
  A2 - SRSCK
  A3
  
*/

void setup() 
{
  PORTB |= (1<<PB0);       // pull-up i.e. disabled 
  DDRB = (1<<PB2)|(1<<PB1)|(1<<PB0); // o/p pins 
  USICR = (1<<USIWM0)|(1<<USICS1)|(1<<USICLK); 
  PORTB &= ~(1<<PB0);    // enable Slave 
  USICR = 0;            // disable USI pins 
  PORTB |= (1<<PB0);     // disable Slave 

}

unsigned char spi(unsigned char val) 
{ 
  USIDR = val; 
  USISR = (1<<USIOIF); 
  do { 
    USICR = (1<<USIWM0)|(1<<USICS1)|(1<<USICLK)|(1<<USITC); 
  } 
  while ((USISR & (1<<USIOIF)) == 0); 
  return USIDR; 
}

byte x = 0;
void loop() {
  delay(500);
  spi(x);
  x++;
}

