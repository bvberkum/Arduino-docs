#include "debug.h"


//int rate = 30;
//int pulse_delay = 5;
//int alt_delay = 150;

int counter = 0;
//int latch_delay = 1;
//int clock_delay = 4;

void setup() {
  initdebug();
  Serial.begin(56700);
  Serial.println("[LedSegmentDebug v1]");
//  pinMode(A2, OUTPUT);
//  pinMode(A3, OUTPUT);
//  pinMode(A4, OUTPUT);
//  pinMode(A5, OUTPUT);
}

bool dot = false;

void loop() {
  //testsegments();
  
  // increment
  counter++;
  if (counter == 9999) 
    counter = 1111;/*
    
    
  digitalWrite(A2, HIGH);
  digitalWrite(A3, HIGH);  
  digitalWrite(A4, HIGH);  
  digitalWrite(A5, HIGH);
 
 delay(70); 
     
  digitalWrite(A2, LOW);
  digitalWrite(A3, LOW);  
  digitalWrite(A4, LOW);  
  digitalWrite(A5, LOW);
 
 delay(70); */
  
  for (int c=0;c<8;c++)
  {
      debug_chars(counter, dot ? 1 : 0);
      delay(5);
  }

  dot != dot; 
  
  Serial.print("counter=");
  Serial.println(counter); 
}
