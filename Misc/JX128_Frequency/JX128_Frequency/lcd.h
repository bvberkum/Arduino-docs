#define LCD_DDR_CTRL		DDRD
#define LCD_CTRL 			PORTD

#define LCD_DDR				DDRA
#define	LCD_PORT			PORTA


#define LCD_E_PIN        	5            //pin  for Enable line     
#define	LCD_RW_PIN			6			 //pin  for R/W line
#define LCD_RS_PIN       	7            //pin  for RS line         


#define lcd_e_high()    	LCD_CTRL  |=  _BV(LCD_E_PIN); _delay_us (5);
#define lcd_e_low()     	LCD_CTRL  &= ~_BV(LCD_E_PIN); _delay_us (5);

#define lcd_e_toggle()  	toggle_e()

#define lcd_rs_high()   	LCD_CTRL |=  _BV(LCD_RS_PIN); _delay_us (5);
#define lcd_rs_low()    	LCD_CTRL &= ~_BV(LCD_RS_PIN); _delay_us (5);

#define LCD_FUNCTION_SET 	0x28
#define LCD_ON 				0x0c   // if you need to currsor use 0x0f
#define LCD_MODE_SET 		0x06
#define LCD_CLEAR 			0x01

void delay_us (unsigned long ndelay);
void LCD_Write_cmd (unsigned char cmd);
void LCD_Write_data (unsigned char dat);
void lcd_gotoxy (char row, char col);
void LCD_DisplayCharacter (unsigned char dat);
void lcd_putc (unsigned char c);
void lcd_printf (unsigned char *string);
void init_lcd (void);
void lcd_clear (char line);
