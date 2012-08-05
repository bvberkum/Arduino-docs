/* Change the following defines to fit your individual application */ 

#define SER_DDR  	DDRA		// Data in
#define SER_PORT 	PORTA
#define SER_NR 		PA6	

#define CLK_DDR		DDRA		// Data Clock
#define CLK_PORT	PORTA
#define CLK_NR		PA4

#define LE_DDR		DDRA		// Latch Enable
#define LE_PORT		PORTA
#define LE_NR		PA7


/*###################################################################
*	
*	When implementing just adjust the defines above.
*
*	Don't change anything in the code below! 
*	
*
*###################################################################*/

#include <avr/pgmspace.h>
#include <util/delay.h>
#ifndef F_CPU
// #warning "F_CPU was not defined, is now set to 1MHz"
#define F_CPU 1000000UL    // Systemclock in Hz - Definition as unsigned long 
#endif


#define true 		1
#define false 		0

#define ser_high()		SER_PORT |= (1<<SER_NR)
#define ser_low()		SER_PORT &= ~(1<<SER_NR)
#define clk_high()		CLK_PORT |=  (1<<CLK_NR)
#define clk_low()		CLK_PORT &= ~(1<<CLK_NR)
#define le_high()		LE_PORT |= (1<<LE_NR)
#define le_low()		LE_PORT &= ~(1<<LE_NR)

/* Adjust segmentcontrol if you are using different hardware setup. */
uint8_t segmentcontrol[]  PROGMEM = { 	0b00111111, 		// Value 0
					0b00001001,		// Value 1
					0b01011110,		// Value 2
					0b01011011,		// Value 3
					0b01101001,		// Value 4
					0b01110011,		// Value 5
					0b01110111,		// Value 6
					0b00011001,		// Value 7
					0b01111111,		// Value 8
					0b01111011,		// Value 9
					0b01111101,		// Value A
					0b01100111,		// Value B
					0b00110110,		// Value C	
					0b01001111,		// Value D
					0b01110110,		// Value E
					0b01110100		// Value F
				}; 	// code to display the value

// function prototypes
void initdebug();
void debug(uint8_t tmp);
void writesegment(uint16_t sevensegment);


void initdebug()			// Enable the output ports
{
SER_DDR |= (1<<SER_NR);
CLK_DDR |= (1<<CLK_NR);
LE_DDR |= (1<<LE_NR);
}


void debug(uint8_t tmp)
{
	writesegment(( pgm_read_byte(segmentcontrol + (tmp/16)) )*256 + pgm_read_byte(segmentcontrol + (tmp%16)) );
}

void writesegment(uint16_t sevensegment)
{
uint8_t i;

for(i=0; i<16; i++)
	{
	ser_low();					// Serial input (of 74HC595) low
      	if( sevensegment & 0x1 ) ser_high();		// If first bit is set, switch serial input high
      		clk_high();				// trigger clock to shift if in.
		_delay_us(10);				// Wait a bit	
	      	clk_low();				// switch clock low again
      		sevensegment >>= 1;			// shift value of sevensegment one to the left.
		}
	le_high();			// All data is in the shiftregister, now latch it to put it to the output
	_delay_us(10);			// Wait a bit
	le_low();			// And lower the bit again.

}

