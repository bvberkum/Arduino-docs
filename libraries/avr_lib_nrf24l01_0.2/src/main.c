/*
nrf24l01 lib sample

copyright (c) Davide Gironi, 2012

Released under GPLv3.
Please refer to LICENSE file for licensing information.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

//define debug print enable or disable on uart
#define DEBUGENABLED 1

//include uart
#if DEBUGENABLED == 1
#include "uart/uart.h"
#endif

//include nrf24l01
#include "nrf24l01/nrf24l01.h"

//role definitions
#define ROLETX 1
#define ROLERX 0

//role button/pin
#define BUTTONROLE_DDR DDRC
#define BUTTONROLE_PORT PORTC
#define BUTTONROLE_PIN PINC
#define BUTTONROLE PC0
//send button
#define BUTTONSEND_DDR DDRC
#define BUTTONSEND_PORT PORTC
#define BUTTONSEND_PIN PINC
#define BUTTONSEND PC1
//output led
#define LEDOUT_DDR DDRC
#define LEDOUT_PORT PORTC
#define LEDOUT PC5
#define LEDOUTPAUSE 300

//main here
int main(void) {
	uint8_t txrxrole = 0; // 1 transmitter 0 receiver
	uint8_t i = 0;

	//nrf24l01 variables
	uint8_t bufferout[NRF24L01_PAYLOAD];
	uint8_t bufferin[NRF24L01_PAYLOAD];

	//setup port
	BUTTONROLE_DDR &= ~(1<<BUTTONROLE); //input
	BUTTONROLE_PORT &= ~(1<<LEDOUT); //off
	BUTTONSEND_DDR &= ~(1<<BUTTONSEND); //input
	BUTTONSEND_PORT &= ~(1<<LEDOUT); //off
	LEDOUT_DDR |= (1<<LEDOUT); //output
	LEDOUT_PORT &= ~(1<<LEDOUT); //off

	#if DEBUGENABLED == 1
	//init uart
	uart0_init( UART_BAUD_SELECT(UART_BAUD_RATE,F_CPU) );
	#endif

	//init nrf24l01
	nrf24l01_init();

	//init interrupt
	sei();

	//select a role
	if ((BUTTONROLE_PIN & (1<<BUTTONROLE)) == 0)
		txrxrole = ROLERX;
	else
		txrxrole = ROLETX;

	//blink led depending on role
	if(txrxrole == ROLETX) {
		LEDOUT_PORT |= (1<<LEDOUT); _delay_ms(LEDOUTPAUSE);
		LEDOUT_PORT &= ~(1<<LEDOUT); _delay_ms(LEDOUTPAUSE);
	} else if(txrxrole == ROLERX) {
		LEDOUT_PORT |= (1 << LEDOUT); _delay_ms(LEDOUTPAUSE);
		LEDOUT_PORT &= ~(1 << LEDOUT); _delay_ms(LEDOUTPAUSE);
		LEDOUT_PORT |= (1<<LEDOUT); _delay_ms(LEDOUTPAUSE);
		LEDOUT_PORT &= ~(1<<LEDOUT); _delay_ms(LEDOUTPAUSE);
	}

	#if DEBUGENABLED == 1
	if(txrxrole == ROLETX)
		uart0_puts("starting as tx...\r\n");
	else if(txrxrole == ROLERX)
		uart0_puts("starting as rx...\r\n");
	#endif

	//setup buffer
	for(i=0; i<sizeof(bufferout); i++)
		bufferout[i] = i+'a';
	for(i=0; i<sizeof(bufferin); i++)
		bufferin[i] = 0;

	//sending buffer addresses
	uint8_t sendpipe = 0;
	uint8_t addrtx0[NRF24L01_ADDRSIZE] = NRF24L01_ADDRP0;
	uint8_t addrtx1[NRF24L01_ADDRSIZE] = NRF24L01_ADDRP1;
	uint8_t addrtx2[NRF24L01_ADDRSIZE] = NRF24L01_ADDRP2;
	uint8_t addrtx3[NRF24L01_ADDRSIZE] = NRF24L01_ADDRP3;
	uint8_t addrtx4[NRF24L01_ADDRSIZE] = NRF24L01_ADDRP4;
	uint8_t addrtx5[NRF24L01_ADDRSIZE] = NRF24L01_ADDRP5;

	#if DEBUGENABLED == 1 && NRF24L01_PRINTENABLE == 1
		nrf24l01_printinfo(uart0_puts, uart0_putc);
	#endif

	//main loop
	for(;;) {
		if(txrxrole == ROLETX) {

			//tx
			#if DEBUGENABLED == 0
			if ((BUTTONSEND_PIN & (1<<BUTTONSEND)) != 0) { //if button pressed
			#endif

				#if DEBUGENABLED == 1
				char pipebuffer[5];
				uart0_puts("sending data, on pipe ");
				itoa(sendpipe, pipebuffer, 10);
				uart0_puts(pipebuffer);
				uart0_puts("... ");
				#endif

				if(sendpipe == 0) {
					//set tx address for pipe 0
					nrf24l01_settxaddr(addrtx0);
				} else if(sendpipe == 1) {
					//set tx address for pipe 1
					nrf24l01_settxaddr(addrtx1);
				} else if(sendpipe == 2) {
					//set tx address for pipe 2
					nrf24l01_settxaddr(addrtx2);
				} else if(sendpipe == 3) {
					//set tx address for pipe 3
					nrf24l01_settxaddr(addrtx3);
				} else if(sendpipe == 4) {
					//set tx address for pipe 4
					nrf24l01_settxaddr(addrtx4);
				} else if(sendpipe == 5) {
					//set tx address for pipe 5
					nrf24l01_settxaddr(addrtx5);
				}

				//write buffer
				uint8_t writeret = nrf24l01_write(bufferout);

				#if DEBUGENABLED == 1
				if(writeret == 1)
					uart0_puts("ok\r\n");
				else
					uart0_puts("failed\r\n");
				#endif

				sendpipe++;
				sendpipe%=6;

				//blink led for output data
				LEDOUT_PORT |= (1<<LEDOUT); _delay_ms(LEDOUTPAUSE);
				LEDOUT_PORT &= ~(1<<LEDOUT); _delay_ms(LEDOUTPAUSE);
			#if DEBUGENABLED == 0
			}
			#endif
			#if DEBUGENABLED == 1
			_delay_ms(1000);
			#endif
		} else if(txrxrole == ROLERX) {

			//rx
			uint8_t pipe = 0;
			if(nrf24l01_readready(&pipe)) { //if data is ready

				#if DEBUGENABLED == 1
				char pipebuffer[5];
				uart0_puts("getting data, on pipe ");
				itoa(pipe, pipebuffer, 10);
				uart0_puts(pipebuffer);
				uart0_puts("\r\n");
				#endif

				//read buffer
				nrf24l01_read(bufferin);

				#if DEBUGENABLED == 1
				uint8_t samecheck = 1;
				uart0_puts("  data: ");
				for(i=0; i<sizeof(bufferin); i++) {
					if(bufferin[i] != bufferout[i])
						samecheck = 0;
					uart0_putc(bufferin[i]);
				}
				uart0_puts("\r\n");
				if(samecheck)
					uart0_puts("  check ok\r\n");
				else
					uart0_puts("  check fails\r\n");
				for(i=0; i<sizeof(bufferin); i++)
					bufferin[i] = 0;
				#endif

				//blink led for input data
				LEDOUT_PORT |= (1<<LEDOUT); _delay_ms(LEDOUTPAUSE);
				LEDOUT_PORT &= ~(1<<LEDOUT); _delay_ms(LEDOUTPAUSE);
			}
			_delay_ms(10);

		}
	}
}




