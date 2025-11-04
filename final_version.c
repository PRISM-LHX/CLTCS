#include "C8051F020.h"
#include "absacc.h"
#include "data_define.c"
#include "Init_Device.c"
#include "math.h"
 
#define CS1        XBYTE[0x20AA]
#define DP1        XBYTE[0x0000]
#define DP2        XBYTE[0x0001]
#define DP3        XBYTE[0x0002]
#define DP4        XBYTE[0x0003]
#define DAC        XBYTE[0x4000]
#define DAC_MAX    0xFF
#define DAC_HALF   0x80
#define DAC_MIN    0x00

#define Kp         0.4
#define Ki         0.02
#define Kd         0.5

unsigned char num[] = {0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0x80,0x90};
unsigned char place_table[4][4] = {{0,1,2,3},{4,5,6,7},{8,9,0x0A,0x0B},{0x0C,0x0D,0x0E,0x0F}};
unsigned char press_prestate[4][4] = {{1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1}};
unsigned char press_curstate[4][4] = {{1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1}};
int set_temperature =0;
unsigned char adc_val = 0;
static int last_error = 0;
static float integral = 0.0;

unsigned char get_adc(void);
unsigned char read_adc(void);
void display(unsigned char x);
int read_from_keyboard(unsigned char *row_value, unsigned char *column_value);
void key_process(unsigned char *row_value, unsigned char *column_value, int *tens_digit, int *ones_digit, unsigned char *process_flag);
void input_control(void);
void delay(unsigned int x);

enum state
{
    SYS_INIT,
    SYS_INPUT,
    SYS_RUNNING
} SystemState;

void main(void)
{
    unsigned char row = 0, column = 0, process_flag = 0;
    int tens_digit = 0, ones_digit = 0;
    
    SystemState = SYS_INIT;
    while(1)
    {
        switch(SystemState)
        {
            case SYS_INIT:
                Init_Device();
                DP1 = DP2 = 0xFF;
                DP3 = DP4 = 0xFF;
                get_adc();
                SystemState = SYS_INPUT;
                break;
            
            case SYS_INPUT:
                get_adc();
                if(read_from_keyboard(&row, &column))
                {
                    key_process(&row, &column, &tens_digit, &ones_digit, &process_flag);
                    if(process_flag)
                    {
                        SystemState = SYS_RUNNING;
                    }
                }
                break;
            
            case SYS_RUNNING:
                if(read_from_keyboard(&row, &column))
                {
                    key_process(&row, &column, &tens_digit, &ones_digit, &process_flag);
                    if(!process_flag)
                    {
                        SystemState = SYS_INPUT;
                        DP3 = DP4 = 0xFF;
                        break;
                    }
                }
                input_control();
                break;
        }
    }
}

unsigned char get_adc(void)
{ 
    unsigned char x = 0;
    x = read_adc();
    adc_val = x;
    delay(10);
    display(x);
    return x;
}

unsigned char read_adc(void)
{
    unsigned char adc_value = 0;
    CS1 = 0;
    delay(1);
    adc_value = CS1;
    CS1 = 1;
    return adc_value;
}


void display(unsigned char x)
{
    int temp = 0;
    temp = (x * 100) / 255;
    DP1 = num[temp / 10];
    DP2 = num[temp % 10];
    delay(30);
}

int read_from_keyboard(unsigned char *row_value, unsigned char *column_value)
{ 
    unsigned char row_state = 0, check_num = 0, counter = 0;
    int addr = 0x0004;
    int i = 0;

    for(i = 1; i < 5; i++, addr++)
    {  
        row_state = ~XBYTE[addr] & 0x0F;
        if(row_state != 0)
        {
            for(counter = 1, check_num = 0x01; counter < 5; counter++, check_num <<= 1)
            {
                if((row_state & check_num) != 0)
                {
                    *row_value = counter;
                    *column_value = i;
                    return 1;
                }
            }
        }
    }
    return 0;
}

void key_process(unsigned char *row_value, unsigned char *column_value, int *tens_digit, int *ones_digit, unsigned char *process_flag)
{
    static unsigned char digit = 0;
    int j = 0, k = 0;
    unsigned char key_val = 0;

    if(press_prestate[*row_value - 1][*column_value - 1] == 1)
    {
        delay(20);
        if(!read_from_keyboard(row_value, column_value))
            return;

        key_val = place_table[*row_value - 1][*column_value - 1];

        if(key_val == 0x0A)
        {
            *process_flag = 1;
            integral = DAC_HALF;
            last_error = 0;
            digit = 0;
        }
        if(key_val == 0x0B)
        {
            *process_flag = 0;
            digit = 0;
            DAC = DAC_HALF;
        }

        if(key_val <= 9)
        {
            if(digit == 0)
            {
                DP3 = DP4 = 0xFF;
                *tens_digit = key_val;
                DP3 = num[*tens_digit];
                digit = 1;
            }
            else
            {
                *ones_digit = key_val;
                DP4 = num[*ones_digit];
                set_temperature = 10 * (*tens_digit) + (*ones_digit);
                digit = 0;
            }
        }

        for(j = 0; j < 4; j++)
        {
            for(k = 0; k < 4; k++)
            {
                if(j == *row_value - 1 && k == *column_value - 1)
                    press_curstate[j][k] = 0;
                else
                    press_curstate[j][k] = 1;
                press_prestate[j][k] = press_curstate[j][k];
            }
        }
    }
    else
    {
        return;
    }
}

void input_control(void)
{
    unsigned char dac_out = 0;
    int goal_temp = 0;
    unsigned char adc_current = 0;
    int error = 0;
    float P_value = 0.0, I_value = 0.0, D_value = 0.0;
    float PID_value = 0.0;

    adc_current = get_adc();
    goal_temp = (set_temperature * 255) / 100;

    error = goal_temp - (int)adc_current;

    P_value = Kp * error;
    integral += Ki * error;
    I_value = integral;
    D_value = Kd * (error - last_error);
    last_error = error;

    PID_value = P_value + I_value + D_value;

    if(PID_value > DAC_MAX)
        dac_out = DAC_MAX;
    else if(PID_value < DAC_MIN)
        dac_out = DAC_MIN;
    else
        dac_out = (unsigned char)PID_value;

    DAC = dac_out;
    delay(10);
}

void delay(unsigned int x)
{ 
    unsigned long i = 0;
    for(i = 0; i < 1000 * x; i++);
}