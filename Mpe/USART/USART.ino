
// Datasheet example code:
#define FOSC     16000000UL
#define BAUD     9600
#define MYUBBR FOSC/16/BAUD-1

// Tutorial:
//#define F_CPU 16000000UL
//#define BAUDRATE 9600
//#define BAUD 9600 //The baudrate that we want to use
//#define BAUD_PRESCALLER (((F_CPU / (BAUDRATE * 16UL))) - 1)//The formula that does all the required maths

#include <avr/io.h>
#include <util/delay.h>

//Declaration of our functions
void USART_init(void);
unsigned char USART_receive(void);
void USART_send( unsigned char data);
void USART_putstring(char* StringPtr);

char String[]="Hello world!!";



//#include <avr/io.h>
//#include <util/delay.h>


// mixed:
/** ubrr is the baud-prescaler
 */
void USART_init(unsigned int ubrr)
{
  UBRR0H = (uint8_t)(ubrr>>8);
  UBRR0L = (uint8_t)ubrr;
  UCSR0B = (1<<RXEN0)|(1<<TXEN0);
  //328p: UCSR0C = ((1<<UCSZ00)|(1<<UCSZ01));
  UCSR0C = (1<<USBS0)|(3<<UCSZ00);
}

unsigned char USART_receive(void){

  while(!(UCSR0A & (1<<RXC0)));
  return UDR0;

}

void USART_send( unsigned char data){

  while(!(UCSR0A & (1<<UDRE0)));
  UDR0 = data;

}

void USART_putstring(char* StringPtr){

  while(*StringPtr != 0x00){
    USART_send(*StringPtr);
    StringPtr++;
  }

}

// Main
//int main(void) {
//  return 0;
//}

void setup() {
  USART_init(MYUBBR);

  //Serial.begin(57600);
  //Serial1.begin(9600);
  //Serial.println("Testing 1284");
  for( int i=0 ; i<14 ; i++ )
  {
    pinMode(i, OUTPUT);
  }
>>>>>>> eb55d1eb430551ddb80ca886f775e9892293bfee
}

void loop() {
  

	USART_putstring(String);    //Pass the string to the USART_putstring function and sends it over the serial
  
  for( int i=0 ; i<14 ; i++ )
  {
    digitalWrite(i, 1);
  }

  //int sensorValue = analogRead(A0);
  //Serial.println(sensorValue);
  delay(50);

  for( int i=0 ; i<14 ; i++ )
  {
    digitalWrite(i, 0);
  }

  //sensorValue = analogRead(A0);
  //Serial.println(sensorValue);

  //Serial.print('.');
//  delay(2100);
	_delay_ms(2000);        //Delay for 5 seconds so it will re-send the string every 5 seconds



}


