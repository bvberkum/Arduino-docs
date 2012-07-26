#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdarg.h>
#include <string.h>
#include <avr/pgmspace.h>

#include "mydefs.h"
#include <util/delay.h>
#include "lcd.h"


void delay_us (unsigned long ndelay)
{
	unsigned long x;
	for (x= 0;x < (ndelay*250);x++);   // don't change 250 loop
}
//---------------------------------------------------------------------------------//
static void toggle_e(void)
{
    lcd_e_high();
	delay_us (5);
    lcd_e_low();
	delay_us (5);
};
//---------------------------------------------------------------------------------//
void LCD_Write_cmd (unsigned char cmd)
{
	unsigned char temp;
	
	temp 	= 	LCD_PORT;
	temp	=	temp & 0x0f; // 0 = out , 1 = in
	
	lcd_e_low ();
	LCD_PORT = (cmd & 0xf0) | temp; 	    // Load high-data to port
	lcd_rs_low ();					        // command = 0;
	lcd_e_toggle ();

	LCD_PORT =((cmd & 0x0f) << 4) | temp; 	// Load low-data to port
	lcd_rs_low ();						    // command = 0;
	lcd_e_toggle ();
}
//---------------------------------------------------------------------------------//
/* Game! */
void LCD_Write_data (unsigned char dat)
{
	unsigned char temp;
	
	temp 	= 	LCD_PORT;
	temp	=	temp & 0x0f; 			    // 0 = out , 1 = in
	
	lcd_e_low ();
	LCD_PORT = (dat & 0xf0) | temp; 		// Load high-data to port
	lcd_rs_high ();						    // data = 1;
	lcd_e_toggle ();

	LCD_PORT =((dat & 0x0f) << 4) | temp; 	// Load low-data to port
	lcd_rs_high ();						    // data = 1;
	lcd_e_toggle ();
}
//---------------------------------------------------------------------------------//
void lcd_gotoxy (char row, char col)
{
	switch (row)
	{
		case 1 : LCD_Write_cmd (0x80 + col -1); 
					  break;
		case 2 : LCD_Write_cmd (0xC0 + col -1); 
					  break;
		default : break;
	}
}
//---------------------------------------------------------------------------------//
void lcd_putc (unsigned char c)
{
	switch (c)
	{
		
		case '\n'  : 	lcd_gotoxy (2,1);
						break;
		case '\b'	:	LCD_Write_cmd (0x10);
						break;
		default  	:	LCD_Write_data (c);
						break;
		
	}
}
//---------------------------------------------------------------------------------//
void LCD_DisplayCharacter (unsigned char dat)
{
	LCD_Write_data (dat);
}
//---------------------------------------------------------------------------------//
void lcd_printf (unsigned char *string)
{
		while (*string)
			lcd_putc (*string++);
}
//---------------------------------------------------------------------------------//
void lcd_clear (char line)
{
   if (line == 1)
      lcd_gotoxy (1,1);
   else
      if (line == 2)
         lcd_gotoxy (2,1);

   lcd_printf("                    ");
}
//---------------------------------------------------------------------------------//
void init_lcd (void)
{
	LCD_DDR_CTRL |= _BV(5) | _BV(6) | _BV(7);// all port are output
	LCD_DDR	     = 0xff; 					//all port output
	
	LCD_PORT &= ~_BV(LCD_RW_PIN);
	
	delay_us(100);
	LCD_Write_cmd (0x33);				// Command for 4 bit interface
	LCD_Write_cmd (0x32);				// 
	LCD_Write_cmd (LCD_FUNCTION_SET);	// Function
	LCD_Write_cmd (LCD_ON);				// 5*7 , Blink
	LCD_Write_cmd (LCD_MODE_SET);		// mode
	LCD_Write_cmd (LCD_CLEAR);			// clear 
	delay_us(10);
}
//-----------------------------------------------------------------------------------------//
