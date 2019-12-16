/*

AtmegaEEPROM extends Node

 */



/* *** Globals and sketch configuration *** */
#define SERIAL_EN         1 /* Enable serial */
#define DEBUG             1 /* Enable trace statements */

#define _MEM              1 // Report free memory

#define CONFIG_VERSION    "nx1 "
#define CONFIG_EEPROM_START 0



#include <EEPROM.h>
#include <JeeLib.h>
#include <Wire.h>
#include <OneWire.h>
#include <DotmpeLib.h>
#include <mpelib.h>




const String sketch = "X-AtmegaEEPROM";
const int version = 0;

char node[] = "nx";
char node_id[7];


/* IO pins */
//              RXD      0
//              TXD      1
//              INT0     2
//              MOSI     11
//              MISO     12
//#       define _DBG_LED 13 // SCK



MpeSerial mpeser (57600);


/* *** InputParser *** {{{ */

Stream& cmdIo = cmdIo;
extern const InputParser::Commands cmdTab[] PROGMEM;
byte* buffer = (byte*) malloc(50);
const InputParser parser (buffer, 50, cmdTab);


/* *** /InputParser *** }}} */

/* *** Report variables *** {{{ */




/* *** /Report variables *** }}} */

/* *** Scheduled tasks *** {{{ */




/* *** /Scheduled tasks *** }}} */

/* *** EEPROM config *** {{{ */


struct Config {
  char node[2];
  int node_id;
  int version;
  char config_id[4];
  signed int temp_offset;
  //float temp_k;
  int temp_k;
} static_config = {
  /* default values */
  { node[0], node[1], }, 0, version,
  CONFIG_VERSION,
  TEMP_OFFSET, TEMP_K
};

Config config;


bool loadConfig(Config &c)
{
  unsigned int w = sizeof(c);

  if (
      //EEPROM.read(CONFIG_EEPROM_START + 1 ) == CONFIG_VERSION[3] &&
      //EEPROM.read(CONFIG_EEPROM_START + 2 ) == CONFIG_VERSION[2] &&
      //EEPROM.read(CONFIG_EEPROM_START + 3 ) == CONFIG_VERSION[1] &&
      //EEPROM.read(CONFIG_EEPROM_START) == CONFIG_VERSION[0]
      EEPROM.read(CONFIG_EEPROM_START + w - 3) == c.config_id[1] &&
      EEPROM.read(CONFIG_EEPROM_START + w - 4) == c.config_id[0]
  ) {

    for (unsigned int t=0; t<w; t++)
    {
      *((char*)&c + t) = EEPROM.read(CONFIG_EEPROM_START + t);
    }
    return true;

  } else {
#if SERIAL_EN && DEBUG
    cmdIo.println("No valid data in eeprom");
#endif
    return false;
  }
}

void writeConfig(Config &c)
{
  for (unsigned int t=0; t<sizeof(c); t++) {

    EEPROM.write(CONFIG_EEPROM_START + t, *((char*)&c + t));

    // verify
    if (EEPROM.read(CONFIG_EEPROM_START + t) != *((char*)&c + t))
    {
      // error writing to EEPROM
#if SERIAL_EN && DEBUG
      cmdIo.println("Error writing "+ String(t)+" to EEPROM");
#endif
    }
  }
#if _RFM12B
#endif //_RFM12B
}



/* *** /EEPROM config *** }}} */

/* *** Peripheral devices *** {{{ */

#if LDR_PORT
#endif

#if _DHT
/* DHTxx: Digital temperature/humidity (Adafruit) */
#endif // DHT

#if _LCD84x48
/* Nokkia 5110 display */
#endif // LCD84x48

#if _DS
/* Dallas OneWire bus with registration for DS18B20 temperature sensors */

#endif // DS

#if _RFM12B
/* HopeRF RFM12B 868Mhz digital radio */


#endif // RFM12B

#if _NRF24
/* nRF24L01+: nordic 2.4Ghz digital radio */


#endif // NRF24

#if _RTC
/* DS1307: Real Time Clock over I2C */


#endif // RTC

#if _HMC5883L
/* Digital magnetometer I2C module */


#endif // HMC5883L


/* *** /Peripheral devices *** }}} */

/* *** Peripheral hardware routines *** {{{ */


#if LDR_PORT
#endif

/* *** - PIR routines *** {{{ */
#if PIR_PORT
#endif // PIR_PORT
/* *** /- PIR routines *** }}} */

#if _DHT
/* DHT temp/rh sensor routines (AdafruitDHT) */

#endif // DHT

#if _LCD
#endif //_LCD

#if _LCD84x48


#endif // LCD84x48

#if _DS
/* Dallas DS18B20 thermometer routines */


#endif // DS

#if _RFM12B
/* HopeRF RFM12B 868Mhz digital radio routines */


#endif //_RFM12B

#if _NRF24
/* Nordic nRF24L01+ radio routines */


#endif // NRF24 funcs

#if _RTC
#endif //_RTC

#if _HMC5883L
/* Digital magnetometer I2C routines */


#endif // HMC5883L


/* *** /Peripheral hardware routines *** }}} */

/* *** UI *** {{{ */

/* *** /UI *** }}} */

/* *** UART commands *** {{{ */

#if SERIAL_EN

void help_sercmd(void) {
  cmdIo.println("n: print Node ID");
  cmdIo.println("c: print config");
  cmdIo.println("v: print version");
  cmdIo.println("N: set Node (3 byte char)");
  cmdIo.println("C: set Node ID (1 byte int)");
  cmdIo.println("W: load/save config EEPROM");
  cmdIo.println("E: erase EEPROM!");
  cmdIo.println("r: report now");
  cmdIo.println("m: measure now");
  cmdIo.println("x: reset");
  cmdIo.println("?/h: this help");
}

void config_sercmd() {
  cmdIo.print("c ");
  cmdIo.print(config.node);
  cmdIo.print(" ");
  cmdIo.print(config.node_id);
  cmdIo.print(" ");
  cmdIo.print(config.version);
  cmdIo.print(" ");
  cmdIo.print(config.config_id);
  cmdIo.println();
}

static void configNodeCmd(void) {
  cmdIo.print('n');
  cmdIo.print(' ');
  cmdIo.print(node_id);
  cmdIo.print('\n');
}

static void configVersionCmd(void) {
  cmdIo.print("v ");
  cmdIo.println(config.version);
  //cmdIo.print("v ");
  //showNibble(config.version);
  //cmdIo.println("");
}

// fwd decl.
void initConfig(void);

static void configSetNodeCmd() {
  const char *node;
  parser >> node;
  config.node[0] = node[0];
  config.node[1] = node[1];
  config.node[2] = node[2];
  initConfig();
  cmdIo.print("N ");
  cmdIo.println(config.node);
}

static void configNodeIDCmd() {
  parser >> config.node_id;
  initConfig();
  cmdIo.print("C ");
  cmdIo.println(node_id);
  serialFlush();
}

static void configToEEPROMCmd() {
  int write;
  parser >> write;
  if (write) {
    writeConfig(config);
  } else {
    loadConfig(config);
    initConfig();
  }
  cmdIo.print("W ");
  cmdIo.println(write);
}

void ctempoffset_sercmd(void) {
  char c;
  parser >> c;
  int v = c;
  if( v > 127 ) {
    v -= 256;
  }
  config.temp_offset = v;
  cmdIo.print("New offset: ");
  cmdIo.println(config.temp_offset);
  writeConfig(config);
}

void ctemp_sercmd(void) {
  cmdIo.print("Offset: ");
  cmdIo.println(config.temp_offset);
  cmdIo.print("K: ");
  cmdIo.println(config.temp_k);
  cmdIo.print("Raw: ");
  cmdIo.println(internalTemp(config.temp_offset, config.temp_k));
}

void tempCmd(void) {
  float temp_k = config.temp_k / 10;
  double t = ( internalTemp(config.temp_offset, temp_k) + config.temp_offset ) * temp_k ;
  cmdIo.println( t );
}

static void eraseEEPROM() {
  cmdIo.print("! Erasing EEPROM..");
  for (int i = 0; i<EEPROM_SIZE; i++) {
    char b = EEPROM.read(i);
    if (b != 0x00) {
      EEPROM.write(i, 0);
      cmdIo.print('.');
    }
  }
  cmdIo.println(' ');
  cmdIo.print("E ");
  cmdIo.println(EEPROM_SIZE);
}

#endif


/* *** UART commands *** }}} */

/* *** Initialization routines *** {{{ */

void initConfig(void)
{
  // See Prototype/Node
  sprintf(node_id, "%s%i", config.node, config.node_id);
}

void doConfig(void)
{
  /* load valid config or reset default config */
  if (!loadConfig(config)) {
    writeConfig(config);
  }
  initConfig();
}

void initLibs()
{
}


/* *** /Initialization routines *** }}} */

/* *** Run-time handlers *** {{{ */

void doReset(void)
{
  doConfig();

  tick = 0;
}

bool doAnnounce(void)
{
/* see CarrierCase */
#if SERIAL_EN && DEBUG
  cmdIo.print("\n[");
  cmdIo.print(sketch);
  cmdIo.print(".");
  cmdIo.print(version);
  cmdIo.println("]");
#endif // SERIAL_EN && DEBUG
  cmdIo.println(node_id);
  return false;
}


// readout all the sensors and other values
void doMeasure(void)
{
  int ctemp = internalTemp(config.temp_offset, (float) config.temp_k/10);
}



#if PIR_PORT

// send packet and wait for ack when there is a motion trigger
#endif // PIR_PORT



/* *** /Run-time handlers *** }}} */

/* *** InputParser handlers *** {{{ */

#if SERIAL_EN

void handshakeCmd() {
  int v;
  //char buf[7];
  //node.toCharArray(buf, 7);
  parser >> v;
  sprintf(node_id, "%s%i", node, v);
  cmdIo.print("handshakeCmd:");
  cmdIo.println(node_id);
  serialFlush();
}

const InputParser::Commands cmdTab[] = {
  { '?', 0, help_sercmd },
  { 'h', 0, help_sercmd },
  { 'c', 0, config_sercmd },
  { 'n', 0, configNodeCmd },
  { 'v', 0, configVersionCmd },
  { 'N', 3, configSetNodeCmd },
  { 'C', 1, configNodeIDCmd },
  { 'W', 1, configToEEPROMCmd },
  //{ 'o', 0, ctempconfig_sercmd },
  { 'T', 1, ctempoffset_sercmd },
  { 't', 0, tempCmd },
  { 'A', 0, handshakeCmd },
  { 'm', 0, doMeasure },
  { 'E', 0, eraseEEPROM },
  { 'x', 0, doReset },
  { 0 }
};

#endif // SERIAL_EN


/* *** /InputParser handlers *** }}} */

/* *** Main *** {{{ */


void setup(void)
{
#if SERIAL_EN
  mpeser.begin();
  mpeser.startAnnounce(sketch, String(version));
#if DEBUG || _MEM
  cmdIo.print(F("Free RAM: "));
  cmdIo.println(freeRam());
#endif
  serialFlush();
#endif

  initLibs();

  doReset();
}

void loop(void)
{
  parser.poll();

    serialFlush();
}

/* *** }}} */
