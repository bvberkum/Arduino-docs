/*

TODO: want a simple freq meter with lcd 

 */
#include <Wire.h>
#include <LiquidCrystal.h>


/**

LCD: 8 pins

6 pins for display, one for backlight and one for contrast

*/

#define   CONTRAST_PIN   9
#define   BACKLIGHT_PIN  7
#define   CONTRAST       110
LiquidCrystal lcd(12, 11, 5, 4, 3, 2, BACKLIGHT_PIN );


void setup()
{
  Serial.begin ( 57600 );
  
  // Switch on the backlight and LCD contrast levels
  pinMode(CONTRAST_PIN, OUTPUT);
  analogWrite ( CONTRAST_PIN, CONTRAST );

  //lcd.setBacklightPin ( BACKLIGHT_PIN, POSITIVE );
  //lcd.setBacklight ( HIGH );
  //lcd.backlight();
  
  lcd.begin(16,2);               // initialize the lcd    
}

void loop()
{
}
