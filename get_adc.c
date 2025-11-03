#include "C8051F020.h"
#include "absacc.h"
#include "delay.h"
#include "data_define.c"
#define    CS1  XBYTE[0x20AA]
#include "Init_Device.c"
unsigned char num[]={0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0x80,0x90,0x7f};
#define    LED1      XBYTE[0x0000]
#define    LED2      XBYTE[0x0001]
//#define filter_core 3  滤波的内容先不考虑
//static unsigned char last_filtered = 0;

void delay(uint x);
unsigned char read_adc(void);
unsigned char ave_adc(void);
unsigned char filter(void);
void display(unsigned char x);

void main(void)//测试后改名get_adc
{ 
  unsigned char x=0;
  Init_Device();
 
  //last_filtered = read_adc();
	while(1)
	{ 
		x = ave_adc();
		delay(10);
		display(x);
	}
}

unsigned char read_adc(void)
{
	unsigned char adc_value=0;
	CS1 = 0;         // 启动ADC转换
	delay(10);       // 等待转换完成
	adc_value = CS1; // 读取转换结果
	return adc_value;
}

/*unsigned char filter(void)
{
	unsigned char curr_adc;  
	curr_adc = read_adc();
	
	last_filtered = (last_filtered * (filter_coe - 1) + curr_adc) / filter_coe;
	return last_filtered;
}*/

void delay(uint x)
{ 
  unsigned long i=0;
	for(i=0;i<1000*x;++i) i=i; 
}

unsigned char ave_adc(void)
{
	int count = 10;
	int i=0;
	int sum = 0;

	for(i = 0; i < count; i++)
    {
		//sum += filter();  // 累加滤波后的值
		sum += read_adc();
	}
	return (unsigned char)(sum / count);
}

void display(unsigned char x)
{
	int i=0;
	x = (x*100)/255;  // 转换为0-100的百分比
	i = x%10;         // 个位
	x = (x-i)/10;     // 十位
	LED1 = num[x];    // 显示十位
	LED2 = num[i];    // 显示个位
	delay(30);
}