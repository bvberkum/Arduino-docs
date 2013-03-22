Issues

- RF24 can sent 32 bytes at most, only haf of JeeNode/RF12
- 

Roles
  reporter
    - nodes reports changes for attached sensors, or last value upon report.
    - normally report over radio.
 
  logger
    - node reads radio packets, writes to serial. 
      Keep some log stats?

Ideas:
  repeater
    - node repeats what it heard, either serial or radio

  	

Modules
  nRF24L01+
    - 40bit addresses, a module can send to 1 and listen to several addresses
    - Module preferably resides in low-power mode, a write or startListening
      will wake it up.
    - Maskable interrupts at send, received and max retries.


Commands
  - byte: 255 commands 
