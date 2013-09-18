/*

Structs with bitfields 

- exceeding datatype bits gives compiler warning

char 1byte
short 2byte
int 2byte
long 4byte
*/

struct {
    byte light :8;     // light sensor: 0..255
    byte moved :1;  // motion detector: 0..1
    byte humi  :7;  // humidity: 0..100
    int temp   :10; // temperature: -500..+500 (tenths)
    byte lobat :1;  // supply voltage dropped under 3.1V: 0..1
} payload;

void printByte(byte x) {
	Serial.print((x >> 7) & 1);	// MSB
	Serial.print((x >> 6) & 1);	
	Serial.print((x >> 5) & 1);	
	Serial.print((x >> 4) & 1);	
	Serial.print((x >> 3) & 1);	
	Serial.print((x >> 2) & 1);	
	Serial.print((x >> 1) & 1);	
	Serial.print((x >> 0) & 1);	// LSB
	Serial.println();	
}

volatile uint8_t buffer[4];

void setup(void) {
	Serial.begin(57600);
	Serial.println("ATmegaStructPacking");

	Serial.println(0xff);
	Serial.println((1<<7));

	Serial.println();
	Serial.println("MSB  LSB");
	Serial.println();

	// unpack
	//= *(Struct1*) 0xfff;

	payload.light = 0x0; // 8 bits
	payload.moved = 0x1; // 1 bit
	payload.humi = 63; // 7 bits
	payload.temp = -420; // 10 bits
	payload.lobat = 1; // 1 bit
  
   	// pack
    memcpy((void*) buffer, &payload, sizeof payload);

	printByte(buffer[0]);
	printByte(buffer[1]);
	printByte(buffer[2]);
	printByte(buffer[3]);
/*
         MSB  LSB
 Byte 0	 76543210   Light; bit 7-0
 Byte 1  65432100   Humi; bit 6-0, moved; 1 bit
 Byte 2  76543210   Temp; bit 7-0
 Byte 3  -----098   (5 bits unused), Lobat; 1bit, Temp MSB's 9-8

*/
	Serial.println();
}

void loop(void) {
}


