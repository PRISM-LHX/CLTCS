//该函数用于控制DAC输出

#include "C8051F020.h"
#include "absacc.h"
#include "delay.h"
#include "data_define.c"
#include "Init_Device.c"

#include "get_adc.h" //获取当前adc采样值的头文件
#include "keyboard.h" //获取预设定温度

#define DAC XBYTE[0x4000] //为DAC分配地址
#define DAC_MAX 0xFF //设定DAC最大和最小输出，防止数据溢出
#define DAC_MIN 0x00

//选择PID算法：因为PID算法具有响应速度快、稳态误差小和抑制超调的特点

//确定PID参数

#define Kp 1.2 //比例系数
#define Ki 0.05 //积分系数
#define Kd 0.3 //微分系数

//PID算法所需要的上一次误差数据和多次误差累积的积分项

static int last_error = 0; //记录上一次误差数据
static float integral = 0.0; //记录积分项累积

void main(void){//测试后改名input_control
    
    unsigned char x=0; //输入到DAC的数字量
    unsigned char goal_temp=0; //预设定的温度
    unsigned char adc_current=0; //当前ADC的采样值
    unsigned int error=0; //当前时刻ADC的采样值与设定值的数字量误差
    float P_value=0, I_value=0, D_value=0; //定义误差与系数相乘后的PID三项数值
    float PID_value=0; //定义PID三项之和

    Init_Device();  //初始化系统

    while(1){

        //获取当前adc采样值
        adc_current = get_adc(); 
        //获取设定温度的数字量
        goal_temp = set_temperature; //keyboard.c函数需要多出来一个接口，把设定值的数字量传到这里！！！！
        //计算误差
        error = (int)(goal_temp)-(int)(adc_current);
        
        //计算PID三项参数和总参数

        //比例项：与当前误差成正比
        P_value = Kp * error;

        //积分项：需要积累误差
        //integral = integral + Ki * error;
        I_value = integral;

        //微分项：与误差变化率成正比
        D_value = Kd * (error - last_error); //变化率 = 当前误差-上次误差
        last_error = error; //记录当前误差为下一次计算变化率作准备

        //计算总控制量参数
        PID_value = P_value + I_value + D_value;

        //DAC输出限幅保护（防止超过或低于DAC量程）

        if (PID_value > DAC_MAX){

            x = DAC_MAX; //最大不能超过DAC输出的上限

        }

        else if (PID_value < DAC_MIN){

            x = DAC_MIN; //最小不能低于DAC输出的下限
        }

        else {
            x = (unsigned char) (PID_value);
            integral = integral + Ki * error;//输出未超出限幅时才进行积分项累积，避免积分饱和
        }

        //数字量输入DAC
        DAC = x;

        delay(10); //控制延时周期进入下一次循环
    }

}