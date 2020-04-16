/*
 * Name:        Shida Yang
 * Lab #:       23195
 * Description: This program samples sound at 48kHz and play it back at
 *              different rates based on DIP switches
 */

#include <F28x_Project.h>

#include "SPIB.h"
#include "SPIA.h"
#include "CodecGPIO.h"
#include "AIC23.h"
#include "Mcbsp.h"
#include "interrupt.h"
#include "cputimer.h"

//buffer mask
#define BUF_MASK 0x3FFFF

//command for different sampling rates
#define SAMPLING_48KHz  ((8<<9)|0)
#define SAMPLING_32KHz  ((8<<9)|(12<<1))
#define SAMPLING_8KHz   ((8<<9)|(6<<1))

volatile Uint32 counter;
volatile bool isRecording, isPlaying, sampleFlag, isLeft;
volatile float32 sampleInL, sampleInR, sampleOutL, sampleOutR;
volatile Uint16 curr_sampling_rate;

__interrupt void McbspbISR();
__interrupt void PB_ISR();
__interrupt void Timer1_debounce_ISR();

void record();
void playback();
void initTimer();

int main(){

    //init system clocks and get board speed running at 200 MHz
    InitSysCtrl();

    EALLOW; //enable protective reg write

    //disable all interrupt
    Interrupt_initModule();
    //init interrupt table
    Interrupt_initVectorTable();

    //init SPIB
    initSpiGpio();
    initSpi();
    //init GPIO for SW, PB, LED
    initGpio();
    //init McBSPB as I2S
    initMcbspbI2S();
    //init SPIA
    InitSPIA();
    //init AIC23 board
    InitAIC23();
    //init timer1 (1ms)
    initTimer();

    //set counter to 0
    counter=0;
    //set states to false
    isRecording=0;
    isPlaying=0;
    //initialize sampleIn/sampleOut
    sampleInL=0;
    sampleOutL=0;
    sampleInR=0;
    sampleOutR=0;
    //initialize sampleFlag
    sampleFlag=0;
    //default sampling rate is 48kHz
    curr_sampling_rate=SAMPLING_48KHz;
    //initialize isLeft
    isLeft=0;

    //PB falling edge trigger
    //PB2(left)=GPIO16=INPUT6=XINT3
    //PB0(right)=GPIO14=INPUT14=XINT5

    //setup GPIO to input
    InputXbarRegs.INPUT6SELECT=16;
    InputXbarRegs.INPUT14SELECT=14;
    //setup XINT to falling edge
    XintRegs.XINT3CR.bit.POLARITY=2;
    XintRegs.XINT5CR.bit.POLARITY=2;
    //enable XINT
    XintRegs.XINT3CR.bit.ENABLE=1;
    XintRegs.XINT5CR.bit.ENABLE=1;
    //point int to ISR
    Interrupt_register(INT_XINT3, &PB_ISR);
    Interrupt_register(INT_XINT5, &PB_ISR);
    //enable int in PIE and IER
    Interrupt_enable(INT_XINT3);
    Interrupt_enable(INT_XINT5);

    //point INT to my ISR
    Interrupt_register(INT_MCBSPB_RX, &McbspbISR);
    Interrupt_register(INT_TIMER1, &Timer1_debounce_ISR);
    //enable INT
    Interrupt_enable(INT_MCBSPB_RX);
    Interrupt_enable(INT_TIMER1);
    //enable global interrupt
    Interrupt_enableMaster();

    while(1){
        //left and right channel data received
        //mode select
        if(sampleFlag){
            //if it is recording now
            if(isRecording){
                //record
                record();
            }
            //if it is playing now
            else if(isPlaying){
                //play
                playback();
            }
            //done processing, reset sampleFlag
            sampleFlag=0;
        }
        //sampling rate select
        Uint16 sw_value=readSwitch();
        //SW0
        if(sw_value==1){
            //set sampleing rate to 8kHz
            if(curr_sampling_rate!=SAMPLING_8KHz){
                SpiaTransmit (SAMPLING_8KHz);
                SmallDelay();
                //update current sampling rate
                curr_sampling_rate=SAMPLING_8KHz;
            }
        }
        //SW1
        else if(sw_value==2){
            //set sampleing rate to 32kHz
            if(curr_sampling_rate!=SAMPLING_32KHz){
                SpiaTransmit (SAMPLING_32KHz);
                SmallDelay();
                //update current sampling rate
                curr_sampling_rate=SAMPLING_32KHz;
            }
        }
        //SW2
        else if(sw_value==4){
            //set sampleing rate to 48kHz
            if(curr_sampling_rate!=SAMPLING_48KHz){
                SpiaTransmit (SAMPLING_48KHz);
                SmallDelay();
                //update current sampling rate
                curr_sampling_rate=SAMPLING_48KHz;
            }
        }

    }

}

/*
 * Description:
 *      ISR for McBSPB to receive sound and put them in
 *      sampleInL and sampleInR buffers. It also transmit
 *      sound in sampleOutL and sampleOutR based on isLeft.
 * Parameters:
 *      None
 * Return:
 *      None
 */
__interrupt void McbspbISR(){
    //left channel
    if(isLeft){
        //put input in sampleInL
        sampleInL=(int)McbspbRegs.DRR2.all;
        //discard lower 16 bits
        McbspbRegs.DRR1.all;
        //transmit left output
        McbspbRegs.DXR2.all=(int)sampleOutL;
        //discard lower 16 bits
        McbspbRegs.DXR1.all=0;
    }
    //right channel
    else{
        //put input in sampleInR
        sampleInR=(int)McbspbRegs.DRR2.all;
        //discard lower 16 bits
        McbspbRegs.DRR1.all;
        //transmit right output
        McbspbRegs.DXR2.all=(int)sampleOutR;
        //discard lower 16 bits
        McbspbRegs.DXR1.all=0;
        //both channel received
        sampleFlag=1;
    }
    //toggle isLeft flag
    isLeft=!isLeft;
    //ack interrupt
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP6);
}

/*
 * Description:
 *      ISR for push button. It starts timer1 to debounce (1ms)
 * Parameters:
 *      None
 * Return:
 *      None
 */
__interrupt void PB_ISR(){
    CPUTimer_resumeTimer(CPUTIMER1_BASE);
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP12);
}

/*
 * Description:
 *      ISR for timer1. Debounce: 1ms after PB is pressed, check
 *      the current PB state and decide action.
 * Parameters:
 *      None
 * Return:
 *      None
 */
__interrupt void Timer1_debounce_ISR(){
    //stop timer
    CPUTimer_stopTimer(CPUTIMER1_BASE);
    //reload count
    CPUTimer_reloadTimerCounter(CPUTIMER1_BASE);
    //when it is not recording, mixing, or playing
    //check which button is pressed
    if(!isRecording && !isPlaying){
        //if PB2 is pressed
        if(readPB()==4){
            //clear LED
            writeToLED(0);
            isRecording=1;
        }
        //PB0 is pressed
        else if(readPB()==1){
            //clear LED
            writeToLED(0);
            isPlaying=1;
        }
    }
    //ack group 13 interrupt for TIMER1
    Interrupt_clearACKGroup(INTERRUPT_CPU_INT13);
}

/*
 * Description:
 *      Record a left sound sample
 * Parameters:
 *      None
 * Return:
 *      None
 */
void record(){
    //write left sample into SRAM
    //only record left sample
    writeToSRAM(counter, (int16)sampleInL);
    //record for 5 seconds (at 46.7kHz)
    if(counter==((Uint32)46700*5)){
        //stop recording
        isRecording=0;
        //light up LED0
        writeToLED(1);
        //reset counter
        counter=0;
        return;
    }
    //inc counter
    counter=(counter+1)&BUF_MASK;
}

/*
 * Description:
 *      Play the same SRAM data through left and right channels
 * Parameters:
 *      None
 * Return:
 *      None
 */
void playback(){
    //read data (left channel)
    int16 data=readFromSRAM(counter);
    //play it back through both channels
    sampleOutL=data;
    sampleOutR=data;
    //play for 5 seconds (at 46.7kHz)
    if(counter==((Uint32)46700*5)){
        //stop playing
        isPlaying=0;
        //light up LED2
        writeToLED(4);
        //reset counter
        counter=0;
        return;
    }
    //inc counter
    counter=(counter+1)&BUF_MASK;
}

/*
 * Description:
 *      Initialize timer1 to overflow evert 1ms
 * Parameters:
 *      None
 * Return:
 *      None
 */
void initTimer(){
    // initialize the timer structs and setup the timers in their default states
    InitCpuTimers();

    //timer1, cpu freq=200MHz, timer period=1000us
    ConfigCpuTimer(&CpuTimer1, 200, 1000);

}
