/*
 Copyright (C) 2011 J. Coliz <maniacbug@ymail.com>
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */
 
/**
 * @file printf.h
 *
 * Setup necessary to direct stdout to the Arduino Serial library, which
 * enables 'printf'
 */

#ifndef __PRINTF_H__
#define __PRINTF_H__

#ifdef ARDUINO

int serial_putc( char c, FILE * ) 
{
  Serial.write( c );

  return c;
} 

void printf_begin(void)
{
  fdevopen( &serial_putc, 0 );
}

#else
#error This example is only for use on Arduino.
#endif // ARDUINO

#endif // __PRINTF_H__


#ifdef SUPPORT_LONGLONG

void Print::println(int64_t n, uint8_t base)
{
	  print(n, base);
	  println();
}

void Print::print(int64_t n, uint8_t base)
{
	  if (n < 0)
		  {
			    print((char)'-');
			    n = -n;
			  }
			  if (base < 2) base = 2;
			  print((uint64_t)n, base);
}

void Print::println(uint64_t n, uint8_t base)
{
	  print(n, base);
	  println();
}

void Print::print(uint64_t n, uint8_t base)
{
	  if (base < 2) base = 2;
	  printLLNumber(n, base);
}

void Print::printLLNumber(uint64_t n, uint8_t base)
{
	  unsigned char buf[16 * sizeof(long)]; 
	  unsigned int i = 0;

	  if (n == 0) 
		  {
			    print((char)'0');
			    return;
			  }

			  while (n > 0) 
				  {
					    buf[i++] = n % base;
					    n /= base;
					  }

					  for (; i > 0; i--)
						    print((char) (buf[i - 1] < 10 ?
									      '0' + buf[i - 1] :
									      'A' + buf[i - 1] - 10));
}
#endif

