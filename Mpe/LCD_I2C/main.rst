- Display header [DISPLAY] pin 16 must be backlight minus.
- Choose 3.3 or 5 volt backlight
- Choose 3.3 or 5 volt logic level
- JeePort is ofcourse:

  1. PWR
  2. DIO
  3. GND
  4. VCC 
  5. AIO
  6. IRQ

- Either PWR or VCC to DISPLAY pin 2 (LOGIC) and 15 (LIGHT)
- The MCP23008 [IC1] pin 7 is NC.
- Of all 8 digital pins, 6 are needed for control, one may be 
  used as backlight switch, and the last spare IO is tied to JP1, which
  is for an optional switch SW.  
- The MCP23008 digital connections and DISPLAY header connections:

  10. P0: DISPLAY pin 11 (DB5)
  11. P1: DISPLAY pin 12 (DB6)
  12. P2: DISPLAY pin 13 (DB6)
  13. P3: DISPLAY pin 14 (DB6)
  14. P4: DISPLAY pin 4 (RS)
  15. P5: SW 1 (can be NC)
  16. P6: DISPLAY pin 6 (E)
  17. P7: DISPLAY pin 16 (BKL switch through R1, Q1, R3)

- Other MCP23008 [IC1] and DISPLAY pins:

  1. SCL: AIO
  2. SDA: DIO
  3. A2: VCC
  4. A1: GND
  5. A0: GND
  6. RST: VCC 
  7. : NC
  8. INT: IRQ
  9. VSS: GND
  18. VDD: VCC   

  * GND: DISPLAY pin 1 and 5
  * LOGIC: DISPLAY pin 2
  * R2 wiper: DISPLAY pin 3
  * NC: DISPLAY pins 7, 8, 9, 10.

    - NC: DISPLAY pin 7 (DB0)
    - NC: DISPLAY pin 8 (DB2)
    - NC: DISPLAY pin 9 (DB3)
    - NC: DISPLAY pin 10 (DB4)

  * LIGHT: DISPLAY pin 15

- Q1 is the BLK switch, R1 (1k) and R3 (10) serve to limit current,
  Q1 value is unknown.
- R2 is a variable resistor (10k) to adjust contrast.
- One ceramic capacitor C1 0.1 uF between VCC and GND.
 

