/*

Hookup:
  - VCC 3V3
  - D2 Si403 RST?
  - D8 SSD1306 RES (p5)
  - D9 SSD1306 DC (p6)
  - D10 SSD1306 CS (p7)
  - D11 SSD1306 D1 (p4)
  - D13 SSD1306 D0 (p3)
  - A4 SDA Si403
  - A5 SCL Si403

TODO: Move to Prototype/StereoFM_RDS_128x64
 */
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <radio.h>
#include <RDA5807M.h>
#include <SI4703.h>
#include <RDSParser.h>

SI4703 radio;
RDSParser rds;

int inByte;

U8G2_SSD1306_128X64_NONAME_1_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);

uint16_t g_block1;

// use a function inbetween the radio chip and the RDS parser
// to catch the block1 value (used for sender identification)
void RDS_process(uint16_t block1, uint16_t block2, uint16_t block3, uint16_t block4) {
  g_block1 = block1;
  rds.processData(block1, block2, block3, block4);
}

/// Update the ServiceName text on the LCD display.
void DisplayServiceName(char *name)
{
  bool found = false;

  for (uint8_t n = 0; n < 8; n++)
    if (name[n] != ' ') found = true;

  if (found) {
    Serial.print("RDS:");
    Serial.print(name);
    Serial.println('.');
  }
}

void setup(void)
{
  Serial.begin(57600);
  Serial.println("sketch-mar01a-spi-oled...");

  u8g2.begin();
  u8g2.firstPage();
  do {
    draw();
  } while( u8g2.nextPage() );

  //radio.debugEnable();
  radio.init();

  radio.setVolume(5);
  radio.setMono(false);
  radio.setMute(false);
  radio.setBandFrequency(RADIO_BAND_FM, 10160);
  //radio.setBandFrequency(RADIO_BAND_FM, 8930);

  radio.attachReceiveRDS(RDS_process);
  rds.attachServicenNameCallback(DisplayServiceName);
}

void draw(void)
{
  //u8g2.setFont(u8g2_font_logisoso20_tf);
  //u8g2.drawStr(35,40,"STBY");

  //u8g2.setFont(u8g2_font_logisoso32_tf);
  //u8g2.drawStr(25,55,"STBY");

  u8g2.setFont(u8g2_font_logisoso42_tf);
  u8g2.drawStr(10, 62, "STBY");
}

void loop(void)
{
  char s[12];
  radio.formatFrequency(s, sizeof(s));
  Serial.print("Station:");
  Serial.println(s);

  Serial.print("Radio:");
  radio.debugRadioInfo();

  Serial.print("Audio:");
  radio.debugAudioInfo();

  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_logisoso20_tf);
    u8g2.drawStr( 0, 62, s);

  } while( u8g2.nextPage() );

  radio.checkRDS();

  //RADIO_INFO ri;
  //radio.getRadioInfo(&ri);
  //Serial.print(ri.rssi); Serial.print(' ');
  //Serial.print(ri.snr); Serial.print(' ');
  //Serial.print(ri.stereo ? 'S' : 'm');
  //Serial.print(ri.rds ? 'R' : '-');

  delay(2500);

  if (Serial.available() > 0) {
    inByte = Serial.read();
    if (inByte == '+') {
      radio.seekUp(true);
    } else if (inByte == '-') {
      radio.seekDown(true);
    }
  }

  //if (digitalRead(4) == 1) {
  //  radio.seekUp(true);
  //}
}
