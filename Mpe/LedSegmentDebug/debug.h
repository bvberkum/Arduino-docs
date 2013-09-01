
// alternatively use SPI http://gammon.com.au/forum/?id=11518

#include <avr/pgmspace.h>
#include <util/delay.h>


#define SER_DDR  	DDRD		// SER/Data in - D5 - PDIP pin11
#define SER_PORT 	PORTD
#define SER_NR 		PORTD5//JP2D/D5/PD5

#define CLK_DDR		DDRD		// SCK/Data Clock - D6 - PDIP pin 12
#define CLK_PORT	PORTD
#define CLK_NR		PORTD6

#define LE_DDR		DDRD		// RCK/Latch Enable - D7 - PDIP pin 13
#define LE_PORT		PORTD
#define LE_NR		PORTD7//JP4D//PD7

#define DIGITS          4
#define DIGITS_DDR      DDRC
#define DIGITS_PORT     PORTC

#define true 		1
#define false 		0

#define ACTIVE_STATE    false              // low (0) for common anode, set to 1 for common cathode


#define ser_high()		SER_PORT |= (1<<SER_NR)
#define ser_low()		SER_PORT &= ~(1<<SER_NR)
void set_ser(int x) {
  ((!ACTIVE_STATE && x) || (ACTIVE_STATE && !x)) ? ser_low() : ser_high();
}

#define clk_high()		CLK_PORT |=  (1<<CLK_NR)
#define clk_low()		CLK_PORT &= ~(1<<CLK_NR)

#define le_high()		LE_PORT |= (1<<LE_NR)
#define le_low()		LE_PORT &= ~(1<<LE_NR)

const uint8_t digits[] = {
  16, // PC2/A2
  17,
  18,
  19 // PC5/A5
};

void set_digit(int x, int state) {
   if (state) 
     DIGITS_PORT |= (1<<(digits[DIGITS-x-1]-14));
   else
     DIGITS_PORT &= ~(1<<(digits[DIGITS-x-1]-14)); // set first to low
}

/*
uint8_t test[]  PROGMEM = {
 //ABCDEFG.
 0b10000000,           // Value 0
 0b01000000,		// Value 1
 0b00100000, 		// Value 2
 0b00010000,		// Value 3
 0b00010000,		// Value 4
 0b00001000,		// Value 5
 0b00000100,		// Value 6
 0b00000010,		// Value 7
 0b00000001,		// Value 8
 0b11111110,		// Value 9
 0b11111100,           // Value A
 0b11111000,		// Value B
 0b11110000,		// Value C	
 0b11100000,		// Value D
 0b11000000,		// Value E
 0b10000000,		// Value F
 }; */

uint8_t segmentcontrol[]  PROGMEM = {
  //ABCDEFG.
  0b11111100,           // Value 0
  0b01100000,		// Value 1
  0b11011010, 		// Value 2
  0b11110010,		// Value 3
  0b01100110,		// Value 4
  0b10110110,		// Value 5
  0b10111110,		// Value 6
  0b11100000,		// Value 7
  0b11111110,		// Value 8
  0b11110110,		// Value 9
  0b11101110,           // Value A
  0b00111110,		// Value B
  0b00011010,		// Value C	
  0b01111010,		// Value D
  0b10011110,		// Value E
  0b10001110,		// Value F
}; 	

// function prototypes
void initdebug();
void debug_chars(uint8_t tmp, int dot);
void debug(uint8_t tmp, int dot);
void writesegment(int dot, uint16_t sevensegment);


void initdebug()			// Enable the output ports
{
  SER_DDR |= (1<<SER_NR);
  CLK_DDR |= (1<<CLK_NR);
  LE_DDR |= (1<<LE_NR);

  for (int i=0;i<DIGITS;i++) {
    DIGITS_DDR |= (1<<(digits[DIGITS-i-1]-14));
    set_digit(i, 0);
  }
}

void debug_chars(uint8_t digit, int dot = 1) {
  for (int x=0;x<DIGITS;x++) {
      // stay below ~5000us

    set_digit(x, 1);
    debug(digit, dot);
    digit >>= 8;
    _delay_us(500);
    set_digit(x, 0);
    _delay_us(800);
  }
  _delay_us(9250);
}

void debug(unsigned char ch, int dot = 1)
{
  writesegment(dot,
  ( pgm_read_byte(segmentcontrol + (ch/16)) ) * 256 + pgm_read_byte(segmentcontrol + (ch%16)) 
  );
}

void writesegment(int dot, uint16_t sevensegment)
{
  uint8_t i;
  for(i=0; i<8; i++)
  {
    set_ser(0);  // Serial input (of 74HC595) low
    if (i == 0) {
      //if (i % 8 == dot)
      //  set_ser(1);
    }
    else if( sevensegment & 0x1 ) {
      set_ser(1);	                // If first bit is set, switch serial input high
    }
    clk_high();				// trigger clock to shift in,
    _delay_us(2);			// wait a bit	
    clk_low();				// switch clock low again.
    sevensegment >>= 1;			// Shift value of sevensegment one to the left.
  }
  le_high();			        // All data is in the shiftregister, now latch it to put it to the output
  _delay_us(2);			// Wait a bit
  le_low();			        // And lower the bit again.
}

///
/*
int test_step = 0;
 void testsegments()
 {
 uint16_t sevensegment = test[test_step];
 test_step++;
 if (test_step==16) {
 test_step = 0;
 }
 
 for (int i=0; i<8; i++) {
 if (!ACTIVE_STATE)
 ser_high();
 else
 ser_low();
 if( sevensegment & 0x1 ) {
 if (!ACTIVE_STATE)
 ser_low();
 else
 ser_high();		        // If first bit is set, switch serial input high
 }
 clk_high();				// trigger clock to shift in,
 _delay_us(10);			// wait a bit	
 clk_low();				// switch clock low again.
 sevensegment >>= 1;			// Shift value of sevensegment one to the left.
 }
 le_high();			        // All data is in the shiftregister, now latch it to put it to the output
 _delay_us(10);			// Wait a bit
 le_low();			        // And lower the bit again.
 }
 */

