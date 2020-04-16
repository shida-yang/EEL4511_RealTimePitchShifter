/*
 * Name:        Shida Yang
 * Lab #:       23195
 * Description: This program uses a circular buffer and three PBs
 *              to record, mix, and play sound.
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

volatile Uint32 counter;
volatile bool isRecording, isMixing, isPlaying, sampleFlag, isLeft;
volatile float32 sampleInL, sampleInR, sampleOutL, sampleOutR;

__interrupt void McbspbISR();
__interrupt void PB_ISR();
__interrupt void Timer1_debounce_ISR();

void record();
void mix();
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
    isMixing=0;
    isPlaying=0;
    //initialize sampleIn/sampleOut
    sampleInL=0;
    sampleOutL=0;
    sampleInR=0;
    sampleOutR=0;
    //initialize sampleFlag
    sampleFlag=0;
    //initialize isLeft
    isLeft=0;

    //PB falling edge trigger
    //PB2(left)=GPIO16=INPUT6=XINT3
    //PB1(middle)=GPIO15=INPUT13=XINT4
    //PB0(right)=GPIO14=INPUT14=XINT5

    //setup GPIO to input
    InputXbarRegs.INPUT6SELECT=16;
    InputXbarRegs.INPUT13SELECT=15;
    InputXbarRegs.INPUT14SELECT=14;
    //setup XINT to falling edge
    XintRegs.XINT3CR.bit.POLARITY=2;
    XintRegs.XINT4CR.bit.POLARITY=2;
    XintRegs.XINT5CR.bit.POLARITY=2;
    //enable XINT
    XintRegs.XINT3CR.bit.ENABLE=1;
    XintRegs.XINT4CR.bit.ENABLE=1;
    XintRegs.XINT5CR.bit.ENABLE=1;
    //point int to ISR
    Interrupt_register(INT_XINT3, &PB_ISR);
    Interrupt_register(INT_XINT4, &PB_ISR);
    Interrupt_register(INT_XINT5, &PB_ISR);
    //enable int in PIE and IER
    Interrupt_enable(INT_XINT3);
    Interrupt_enable(INT_XINT4);
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
        if(sampleFlag){
            //if it is recording now
            if(isRecording){
                //record
                record();
            }
            //if it is mixing now
            else if(isMixing){
                //mix
                mix();
            }
            //if it is playing now
            else if(isPlaying){
                //play
                playback();
            }
            //done processing this set of data, reset sampleFlag
            sampleFlag=0;
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
    if(!isRecording && !isMixing && !isPlaying){
        //if PB2 is pressed
        if(readPB()==4){
            //clear LED
            writeToLED(0);
            //start recording
            isRecording=1;
        }
        //PB1 is pressed
        else if(readPB()==2){
            //clear LED
            writeToLED(0);
            //start mixing
            isMixing=1;
        }
        //PB0 is pressed
        else if(readPB()==1){
            //clear LED
            writeToLED(0);
            //start playing
            isPlaying=1;
        }
    }
    //ack group 13 interrupt for TIMER1
    Interrupt_clearACKGroup(INTERRUPT_CPU_INT13);
}

/*
 * Description:
 *      Record a set of left and right sound sample
 * Parameters:
 *      None
 * Return:
 *      None
 */
void record(){
    //write left sample into SRAM
    writeToSRAM(counter, (int16)sampleInL);
    //write right sample into next address
    counter=(counter+1)&BUF_MASK;
    writeToSRAM(counter, (int16)sampleInR);
    //done writing sample (SRAM full)
    if(counter==0x3FFFF){
        //finish recording
        isRecording=0;
        //light up LED0
        writeToLED(1);
    }
    //increment counter to next set of samples
    counter=(counter+1)&BUF_MASK;
}

/*
 * Description:
 *      Record a set of left and right sound sample
 * Parameters:
 *      None
 * Return:
 *      None
 */
void mix(){
    //get left sample from SRAM
    float32 data=(int)readFromSRAM(counter);
    //mix current left sample with left sample from SRAM
    data=(data*0.5+sampleInL*0.5);
    //write back the mixed data
    writeToSRAM(counter, (int16)data);
    //do the same thing for right sample
    counter=(counter+1)&BUF_MASK;
    data=(int)readFromSRAM(counter);
    data=(data*0.5+sampleInR*0.5);
    writeToSRAM(counter, (int16)data);
    //if at the end of SRAM
    if(counter==0x3FFFF){
        //finish mixing
        isMixing=0;
        //light up LED1
        writeToLED(2);
    }
    //increment counter to next set of samples
    counter=(counter+1)&BUF_MASK;
}

/*
 * Description:
 *      Play a set of left and right sound sample
 * Parameters:
 *      None
 * Return:
 *      None
 */
void playback(){
    //read left sample
    Uint16 data=readFromSRAM(counter);
    //put it in sampleOutL
    sampleOutL=(int)data;
    //get the right channel data
    counter=(counter+1)&BUF_MASK;
    data=readFromSRAM(counter);
    //put it in sampleOutR
    sampleOutR=(int)data;
    //if at the end of SRAM
    if(counter==0x3FFFF){
        //stop playing
        isPlaying=0;
        //light up LED2
        writeToLED(4);
    }
    //increment counter to next set of data
    counter=(counter+1)&BUF_MASK;
}

/*
 * Description:
 *      Initialize timer1 to overflow every 1ms
 * Parameters:
 *      None
 * Return:
 *      None
 */
void initTimer(){
    // initialize the timer structs and setup the timers in their default states
    InitCpuTimers();

    //timer1, cpu freq=200MHz, timer period=1ms
    ConfigCpuTimer(&CpuTimer1, 200, 1000);

}
