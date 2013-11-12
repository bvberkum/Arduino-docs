//**************************************************************//
//  Name    : shiftOutCode, Hello World                                
//  Author  : Carlyn Maw,Tom Igoe, David A. Mellis 
//  Date    : 25 Oct, 2006    
//  Modified: 23 Mar 2010                                 
//  Version : 2.0                                             
//  Notes   : Code for using a 74HC595 Shift Register           //
//          : to count from 0 to 255                           
//****************************************************************

#include <avr/io.h>
 
#define SHIFT_REGISTER DDRB
#define SHIFT_PORT PORTB
#define DATA (1<<PB3)     //MOSI (SI)
#define LATCH (1<<PB2)        //SS   (RCK)
#define CLOCK (1<<PB5)        //SCK  (SCK)

#define CHAR1 6
#define CHAR2 7

void write(int num) {
	//Pull LATCH low (Important: this is necessary to start the SPI transfer!)
	SHIFT_PORT &= ~LATCH;

	//Shift in some data
	SPDR = num & 0xFF;
	//Wait for SPI process to finish
	while(!(SPSR & (1<<SPIF)));

	//Toggle latch to copy data to the storage register
	SHIFT_PORT |= LATCH;
	SHIFT_PORT &= ~LATCH;
}

void clear() {
//	write(1);
	write(1);
	write(1);
	write(1);
	write(1);
	write(1);
	write(1);
	write(1);
	write(1);
}

int seq[14] = {1,2,4,8,16,32,32,16,8,4,2,1};

void setup()
{
	//Setup IO
	SHIFT_REGISTER |= (DATA | LATCH | CLOCK); //Set control pins as outputs
	SHIFT_PORT &= ~(DATA | LATCH | CLOCK);        //Set control pins low

	//Setup SPI
	SPCR = (1<<SPE) | (1<<MSTR);  //Start SPI as Master

	pinMode(6, OUTPUT);
	pinMode(7, OUTPUT);
	digitalWrite(6, 0);
	digitalWrite(7, 0);
}

uint8_t led_dp = 0b01111111;
uint8_t led_seg_num[10] = {
	0b11111001,
	0b10100100,
	0b10110000,
	0b10011001,
	0b10010010,
	0b10000010,
	0b11111000,
	0b10000000,
	0b10010000,
	0b11000000
};

void loop()
{
	for (int i=0;i<6;i++) {
		for (int x=0;x<24;x++) {
			// I have common-anode so I reverse the values by substracting fom
			// 0xff and have one led lit up instead of all
			write(0xff-seq[i]);
			digitalWrite(CHAR2, HIGH);
			delay(4);
			digitalWrite(CHAR2, LOW);
			write(seq[i]);
			digitalWrite(CHAR1, HIGH);
			delay(4);
			digitalWrite(CHAR1, LOW);
		}
	}
}
