#include <avr/io.h>
#include <avr/signal.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "lcd.h"
#include "serial.h"
#include "printf.h"
#include "mydefs.h"
#include    <util/delay.h>

volatile uint8_t 	timerOverflow 	= 0;  	// Timer1 Overflows 
volatile uint16_t 	Start= 0, End = 0;    	// 	
volatile uint8_t	update;                 // 	Flag 

//------------------------------------------------------------------------//
//--------------------------------------------------------------------------------- 
// Timer1 ICP1 
ISR (TIMER1_CAPT_vect) 
{ 
	//jnut game
	static uint8_t flag = 1; 

	if (update)
		return; 

	if(flag) 
	{ 
		Start 			= ICR1; 		//Read value counter
		timerOverflow = 0; 
		flag 			= 0;       
	} 
	else 
	{ 
		End		 	= 	ICR1; 
		update 		= 	1;         
		flag 		= 	1;    
	} 
} 
//------------------------------------------------------------------------//
//---------------------------------------------------------------------------------- 
ISR(TIMER1_OVF_vect ) 
{ 
	timerOverflow++; 
}
//------------------------------------------------------------------------//
//------------------------------------------------------------------------//
void delay_led (unsigned char d)
{
	unsigned char nDelay;
	for (nDelay = 0; nDelay < d; nDelay++)
	{
		_delay_ms (1000);
	}
}
//------------------------------------------------------------------------//
//------------------------------------------------------------------------//
void init_timer1 (void)
{
	//Timer1 16bit, 
	//just to use timer 16 bit / 8 bit not enough
	TCCR1B 	|= _BV(CS10) | _BV(ICES1); 	//Prescaling & rising edge detect
	TIMSK 	|= _BV(TICIE1) | _BV(TOIE1); 	//Overflow interrupt 
}
//------------------------------------------------------------------------//
//------------------------------------------------------------------------//
int main(void)
{		

	double 	freq = 0.0;
	uint8_t	lcd_buf[18];
	
	DDRG  |= _BV(LED);
		
	DDRD  =  (0 << PD4)	;				//ICP1 Capture
	
	ser_init();							//init serial port
		
	LED_PORT  &= ~_BV(LED);			//off led 
		
	init_lcd ();
	
	lcd_gotoxy (1,1);
	
	lcd_printf ("JX-MEGA128 METER");
	
	init_timer1 ();

	sei();								//enable interrupt

	while (1)
	{
		if (update)
		{
			freq  = (timerOverflow * 65536) + End - Start; 
			freq  = F_CPU / freq;
			
			update = 1;								//stop interrupt for display
													
			memset (lcd_buf, 0, sizeof lcd_buf);
			if (freq > 1000)	//kHz
			{
				freq /= 1000;
				sprintf (lcd_buf,"Freq= %0.02f kHz ",freq);
			}
			else
			{
				sprintf (lcd_buf, "Freq= %0.02f Hz ",freq);
			}
			lcd_gotoxy (2,1);
			lcd_printf (lcd_buf);
			update = 0;
		}
	}
		
	while (1);
}
