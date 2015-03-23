Listing of node firmware and characteristics.

- These are almost all Arduino projects. See also:

  - AVRNode
  - ESPNode

- All sketches include DotmpeLib/mpelib? Maybe document where not so.

================ == ====== ==================================================
Node             0  nx     
Serial           0         Should sync code with Node
Magnetometer     0  magn   HMC5803L based magnetometer
AdafruitDHT      0  adadht DHT11, DHT22/AM2302 temp/rh
ThermoLog84x48   0  templg 
================ == ====== ==================================================

Node
  Scheduler, Serial, Config

  - JeeLib Scheduler supports low-power sleep and timed events. 
    An additional UI loop keeps uC awake while user or interrupted peripheral
    interaction takes place.
  - The scheduler includes some experimentation with ANNOUNCE, and takes
    XXX some functions from SerialNode, should clean that up.
  - Should fix config loading to somethign useful in a generic way.

Serial
  Scheduler, Serial
Magnetometer
  Scheduler, Serial, RFM12
AdafruitDHT
  Scheduler, InputParser Serial
ThermoLog84x48
  Scheduler, InputParser Serial, LCD8xx48

X-SensorNode nx
   Scheduler, EEPROM Config, DSTempBus

   Need to sync with InputParser, test config

RF24Node  rf24n
  Scheduler, EEPROM Config, LCD84x48, DSTempBus, InputParser
  RF24Network

RelayBox
  TODO
Keypad1x4
  - Bare mpelib sketch w
