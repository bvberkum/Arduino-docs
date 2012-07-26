

#ifndef __MYDEFS_H
#define __MYDEFS_H

#define F_16
#define F_CPU      16000000    //processor clock

#define	 	LCD

#define		LED_PORT	PORTG
#define 	LED			PG0

#define NOP() asm volatile ("nop" ::)
#define sbi(portn, bitn) 	((portn)|=(1<<(bitn)))
#define cbi(portn, bitn) 	((portn)&=~(1<<(bitn)))

#define  F_CPU   16000000

#endif //__MYDEFS_H
