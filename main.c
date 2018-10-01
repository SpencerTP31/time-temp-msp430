#include <msp430.h>
#include "peripherals.h"
#include "stringstuff.h"
#include <stdint.h>
#include <stdlib.h>

typedef enum {S_RUN, S_EDIT} state_t;
typedef enum {E_MON, E_DAY, E_HR, E_MIN, E_SEC} editstate_t;

// Function Prototypes
void swDelay(char numLoops);
void adc_convert(void);
void runtimerA2(void);
void stoptimerA2(void);
void a2delay(float numLoops);
void toArray(int i, char *s);
char displayTemp(float rawTemp, int tempType);
int read_push_button(void);
//void displayStuff(int count);

// Declare globals here

#define CALADC12_15V_30C  *((unsigned int *)0x1A1A)
#define CALADC12_15V_85C  *((unsigned int *)0x1A1C)

struct tm * global_time; //Global time counter.
struct tm * last_global_time; //Global time counter.
volatile unsigned int in_temp;
volatile unsigned int in_wheel;
volatile char temp_changed = 0;
unsigned long times[60];
float tempC[60];
unsigned int timercount;
unsigned int delay_cnt = 0;
char* time_str;
char* last_time_str;
char* date_str;
char* temp_c_str;
char* temp_f_str;
unsigned long utc;

#pragma vector=TIMER2_A0_VECTOR
__interrupt void TimerA2_ISR(void) {
    timercount++;
    delay_cnt++;
    if (timercount == 60000)
        timercount = 0;
    if (timercount % 100 == 0)
    {
        increment_tm(global_time, 1);
        adc_convert();
    }
}

#pragma vector=ADC12_VECTOR
__interrupt void ADC12_ISR(void) {
    in_temp = ADC12MEM0;
    in_wheel = ADC12MEM1;
    temp_changed = 1;
}


// Main
void main(void) {
    volatile float temperatureDegC;
    volatile float temperatureDegF;
    volatile float degC_per_bit;
    volatile unsigned int bits30, bits85;
    // Stop WDT
    WDTCTL = WDTPW | WDTHOLD;       // Stop watchdog timer

    REFCTL0 &= ~REFMSTR;    // Reset REFMSTR to hand over control of
    // internal reference voltages to
    // ADC12_A control registers

    P8SEL &= ~BIT0;
    P8DIR |= BIT0;
    P8OUT |= BIT0;

    ADC12CTL0 = ADC12SHT0_9 | ADC12REFON | ADC12ON | ADC12MSC;     // Internal ref = 1.5V
    ADC12CTL1 = ADC12SHP | ADC12CONSEQ_1;                     // Enable sample timer
    // Using ADC12MEM0 to store reading
    ADC12MCTL0 = ADC12SREF_1 + ADC12INCH_10;  // ADC i/p ch A10 = temp sense
    ADC12MCTL1 = ADC12SREF_0 + ADC12INCH_5 + ADC12EOS;

    __delay_cycles(100);                    // delay to allow Ref to settle
    ADC12CTL0 |= ADC12ENC;                // Enable conversion
    // Use calibration data stored in info memory
    bits30 = CALADC12_15V_30C;
    bits85 = CALADC12_15V_85C;
    degC_per_bit = ((float)(85.0 - 30.0)) / ((float)(bits85 - bits30));

    ADC12IE = BIT1;
    _BIS_SR(GIE);

    global_time = (struct tm*)malloc(sizeof(struct tm));
    last_global_time = (struct tm*)malloc(sizeof(struct tm));
    initialize_tm(global_time);
    initialize_tm(last_global_time);
    configDisplay();
    configKeypad();
    runtimerA2();

    Graphics_clearDisplay(&g_sContext); // Clear the display

    state_t state = S_RUN;
    editstate_t editstate = E_MON;

    time_str = (char*)malloc(sizeof(char) * 9);
    last_time_str = (char*)malloc(sizeof(char) * 9);
    date_str = (char*)malloc(sizeof(char) * 7);
    temp_c_str = (char*)malloc(sizeof(char) * 8);
    temp_f_str = (char*)malloc(sizeof(char) * 8);
    convert_time(time_str, global_time);
    convert_time(last_time_str, global_time);
    convert_date(date_str, global_time);

    volatile int btn;
    const int mon_days[] =
          {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    while(1) {
        btn = getKey();

        switch (state)
                {
                case S_RUN:
                    if (btn == '1') {
                        stoptimerA2();
                        state = S_EDIT;
                    }
                    break;
                case S_EDIT:
                    if (btn == '2') {
                        runtimerA2();
                        state = S_RUN;
                        editstate = E_MON;
                        mktime(global_time);
                        break;
                    }
                    adc_convert();
                    switch (editstate) {
                    case E_MON:
                        if (btn == '1')
                            editstate = E_DAY;
                        global_time->tm_mon = map(in_wheel, 0, 4085, 0, 11);
                        break;
                    case E_DAY:
                        if (btn == '1')
                            editstate = E_HR;
                        global_time->tm_mday = map(in_wheel, 0, 4085, 1, mon_days[global_time->tm_mon]);
                        break;
                    case E_HR:
                        if (btn == '1')
                            editstate = E_MIN;
                        global_time->tm_hour = map(in_wheel, 0, 4085, 0, 23);
                        break;
                    case E_MIN:
                        if (btn == '1')
                            editstate = E_SEC;
                        global_time->tm_min = map(in_wheel, 0, 4085, 0, 59);
                        break;
                    case E_SEC:
                        if (btn == '1')
                            editstate = E_MON;
                        global_time->tm_sec = map(in_wheel, 0, 4085, 0, 59);
                        break;
                    }
                    break;
                }

        if (temp_changed) {
            temperatureDegC = (float) ((long) in_temp - CALADC12_15V_30C )
                    * degC_per_bit + 30.0;
            temperatureDegF = temperatureDegC * 9.0 / 5.0 + 32.0;
            temp_changed = 0;
        }

        convert_temp(temp_c_str, temperatureDegC, 0);
        convert_temp(temp_f_str, temperatureDegF, 1);
        convert_time(time_str, global_time);
        convert_date(date_str, global_time);
        utc = mktime(global_time);

        times[utc % 60] = utc;
        tempC[utc % 60] = temperatureDegC;

//        if (global_time->tm_sec != last_global_time->tm_sec) {
            Graphics_clearDisplay(&g_sContext);
            Graphics_drawStringCentered(&g_sContext, time_str, AUTO_STRING_LENGTH, 51, 10, TRANSPARENT_TEXT);
            Graphics_drawStringCentered(&g_sContext, date_str, AUTO_STRING_LENGTH, 51, 20, TRANSPARENT_TEXT);
            Graphics_drawStringCentered(&g_sContext, temp_c_str, AUTO_STRING_LENGTH, 51, 30, TRANSPARENT_TEXT);
            Graphics_drawStringCentered(&g_sContext, temp_f_str, AUTO_STRING_LENGTH, 51, 40, TRANSPARENT_TEXT);
            Graphics_flushBuffer(&g_sContext); // update display
//            last_global_time->tm_sec = global_time->tm_sec;
//        }
    }
}

//void displayStuff(int count) {
//    int monthDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
//    char month[12][4] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
//    int days = count/86400;
//    int hours = (count%86400)/3600;
//    int minutes = ((count%86400)%3600)/60;
//    int seconds = (((count%86400)%3600)%60);
//
//    int months = 0;
//    int totalDayMonths = 0;
//    int tempTotalDayMonths = 31;
//    int i = 0;
//    while(tempTotalDayMonths < days) {
//        totalDayMonths += monthDays[i];
//        tempTotalDayMonths += monthDays[i+1];
//        months++;
//        i++;
//    }
//
//    char *sDays[4];
//    char *sHours[4];
//    char *sMin[4];
//    char *sSec[4];
//
//    toArray(days, sDays);
//    toArray(hours, sHours);
//    toArray(minutes, sMin);
//    toArray(seconds, sSec);
//
//    days -= totalDayMonths;
//
//    char disp[9][5];
//    disp[0] = month[months];
//    disp[1] = ':';
//    disp[2] = sDays;
//    disp[3] = ':';
//    disp[4] = sHours;
//    disp[5] = ':';
//    disp[6] = sMin;
//    disp[7] = ':';
//    disp[8] = sSec;
//
//    Graphics_clearDisplay(&g_sContext);
//    Graphics_drawStringCentered(&g_sContext, disp, AUTO_STRING_LENGTH, 48, 15, TRANSPARENT_TEXT);
//    Graphics_flushBuffer(&g_sContext); // update display
//}

void toArray(int i,char *s) { // Convert Integer to String
    char *p;
    p = s;
    p[2]=i%10;
    i-=p[2];
    i/=10;
    p[1]=i%10;
    i-=p[1];
    i/=10;
    p[0]=i%10;
    i-=p[0];
    p[3] = 0; // mark end of string
    p[2]+=0x30;
    p[1]+=0x30;
    p[0]+=0x30;
}


void adc_convert() {
    ADC12CTL0 &= ~ADC12SC;  // clear the start bit
    ADC12CTL0 |= ADC12SC;       // Sampling and conversion start
}

void runtimerA2(void) {
    TA2CTL = TASSEL_1 + MC_1 + ID_0;
    TA2CCR0 = 327;
    TA2CCTL0 = CCIE;
}

void stoptimerA2(void) {
    TA2CTL = MC_0;
    TA2CCTL0 &= ~CCIE;
}

void a2delay(float numLoops) {
    delay_cnt = 0; // our timer_cnt increments ~100 times per second
    while(delay_cnt < numLoops){
        //weee! we're waiting around and having fun :)
    }
}

void swDelay(char numLoops)
{
	// This function is a software delay. It performs
	// useless loops to waste a bit of time
	//
	// Input: numLoops = number of delay loops to execute
	// Output: none
	//
	// smj, ECE2049, 25 Aug 2013

	volatile unsigned int i,j;	// volatile to prevent removal in optimization
			                    // by compiler. Functionally this is useless code

	for (j=0; j<numLoops; j++)
    {
    	i = 50000 ;					// SW Delay
   	    while (i > 0)				// could also have used while (i)
	       i--;
    }
}
