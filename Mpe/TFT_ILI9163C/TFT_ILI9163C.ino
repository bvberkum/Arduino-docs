/*
	
*/
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <TFT_ILI9163C.h>

// Color definitions
#define	BLACK   0x0000
#define	BLUE    0x001F
#define	RED     0xF800
#define	GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0  
#define WHITE   0xFFFF

// 1,44' SPI 128*128 V1.1 pins:
// vcc
// gnd
#define CS   8
#define RESET 6 
#define DC   9 // (AO)
//#define MOSI 11 (SDA)
//#define SCLK 13 (SCK)
// led

#define GPS_HW_SERIAL 0
#define rxPin 7
#define txPin 6


TFT_ILI9163C tft = TFT_ILI9163C(CS, DC, RESET);

//#define SERIAL 1
//#if GPS_HW_SERIAL
//#define SERIAL 0
//#endif

//#if !GPS_HW_SERIAL
SoftwareSerial gps_serial(7, 6); // RX, TX
//#elif GPS_HW_SERIAL
//Stream &gps_serial = Serial;
//#endif

TinyGPSPlus gps;

int r=0;
int byteRead = 0;
int overflow_flash = 0;

/*
unsigned long testText() {
	tft.fillScreen();
	tft.setRotation(r);
	r = r+1;
	if (r==4) {
		r=0;
	}
	unsigned long start = micros();
	tft.setCursor(0, 0);
	tft.setTextColor(WHITE);  
	tft.setTextSize(1);
	tft.println("Hello World!");
	tft.setTextColor(YELLOW); 
	tft.setTextSize(2);
	tft.println(1234.56);
	tft.setTextColor(RED);    
	tft.setTextSize(3);
	tft.println(0xDEAD, HEX);
	tft.println();
	tft.setTextColor(GREEN);
	tft.setTextSize(4);
	tft.println("Hello");
	return micros() - start;
}

unsigned long testLines(uint16_t color) {
	tft.fillScreen();
	unsigned long start, t;
	int           x1, y1, x2, y2,
								w = tft.width(),
								h = tft.height();
	tft.fillScreen();
	x1 = y1 = 0;
	y2    = h - 1;
	start = micros();
	for(x2=0; x2<w; x2+=6) tft.drawLine(x1, y1, x2, y2, color);
	x2    = w - 1;
	for(y2=0; y2<h; y2+=6) tft.drawLine(x1, y1, x2, y2, color);
	t     = micros() - start; // fillScreen doesn't count against timing
	tft.fillScreen();
	x1    = w - 1;
	y1    = 0;
	y2    = h - 1;
	start = micros();
	for(x2=0; x2<w; x2+=6) tft.drawLine(x1, y1, x2, y2, color);
	x2    = 0;
	for(y2=0; y2<h; y2+=6) tft.drawLine(x1, y1, x2, y2, color);
	t    += micros() - start;
	tft.fillScreen();
	x1    = 0;
	y1    = h - 1;
	y2    = 0;
	start = micros();
	for(x2=0; x2<w; x2+=6) tft.drawLine(x1, y1, x2, y2, color);
	x2    = w - 1;
	for(y2=0; y2<h; y2+=6) tft.drawLine(x1, y1, x2, y2, color);
	t    += micros() - start;
	tft.fillScreen();
	x1    = w - 1;
	y1    = h - 1;
	y2    = 0;
	start = micros();
	for(x2=0; x2<w; x2+=6) tft.drawLine(x1, y1, x2, y2, color);
	x2    = 0;
	for(y2=0; y2<h; y2+=6) tft.drawLine(x1, y1, x2, y2, color);
	return micros() - start;
}
*/
bool b;
int line = 0;
int c = 0;
int newData = 0;
char in;

void setup() {

//#if SERIAL && !GPS_HW_SERIAL
	Serial.begin(57600);
//#endif

//#if !GPS_HW_SERIAL
	pinMode(rxPin, INPUT);
	pinMode(txPin, OUTPUT);
//#endif

	gps_serial.begin(9600);
	//gps_serial.begin(38400);

	tft.begin();
	//tft.setBitrate(24000000);
	//tft.setBitrate(8000000);
	b = false;
	//testText();
	//tft.setCursor(0, 0);
	tft.setTextColor(WHITE);  
	tft.setTextSize(1);
	tft.setRotation(1);

#if SERIAL && !GPS_HW_SERIAL
	Serial.println("TFT_ILI9163C");
	Serial.println(TinyGPSPlus::libraryVersion());
#endif
}

String serialBuffer = "";         // a string to hold incoming data

void gpsDisplay() {
	tft.fillScreen();
	tft.setCursor(0,0);

	tft.print(gps.date.year()); // Year (2000+) (u16)
	tft.print("-");
	tft.print(gps.date.month()); // Month (1-12) (u8)
	tft.print("-");
	tft.println(gps.date.day()); // Day (1-31) (u8)

	tft.print(gps.time.hour()); // Hour (0-23) (u8)
	tft.print(":");
	tft.print(gps.time.minute()); // Minute (0-59) (u8)
	tft.print(":");
	tft.print(gps.time.second()); // Second (0-59) (u8)
	tft.print(".");
	tft.println(gps.time.centisecond()); // 100ths of a second (0-99) (u8)

	tft.println("");

	tft.print("LAT: ");
	tft.print(gps.location.rawLat().negative ? "-" : "+");
	tft.print(gps.location.rawLat().deg);
	tft.print(".");
	tft.println(gps.location.rawLat().billionths);

	tft.print("LNG: ");
	tft.print(gps.location.rawLng().negative ? "-" : "+");
	tft.print(gps.location.rawLng().deg);
	tft.print(".");
	tft.println(gps.location.rawLng().billionths);

	tft.println("");

	tft.print("ALT: ");
	tft.println(gps.altitude.meters());

	tft.println("");

	tft.print("SAT: ");  
	tft.println(gps.satellites.value());

	tft.println("");

	if (overflow_flash)
		tft.println("!");
}
void loop(){
	if (gps_serial.overflow()) {
		Serial.println("!");
		overflow_flash = 1;
	}
	while (gps_serial.available() > 0) {
		in = gps_serial.read();
		gps.encode(in);
		Serial.print(in);
/*
		byteRead += 1;
		char inchar = (char)gps_serial.read();
		serialBuffer += inchar;
*/
	}
	//if (byteRead >= 64) {
	//	Serial.print("Software Serial Overflow: ");
	//	Serial.println(byteRead);
	//}
	if (byteRead > 0) {
		byteRead = 0;
		newData = 1;
		return;
	}
	if (newData > 0) {
		//Serial.print(serialBuffer.length());
		//Serial.print(' ');
		newData = 0;
		for (int i=0;i<serialBuffer.length(); i++) {
			gps.encode(serialBuffer.charAt(i));
		}
		//Serial.println(serialBuffer);
		serialBuffer = "";
	}
//#if SERIAL && !GPS_HW_SERIAL
	if (gps.altitude.isUpdated())
	{
		Serial.print("ALT=");  Serial.println(gps.altitude.meters());
	}
	if (gps.location.isUpdated())
	{
		Serial.print("LAT=");  Serial.println(gps.location.lat());
		Serial.print("LNG=");  Serial.println(gps.location.lng());
	}
	if (gps.satellites.isUpdated()) {
		Serial.print("SAT=");  Serial.println(gps.satellites.value());
	}
	if (gps.hdop.isUpdated()) {
		Serial.print("HDOP=");  Serial.println(gps.hdop.value());
	}
//#endif

	if ( c % 100000 == 0 ) {
		gpsDisplay();
		byte chk = gps.failedChecksum();
//#if SERIAL && !GPS_HW_SERIAL
		Serial.print("Sentences that failed checksum=");
		Serial.println(chk);
//#endif
	}
	c += 1;
}




