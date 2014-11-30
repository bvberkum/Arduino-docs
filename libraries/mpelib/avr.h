#if defined(__AVR_ATtiny84__) || defined(__AVR_ATtiny85__)
#define SRAM_SIZE       512
#define EEPROM_SIZE     512
#elif defined(__AVR_ATmega168__)
#define SRAM_SIZE       1024
#define EEPROM_SIZE     512
#elif defined(__AVR_ATmega328__) || defined(__AVR_ATmega328P__)
#define SRAM_SIZE       2048
#define EEPROM_SIZE     1024
#endif

extern int freeRam (void);
extern int usedRam (void);

