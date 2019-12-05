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

TODO: See sketch_mar01a_spi_oled
 */
#include <SPI.h>
#include <U8g2lib.h>


U8G2_SSD1306_128X64_NONAME_1_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);

void setup(void)
{
  u8g2.begin();
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
  u8g2.firstPage();
  do {
    draw();
  } while( u8g2.nextPage() );


  if (Serial.available()>0) {
    inByte = Serial.read();
    if (inByte == '+' || inByte == '-'){  //accept only + and - from keyboard
      flag=0;
    }
  }


  if (Radio.read_status(buf) == 1) {
     current_freq =  floor (Radio.frequency_available (buf) / 100000 + .5) / 10;
     stereo = Radio.stereo(buf);
     signal_level = Radio.signal_level(buf);
     //By using flag variable the message will be printed only one time.
     if(flag == 0){
      Serial.print("Current freq: ");
      Serial.print(current_freq);
      Serial.print("MHz Signal: ");
      //Strereo or mono ?
      if (stereo){
        Serial.print("STEREO ");
      }
    else{
      Serial.print("MONO ");
    }
      Serial.print(signal_level);
      Serial.println("/15");
      flag=1;
     }
  }

  //When button pressed, search for new station
  if (search_mode == 1) {
      if (Radio.process_search (buf, search_direction) == 1) {
          search_mode = 0;
      }
  }
  //If forward button is pressed, go up to next station
  if (inByte == '+') {
    last_pressed = current_millis;
    search_mode = 1;
    search_direction = TEA5767_SEARCH_DIR_UP;
    Radio.search_up(buf);
  }
  //If backward button is pressed, go down to next station
  if (inByte == '-') {
    last_pressed = current_millis;
    search_mode = 1;
    search_direction = TEA5767_SEARCH_DIR_DOWN;
    Radio.search_down(buf);
  }

  delay(500);
}
