/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.




Description:
Reads an i2c eeprom R24RF08 (or similar) in a thinkpad laptop to recover the supervisor password. 
Connect wires to the eeprom according to various websites.


!!!!!!!!!!!!   The 12c bus voltage must be 3.3V     !!!!
Reduce the voltage of the arduino or use a level shifter!!

Any damage to your device is you own responsibility 


References:
http://www.allservice.ro
http://www.ja.axxs.net/



*/




//i2c pin definitions
const char scl = 11;
const char sda = 12;
const byte delay_halfperiod = 100;



//returns false on nack
boolean send_i2c_data(byte *data, byte byte_count){
	char ack=0,i,ii;
	
	
	//startbit
	digitalWrite(sda,LOW);
	delayMicroseconds(delay_halfperiod);
	
	
	//send data
	for (ii=0; ii < byte_count; ii++){
		for (i=7; i >= 0; i--){
			digitalWrite(scl,LOW);
			delayMicroseconds(delay_halfperiod);
			digitalWrite(sda,(data[ii] >> i) & 0x01);
			delayMicroseconds(delay_halfperiod);
			digitalWrite(scl,HIGH);
			delayMicroseconds(2 * delay_halfperiod);
			
		}
		
		//get ACK bit
		digitalWrite(scl,LOW);
		delayMicroseconds(delay_halfperiod);
		digitalWrite(sda,HIGH);
		pinMode(sda, INPUT);
		delayMicroseconds(delay_halfperiod);
		digitalWrite(scl,HIGH);
		delayMicroseconds(delay_halfperiod);
		ack = digitalRead(sda);
		delayMicroseconds(delay_halfperiod);
		digitalWrite(scl,LOW);
		delayMicroseconds(delay_halfperiod);
		pinMode(sda,OUTPUT);
		
//Serial.print("ack:");
//Serial.println((ack == HIGH)?'1':'0');
		
		if (ack == HIGH){
			Serial.print("xNACK!! ");
			Serial.println((int)ii);
			break;
			}
	}

	//stop bit
	digitalWrite(sda,LOW);
	delayMicroseconds(delay_halfperiod);
	digitalWrite(scl,HIGH);
	delayMicroseconds(delay_halfperiod);
	digitalWrite(sda,HIGH);
	delayMicroseconds(2 * delay_halfperiod);


	//give chip time to process write
	delay(10);

	return (ack == LOW);
}




byte receive_i2c_data(byte addr){
	char ack=0,i,ii;
	byte data = 0;

	//startbit
	digitalWrite(sda,LOW);
	delayMicroseconds(delay_halfperiod);


	//send address
	for (i=7; i >= 0; i--){
		digitalWrite(scl,LOW);
		delayMicroseconds(delay_halfperiod);
		digitalWrite(sda,(addr >> i) & 0x01);
		delayMicroseconds(delay_halfperiod);
		digitalWrite(scl,HIGH);
		delayMicroseconds(2 * delay_halfperiod);
	}

		
	//get ACK bit
	digitalWrite(scl,LOW);
	delayMicroseconds(delay_halfperiod);
	digitalWrite(sda,HIGH);
	pinMode(sda, INPUT);
	delayMicroseconds(delay_halfperiod);
	digitalWrite(scl,HIGH);
	delayMicroseconds(delay_halfperiod);
	ack = digitalRead(sda);
	delayMicroseconds(delay_halfperiod);
	digitalWrite(scl,LOW);
	delayMicroseconds(delay_halfperiod);
	pinMode(sda,OUTPUT);
	
	if (ack == HIGH){
		Serial.print("yNACK!! ");
		Serial.println((int)ii);
		return 0;
		}


//Serial.print("ack:");
//Serial.println((ack == HIGH)?'1':'0');

	//read byte
	pinMode(sda, INPUT);
	for (i=7; i >= 0; i--){
		digitalWrite(scl,LOW);
		delayMicroseconds(delay_halfperiod);
		delayMicroseconds(delay_halfperiod);
		digitalWrite(scl,HIGH);
		delayMicroseconds(delay_halfperiod);
		data = (data << 1) | ((digitalRead(sda)==HIGH)?1:0);
		delayMicroseconds(delay_halfperiod);
	}	
	pinMode(sda,OUTPUT);

	//send no ack
	digitalWrite(scl,LOW);
	delayMicroseconds(delay_halfperiod);
	digitalWrite(sda,HIGH);
	delayMicroseconds(delay_halfperiod);
	digitalWrite(scl,HIGH);
	delayMicroseconds(delay_halfperiod);
	delayMicroseconds(delay_halfperiod);
	digitalWrite(scl,LOW);
	digitalWrite(sda,HIGH);
	delayMicroseconds(delay_halfperiod);

//Serial.print("ack:");
//Serial.println((ack == HIGH)?'1':'0');

	//stop bit
	digitalWrite(sda,LOW);
	delayMicroseconds(delay_halfperiod);
	digitalWrite(scl,HIGH);
	delayMicroseconds(delay_halfperiod);
	digitalWrite(sda,HIGH);
	delayMicroseconds(2 * delay_halfperiod);

	return data;
}



#define confTableSize 64

//scancode conversion table
char convTable[256] = {
		' ', ' ', '1', '2', '3', '4', '5', '6',    '7', '8', '9', '0', '-', '=', ' ', ' ', 
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',    'O', 'P', ' ', '$', ' ', ' ', 'A', 'S', 
		'D', 'F', 'G', 'H', 'J', 'K', 'L', ';',    '\'','`', '\\','\\','Z', 'X', 'C', 'V', 
		'B', 'N', 'M', ',', '.', '/', '"', '*',    ' ', ' ', ' ', ' ', 'f', 'f', 'f', 'f',
		
		'f', 'f', 'f', 'f', 'f', ' ', ' ', '7',    '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.', ' ', ' ', '>', ' ',    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
		' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
		' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
		
		' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
		' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
		' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
		' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
		
		' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
		' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
		' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
		' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
		};
		

char convTableAlternate[256] = {
		' ', ' ', ' ', ' ', '1', '1', '2', '3',    '3', '3', '4', '4', '5', '5', '6', '6', 
		'7', '7', '8', '8', '9', '9', '0', '0',    '-', '-', '=', '=', ' ', ' ', ' ', ' ',
		'Q', 'Q', 'W', 'W', 'E', 'E', 'R', 'R',    'T', 'T', 'Y', 'Y', 'U', 'U', 'I', 'I',
		'O', 'O', 'P', 'P', ' ', ' ', '$', '$',    ' ', ' ', ' ', ' ', 'A', 'A', 'S', 'S',
		
		'D', 'D', 'F', 'F', 'G', 'G', 'H', 'H',    'J', 'J', 'K', 'K', 'L', 'L', ';', ';',
		'\'','\'','`', '`', '\\','\\','\\','\\',   'Z', 'Z', 'X', 'X', 'C', 'C', 'V', 'V',
		'B', 'B', 'N', 'N', 'M', 'M', ',', ',',    '.', '.', '/', '/', '"', '"', '*', '*',
		' ', ' ', ' ', ' ', ' ', ' ', 'f', 'f',    'f', 'f', 'f', 'f', 'f', 'f', 'f', 'f',
		
		'f', 'f', 'f', 'f', 'f', 'f', 'f', 'f',    'f', 'f', ' ', ' ', ' ', ' ', '7', '7',
		'8', '8', '9', '9', '-', '-', '4', '4',    '5', '5', '6', '6', '+', '+', '1', '1',
		'2', '2', '3', '3', '0', '0', '.', '.',    ' ', ' ', ' ', ' ', '>', '>', ' ', ' ',
		' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
		
		' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
		' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
		' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
		' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
		};



void dumpEeprom(boolean debug = false){
	byte page = 0;
	byte data[2];
	
	data[0] = 0xa8 | 0x00;
	data[1] = 0;
	
	Serial.println("Dump the whole eeprom: ");

	while (true){
		boolean res = send_i2c_data(data,2);
		byte dat = receive_i2c_data(data[0] | 0x01);
	
		if (! res){
			Serial.println("   NACK   ");
			Serial.println("!!! Error !!! Reading Eeprom failed!");
			return;
		}
		
		if (debug){
			Serial.print("Page ");
			Serial.print(page + 1);
			Serial.print("; ");
			Serial.print(data[1],DEC);
			Serial.print(": ");
			Serial.print((unsigned int)data[1] + 255 * (unsigned int)page, HEX);
			Serial.print(" => 0x");
		}
		
		
		if( dat < 0x10){ Serial.print("0");}
		Serial.print(dat,HEX);
		Serial.print(" ");
		
		if (debug){
			Serial.print(" => ");
			Serial.print(convTable[dat]);
			Serial.print(" => ");
			Serial.println(convTableAlternate[dat]);
		}
	
		data[1]++;
		if (data[1] == 0){
			if (page == 0){
				data[0] = 0xa8 | 0x02; 
				page = 1;
			}
			else if (page == 1){
				data[0] = 0xa8 | 0x04;
				page = 2 ;
			}
			else if (page == 2){
				data[0] = 0xa8 | 0x06;
				page = 3 ;
			}
			else if (page == 3){
				Serial.println("");
				return;
			}
			data[1] = 0;
		}
	}
}




void readPassword(char* table){
	byte data[2];
	
	
	data[0] = 0xa8 | 0x06;
	data[1] = 56;
	
	Serial.print("The Supervisor Password is: ");

	while (true){
		boolean res = send_i2c_data(data,2);
		byte dat = receive_i2c_data(data[0] | 0x01);
		
		if (! res){
			Serial.println("   NACK   ");
			Serial.println("!!! Error !!! Could not read the Password!");
			return;
		}
		
		
		Serial.print(convTable[dat]);
		data[1]++;
		
		if (data[1] > 62){
			Serial.println("");
			return;
		}
	}

}


/*
void readAccessProt(){
	byte data[2];
	
	Serial.print("Access Page: ");
	
	data[0] = 0xa8;
	data[1] = 0;
	boolean res = send_i2c_data(data,2);
	
	
	
	for (byte i = 0; i < 10; i++){
		data[0] = 0xb8;
		data[1] = i;
		//boolean res = send_i2c_data(data,2);
		byte dat = receive_i2c_data(data[0] | 0x01);
		if( dat < 0x10){ Serial.print("0");}
		Serial.print(dat,HEX);
		Serial.print(" ");
	
	}

	Serial.println("");


}*/






byte orgData[16];
byte kornData[10] = {	0x00, 0x00,
						//0x25, 0x18, 0x13, 0x31, 0x00, 0x00, 0x00, 0x81};//password korn
						//0x02, 0x03, 0x04, 0x04, 0x00, 0x00, 0x00, 0x0e};//password 1234
						0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};// clear password
						
void setPwKorn(){
	byte data[2];
	int i = 0;
	
	data[0] = 0xa8 | 0x06;
	data[1] = 56;
	
	Serial.println("Original data:");

	while (true){
		boolean res = send_i2c_data(data,2);
		byte dat = receive_i2c_data(data[0] | 0x01);
		
		
		
		if (! res){
			Serial.println("   NACK   ");
			Serial.println("!!! Error !!! Could not read the Password!");
			return;
		}
		
		
		orgData[i++] = dat;
		if( dat < 0x10){ Serial.print("0");}
		Serial.print(dat,HEX);
		Serial.print(" ");
		
		
		data[1]++;
		
		if (data[1] > 71){
			Serial.println("");
			break;
		}
	}

	kornData[0] = 0xa8 | 0x06;
	kornData[1] = 56;
	boolean res = send_i2c_data(kornData,10);
	
	if (! res){
			Serial.println("   NACK   ");
			Serial.println("!!! Error !!! Could not set the Password!");
			return;
		}
		
		
	
	kornData[0] = 0xa8 | 0x06;
	kornData[1] = 64;
	res = send_i2c_data(kornData,10);
	
	if (! res){
			Serial.println("   NACK2   ");
			Serial.println("!!! Error !!! Could not set the Password!");
			return;
		}


}







void setup() {
	
	pinMode(scl, OUTPUT);
	pinMode(sda, OUTPUT);
	digitalWrite(scl,HIGH);
	digitalWrite(sda,HIGH);
	delay(20);
	
	Serial.begin(57600);
	
	while (Serial.available() != 0)
		char c = Serial.read();
}


void loop() {
	Serial.println("");
	Serial.println("");
	Serial.println("");
	Serial.println("Deif's Thinkpad Eeprom Reader");
	Serial.println("Usage:");
	Serial.println("p: Read the Password in Plaintext");
	Serial.println("w: Read the Password in Plaintext (alternate Scancode)");
	Serial.println("d: Dump the Eeprom in HEX format (for import in IBMpass)");
	Serial.println("g: Dump the Eeprom in Text format (for debug)");
	Serial.println("h: Clear the password!");
	
	
	while (Serial.available() == 0);
	
	char c = Serial.read();
Serial.println(c);
	if (c == 'p')
		readPassword(convTable);
	else if (c == 'w')
		readPassword(convTableAlternate);
	else if (c == 'd')
		dumpEeprom(false);
	else if (c == 'g')
		dumpEeprom(true);
	else if (c == 'h')
		setPwKorn();
	else
		Serial.println("Unknown command!");

}

















