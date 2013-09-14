/**
 */
#include <JeeLib.h>
#include <DHT.h>
#include <nRF24L01.h>


#define SERIAL  1   // set to 1 to also report readings on the serial port
#define DEBUG   1   // set to 1 to display each loop() run and PIR trigger

//#define LED_RED      5
//#define LED_YELLOW   6
//#define LED_GREEN    7

#define MEASURE_PERIOD  6 // how often to measure, in tenths of seconds
#define RETRY_PERIOD    10  // how soon to retry if ACK didn't come in
#define RETRY_LIMIT     5   // maximum number of times to retry
#define ACK_TIME        10  // number of milliseconds to wait for an ack
#define REPORT_EVERY    5   // report every N measurement cycles
#define SMOOTH          3   // smoothing factor used for running averages


uint8_t station_addr[5] = { 0xE4, 0xE4, 0xE4, 0xE4, 0xE4 }; 
uint8_t trans_addr[5] = { 0x98, 0x76, 0x54, 0x32, 0x10 };

// node packet payload

uint8_t light;
float temp;
float rhum;

// The scheduler makes it easy to perform various tasks at various times:

enum { MEASURE, REPORT, TASK_END };

static word schedbuf[TASK_END];
Scheduler scheduler (schedbuf, TASK_END);

// Other variables used in various places in the code:

static byte reportCount;    // count up until next report, i.e. packet send
//static byte myNodeID;       // node ID used for this unit

// Conditional code, depending on which sensors are connected and how:

//#define DHTTYPE DHT11   // DHT 11 / DHT 22 (AM2302) / DHT 21 (AM2301)
DHT dht(7, DHT11);
int ldr_pin = A3;

/* has to be defined because we're using the watchdog for low-power waiting */
ISR(WDT_vect) { Sleepy::watchdogEvent(); }
		
/** Generic routines */

static int smoothedAverage(int prev, int next, byte firstTime =0) {
    if (firstTime)
        return next;
    return ((SMOOTH - 1) * prev + next + SMOOTH / 2) / SMOOTH;
}

void blink(int led, int count, int length) {
  for (int i=0;i<count;i++) {
    digitalWrite (led, HIGH);
    delay(length);
    digitalWrite (led, LOW);
    delay(length);
  }
}

// wait a few milliseconds for proper ACK to me, return true if indeed received
//static byte waitForAck() {
//    MilliTimer ackTimer;
//    while (!ackTimer.poll(ACK_TIME)) {
//		status = nrf24_lastMessageStatus();
//		if(status == NRF24_TRANSMISSON_OK)
//		{
//			return 1;
//		}
//		else if(temp == NRF24_MESSAGE_LOST)
//		{                    
//			/* xxx move from irq */
//			blink(LED_RED, 2, 75);
//			return 0;
//		}
//        //if (rf12_recvDone() && rf12_crc == 0 &&
//        //        // see http://talk.jeelabs.net/topic/811#post-4712
//        //        rf12_hdr == (RF12_HDR_DST | RF12_HDR_CTL | myNodeID))
//        //    return 1;
//        //set_sleep_mode(SLEEP_MODE_IDLE);
//        //sleep_mode();
//		nrf24_powerDown();            
//    }
//    return 0;
//}

// readout all the sensors and other values
static void doMeasure() {
    uint8_t firstTime = rhum == 0; // special case to init running avg
    
	uint8_t light_new = ~ analogRead(ldr_pin) >> 2;
	light = smoothedAverage(light, light_new, firstTime);

	rhum = smoothedAverage(rhum, dht.readHumidity(), firstTime);
	temp = smoothedAverage(rhum, dht.readTemperature(), firstTime);
}

/**/

uint8_t data_array[3];

void doReport(void) {
	/* Automatically goes to TX mode */
	data_array[0] = light;
	data_array[1] = rhum;
	data_array[2] = rhum;
	nrf24_send(data_array);        
    #if SERIAL
        Serial.print("ROOM l:");
        Serial.print((int) light);
        Serial.print(" rh:");
        Serial.print((int) rhum);
        Serial.print(" t:");
        Serial.print((int) temp);
        Serial.println();
        delay(2); // make sure tx buf is empty before going back to sleep
    #endif
}

void rf24_config(uint8_t show=1) {
	/* initializes hardware pins */
	nrf24_init();
	
	/* RF channel: #2 , payload length: 4 */
	nrf24_config(2,4);

	/* Set the module's own address */
	nrf24_rx_address(rx_mac);
	
	/* Set the transmit address */
	nrf24_tx_address(tx_mac);

	// TODO: make it print settings to serial 
}


/* Main */

void setup(void)
{
	pinMode(LED_PIN, OUTPUT);
//	pinMode( LED_GREEN, OUTPUT );
//	pinMode( LED_YELLOW, OUTPUT );
//	pinMode( LED_RED, OUTPUT );

//	blink(LED_YELLOW, 1, 150);

    #if SERIAL || DEBUG
	Serial.begin(38400);
	Serial.println("RadioNodeRF24");
        rf24_config();
    #else
        rf24_config(0); // don't report info on the serial port
	#endif

    reportCount = REPORT_EVERY;     // report right away for easy debugging
    scheduler.timer(MEASURE, 0);    // start the measurement loop going
}

void loop(void)
{
    #if DEBUG
        Serial.print('.');
        delay(2);
    #endif

    switch (scheduler.pollWaiting()) {

        case MEASURE:
            // reschedule these measurements periodically
            scheduler.timer(MEASURE, MEASURE_PERIOD);
    
            doMeasure();

            // every so often, a report needs to be sent out
            if (++reportCount >= REPORT_EVERY) {
                reportCount = 0;
                scheduler.timer(REPORT, 0);
            }
            break;
            
        case REPORT:
			/* Send fresh report */
			//blink( LED_YELLOW, 2, 75 );
            doReport();
            break;
    }

	// xxx old
	return;
	/* Automatically goes to TX mode */
	nrf24_send(data_array);        
	
	/* Wait for transmission to end */
	while(nrf24_isSending());

	/* Make analysis on last tranmission attempt */
	temp = nrf24_lastMessageStatus();

	if(temp == NRF24_TRANSMISSON_OK)
	{                    
		//xprintf("> Tranmission went OK\r\n");
	}
	else if(temp == NRF24_MESSAGE_LOST)
	{                    
		//xprintf("> Message is lost ...\r\n");    
		//blink(LED_RED, 3, 150);
	}
	
	/* Retranmission count indicates the tranmission quality */
	temp = nrf24_retransmissionCount();
	//xprintf("> Retranmission count: %d\r\n",temp);

	/* Optionally, go back to RX mode ... */
	//nrf24_powerUpRx();

	/* Or you might want to power down after TX */

	//blink(LED_GREEN, 1, 75 );
	Sleepy::loseSomeTime(10000); /* 10 seconds low power mode */
}


