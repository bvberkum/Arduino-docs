/*

 */
#include <PCD8544.h>
#include <util/delay.h>

#define BUFFER_SIZE  870  // 290 Sample transitions * 3 bytes for each sample = 870 bytes.
#define MAX_SAMPLE_TIME 65534
#define IN1 0 
#define IN2 1
#define IN3 2 
#define IN4 3

#define BTN_INCREASE   PB2
#define BTN_DECREASE   PB3
#define BTN_ZOOM_OUT   PB4
#define LED1           PB5
#define CTL_PORT       PORTB


static PCD8544 lcd;

char dataBuffer[BUFFER_SIZE];
volatile unsigned int  i, counter, bufferUsedSpace, zoom, minSampleTime;
volatile unsigned long  samplesPos;
char int2str[8];


void sendChannelsDataOnLCD (void)
{
//	printCapturedData(samplesPos, IN1);
//	printCapturedData(samplesPos, IN2);
//	printCapturedData(samplesPos, IN3);
//	printCapturedData(samplesPos, IN4);
}

void setup() {

	DDRD = 0x00;
	PORTD = 0x00;

	DDRC = 0x00;
	PORTC = 0xff;

    Serial.begin(57600);
    Serial.println("LogicAnalyzer");
	
    lcd.begin(84, 48);
    lcd.send( LOW, 0x21 );  // LCD Extended Commands.
    //lcd.send( LOW, 0xB9 );  // Set LCD Vop (Contrast). 
    //lcd.send( LOW, 0xC2);   // default Vop (3.06 + 66 * 0.06 = 7V)
    lcd.send( LOW, 0xE0);   // higher Vop for ST7576,  too faint at default
    lcd.send( LOW, 0x04 );  // Set Temp coefficent. //0x04
    lcd.send( LOW, 0x14 );  // LCD bias mode 1:48. //0x13
    lcd.send( LOW, 0x0C );  // LCD in normal mode.
    lcd.send( LOW, 0x20 );  // LCD Extended Commands toggle

	DDRB |= (1<<LED1);

	CTL_PORT &= ~(1<<LED1);
	CTL_PORT |= (1<<BTN_INCREASE);     //Enable pull-up resistor on BTN_DECREASE pin.
	CTL_PORT |= (1<<BTN_DECREASE);    //Enable pull-up resistor on BTN_INCREASE pin.
	CTL_PORT |= (1<<BTN_ZOOM_OUT); //Enable pull-up resistor on BTN_ZOOM_OUT pin.

	_delay_ms(100);

//	lcd.drawBitmap(introScreen);
//	lcd.drawBitmap(createdBy);
	
	samplesPos = 0;
	bufferUsedSpace = 0;
	zoom = 1;
	minSampleTime = MAX_SAMPLE_TIME;
	counter = 0;

	lcd.clear();
	lcd.setCursor(3,3);
	lcd.print("Waiting for");
	lcd.setCursor(4,4);
	lcd.print("signal...");
	
	//checkInputs(); // Stay here until a logic level change will be made on PORT D.
	               // Then, read all changes on PORT D until dataBuffer will be full.

	//lcd_clear();
	//lcd_goto_xy(14,1);
	//lcd_chr('1');
	//printRuler();
	//sendChannelsDataOnLCD();
}

void loop() {
}

