/**

 JeeNode Multi-character LED segment tester
 ==========================================

 :license: Public Domain
 :author: B. van Berkum  <dev@dotmpe.com>
 :data: 5 Aug. 2012
 
 
 This can be used to test a double digit, muxed (10 pin) segmented LED display
 using a JeeNode. It will require (wires to) all the four JeePorts and the 
 SER/PWR/EXT header.
 
 Adjust pin mapping for other 328P (compatible) ATmega uC's.
 
 Overview
 --------
 This code drives n number of LED segment character displays, by using 9 pins.
 With this pincount, all digits will display the same.
 
 To display different digits the display will need to be alternated.
 Ie. the leds are pulsed and consequently the brightness diminishes slightly.
 To mux the digits in this way, 8 + n pins are be needed.
 For an JeeNode example plug see MuxingSegmentCharPlug, which in addition
 of muxing several digits also muxes the 8 cathode pin using a shift register,
 reducing the pincount to 3 + 3 but requiring two shift registers.
 
 This tester still alternates the anodes to be able to identify the digit where a 
 short occurs.
 
 Usage
 --------
 There is no preferred sequence to connect the pins. 
 A4 (SDA on the 328) connects to pin 5 through an 91 Ohm resistor,
 A5 (SCL) idem to pin 10. The other 8 pins are ouputs to connect the rest.
 
 The blinking sequence incrementally lights all segments and then switches each off
 with a small noticable delay of 100 ms or so.
 
 The double-digit LED segment display 
 ------------------------------------
 On bigger packages, the pin numbers 1 and 10 can be read to the epoxy (0.56")
 but the smaller 0.39" does not show intelligble markings in the epoxy.
 The pin numbering is the same as that which is standard for DIP'ed IC's;
 starting at the lower left corner, going right, then left to the upper left corner.
 
 The two digits each have one shared anode, pin 5 for the second and pin 10 for 
 the first digit.
 
 Types are CPS05022FR (0.56") and DY3621K+ (0.36", both red, 30mW (and I think 
 2.4 fwd v drop).
 
 It is possible to break them and get strange results b/c of short circuits.
 Mine did fine these code notes and two 91 Ohm resistors at the anodes, but it 
 cost one half block on which I blew a single segment which now shorts the others
 if turned on. 
 
 The pinout is:
 
 1. G
 2. DP
 3. A
 4. F
 5. CA2
 6. D
 7. E
 8. C
 9. B
 10. CA1
 

 */

int digit1 = A4;
int digit2 = A5;

const int pinCnt = 8;
int pins[pinCnt] = { 
  4, A0,
  5, A1,
  6, A2,
  7, A3
};

void setup() {
  for (int i=0;i<pinCnt;i++) {
    pinMode(pins[i], OUTPUT);
  }
  pinMode(digit1, OUTPUT);
  digitalWrite(digit1, 1);

  pinMode(digit2, OUTPUT);
  digitalWrite(digit2, 1);
}

int enabled = 0;
int delay1 = 7;
//int delay2 = 120;
int rate = 16;

int i = 0;

void loop() {

  digitalWrite(pins[i], enabled);

  i++;

  if (i==pinCnt) {
    i=0;
    enabled = !enabled;
  }

  for (int x=0;x<rate;x++) {
    
    digitalWrite(digit1, 1);
    delay(delay1);
    digitalWrite(digit1, 0);
    
    digitalWrite(digit2, 1);
    delay(delay1);
    digitalWrite(digit2, 0);
    
  }
  //delay(delay2);
}
