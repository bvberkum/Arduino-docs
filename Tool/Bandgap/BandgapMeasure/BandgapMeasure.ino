void setup( void )
{
	Serial.begin( 57600 );
	Serial.println( "\r\n\r\n" );

	delay( 1000 );

	// Set ref to 1.1V internal, measure externally.
	//analogReference( INTERNAL );
	ADMUX = (INTERNAL << 6) | 8;
	// XXX then use result value for correction?
}

void loop( void )
{
}
