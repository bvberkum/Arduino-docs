/* AtmegaSRAM - measure memory usage */

/** Globals and sketch configuration  */
/* *** Globals and sketch configuration *** */
#define SERIAL          1 /* Enable serial */
#define DEBUG           1 /* Enable trace statements */
							
#define DEBUG_MEASURE   1


static String sketch = "AtmegaSRAM";
static String node = "";
static String version = "0";

int memfree;

int freeRam () {
	extern int __heap_start, *__brkval; 
	int v; 
	return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

static void doMeasure()
{
	memfree = freeRam();
#if SERIAL && DEBUG_MEASURE
	Serial.print(" MEM free ");
	Serial.println(memfree);
#endif
}

void setup(void) 
{
#if SERIAL
	Serial.begin(57600);
	Serial.println();
	Serial.print("[");
	Serial.print(sketch);
	Serial.print(".");
	Serial.print(version);
	Serial.print("]");
#endif
	doMeasure();
}

void loop(void)
{
}
