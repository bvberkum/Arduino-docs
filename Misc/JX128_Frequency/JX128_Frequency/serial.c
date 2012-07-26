#include <avr/io.h>
#include <avr/interrupt.h>

#include <stdarg.h>
#include <string.h>
#include <avr/pgmspace.h>

#include "mydefs.h"
#include <util/delay.h>
#include "serial.h"

#define RBUFLEN 16 

volatile unsigned char rbuf[RBUFLEN]; //Ringpuffer 
volatile unsigned char rcnt, rpos, recbuf;
volatile unsigned char busy;

SIGNAL(SIG_UART0_RECEIVE)         /* signal handler for receive complete interrupt */
{
 recbuf= UART0_RECEIVE_REGISTER;  //Byte auf jeden Fall abholen, sonst Endlosinterrupt

								  /* don't overwrite chars already in buffer */
 if(rcnt < RBUFLEN)
	rbuf[(rpos+rcnt++) % RBUFLEN] = recbuf;

}

SIGNAL(SIG_UART0_TRANSMIT)       /* signal handler for transmit complete interrupt */
{
   DISABLE_UART0_TRANSMIT_INT; //ATmega Disable Transmit complete interrupt löschen

 busy=0; //Byte gesendet Flag rücksetzen
}
//-------------------------------------------------------------------------//
unsigned char ser_getc (void)
{
 unsigned char c;

 while(!rcnt) { };   /* wait for character */

 DISABLE_UART0_TRANSMIT_INT; //ATMega Disable Receiveinterrupt

	rcnt--;
	c = rbuf [rpos++];
	if (rpos >= RBUFLEN)  
		rpos = 0;

 ENABLE_UART0_RECEIVE_INT; //ATmega Enable Receiveinterrupt

 return (c);
}

//-------------------------------------------------------------------------//
void ser_putc(unsigned char c)
{
  while(busy==1);	
   
  ENABLE_UART0_TRANSMIT_INT; //ATmega Enable Transmit complete interrupt 
  UART0_TRANSMIT_REGISTER=c;

  busy = 1;        
}
//-------------------------------------------------------------------------//
// Transmit string from RAM
void ser_puts(unsigned char * s)
{
 unsigned char c;

   while((c=*s++))
    {
     if(c == '\n') //CR und LF \n
      {
       ser_putc(0x0D); //CR
       ser_putc(0x0A); //LF
      }
     else ser_putc(c);
    }
}

//-------------------------------------------------------------------------//
// Transmit string from FLASH
void _serputs_P(char const *s)
{
 unsigned char c;

 while((c=pgm_read_byte(s++)))
    {
     if(c == '\n') //CR und LF  \n
      {
       ser_putc(0x0D); //CR
       ser_putc(0x0A); //LF
      }
     else ser_putc(c);
    }
}
//-------------------------------------------------------------------------//
void ser_init(void)
{
  
  rcnt = rpos = 0;  /// init buffers 
  busy = 0;

  sbi(TX0_DDR,TX0_BIT);  //TxD output
  sbi(TX0_PORT,TX0_BIT); //set TxD to 1

  // enable RxD/TxD and ints  
	UART0_CONFIGURE1; // See serial.h
	UART0_CONFIGURE2; // See serial.h

	UART0_BAUD_REGISTER_HIGH=(unsigned char)(UART_BAUD_SELECT>>8); 	// set baudrate
	UART0_BAUD_REGISTER_LOW=(unsigned char)(UART_BAUD_SELECT); 		// set baudrate
			
}
//-------------------------------------------------------------------------//
void ser_puthex(unsigned char by)
{
	unsigned char buff;

		buff=by>>4; 
		if(buff<10) 
			buff+='0'; 
		else 
			buff+=0x37;        
		ser_putc(buff);

		buff=by&0x0f; 
		if(buff<10) 
			buff+='0'; 
		else 
			buff+=0x37;        
	ser_putc(buff);
}

//--------------------------------------------------------------------------------//
void init_uart1 (void)
{
// For uart 1
	UCSR1A=0x00;
	UCSR1B=0x18;
	UCSR1C=0x06;
	UBRR1H=0x00;
	UBRR1L=0xCF;   
				// XTAL = 8 MHZ = 0x33
				//	 4 MHz = 0x19
				//	 16MHz = 0x67 = 9600
				//		   = 0xCF = 4800
}
//--------------------------------------------------------------------------------//
char uart1_ReceiveByte (void)
{
	// Wait until a byte has been received
	while ((UCSR1A&(1<<RXC1)) == 0) ;
	// Return received data
		return UDR1;
}
//--------------------------------------------------------------------------------//
// send a char on the usart
void uart1_SendByte (char data)
{
	// wait for txd buffer to be empty
	while (!( UCSR1A & (1<<UDRE1) ));
	// write data  
	UDR1 = data;
}
//--------------------------------------------------------------------------------//
void uart1_SendString (char *str)
{			
 	// parse and send string until end is found
	//unsigned int len,i;
	UBRR1L=0x67; 
	while (*str != ('\0'))
	{
 		uart1_SendByte (*str++);
		_delay_ms (2);
 	}
	UBRR1L=0xCF;  
};

