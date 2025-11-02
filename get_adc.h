#ifndef __GET_ADC_H__
#define __GET_ADC_H__

#define    CS1       XBYTE[0x20AA]
#define    LED1      XBYTE[0x0000]
#define    LED2      XBYTE[0x0001]

extern unsigned char num[];

//void delay(uint x);
unsigned char read_adc(void);
unsigned char ave_adc(void);
unsigned char filter(void);
void display(unsigned char x);

#endif