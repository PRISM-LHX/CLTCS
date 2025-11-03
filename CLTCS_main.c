//配置系统文件
#include "C8051F020.h"
#include "absacc.h"
#include "delay.h"
#include "get_adc.h"
#include "input_control.h"
#include "keyboard.h"
//#include "Temp_adjust_display.h"
#include "data_define.c"
#include "Init_Device.c"

enum state
{
    SYS_INIT,
    SYS_INPUT,
    SYS_RUNNING
}SystemState;

int main(void)
{
    unsigned char row=0,column=0,process_flag=0;
    int tens_digit=0,ones_digit=0;
    SystemState=SYS_INIT;
    while(1)
    {
        switch(SystemState)
        {
            case SYS_INIT:
                Init_Device(); //单片机初始化配置
                DP1=DP2=DP3=DP4=0xff;//初始状态让四个数码管全灭  
                set_temperature=0;//温度清零
                get_adc();
                SystemState=SYS_INPUT;//进入核心功能 
                break;
            case SYS_INPUT:
                if(read_from_keyboard(&row,&column))
                    key_process(&row,&column,&tens_digit,&ones_digit,&process_flag);
                    if(process_flag)//按下确认键					
						SystemState=SYS_RUNNING;
                break;
            case SYS_RUNNING:
                if(read_from_keyboard(&row,&column))
                    key_process(&row,&column,&tens_digit,&ones_digit,&process_flag);
                    if(!process_flag)//按下停止键则等待再次输入
                        SystemState=SYS_INPUT;
                input_control();
                break;
        }
    }
}