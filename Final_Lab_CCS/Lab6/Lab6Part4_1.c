/*
 * Name:        Shida Yang
 * Lab #:       23195
 * Description: This program creates a real-time reverb effect
 */

#include <F28x_Project.h>

#include "SPIB.h"
#include "SPIA.h"
#include "CodecGPIO.h"
#include "AIC23.h"
#include "Mcbsp.h"
#include "interrupt.h"

//buffer mask
#define BUF_MASK 0x3FFFF
//increment for reverb delay is 10ms
#define increment_s (10.0/1000)
//sampling rate is 46.875kHz
#define fs 46875

volatile Uint32 counter, curr_delay;
volatile bool sampleFlag, isLeft;
volatile float32 sampleInL, sampleInR, sampleOutL, sampleOutR, a=0.7;
volatile Uint16 curr_sw;

__interrupt void McbspbISR();
void produce_output();

int main(){

    //init system clocks and get board speed running at 200 MHz
    InitSysCtrl();

    EALLOW; //enable protective reg write
    //disable all interrupt
    Interrupt_initModule();
    //init interrupt table
    Interrupt_initVectorTable();
    //init GPIO for SW, PB, LED
        initGpio();
    //init SPIB
    initSpiGpio();
    initSpi();

    //init McBSPB as I2S
    initMcbspbI2S();
    //init SPIA
    InitSPIA();
    //init AIC23 board
    InitAIC23();

    //set counter to 0
    counter=0;
    //initialize sampleIn/sampleOut
    sampleInL=0;
    sampleOutL=0;
    sampleInR=0;
    sampleOutR=0;
    //initialize sampleFlag
    sampleFlag=0;
    curr_sw=readSwitch();
    curr_delay=(Uint32)((int32)(curr_sw*increment_s*fs));
    isLeft=0;

    //point INT to my ISR
    Interrupt_register(INT_MCBSPB_RX, &McbspbISR);
    //enable INT
    Interrupt_enable(INT_MCBSPB_RX);
    //enable global interrupt
    Interrupt_enableMaster();

    while(1){
        //both channels sampled
        if(sampleFlag){
            //produce output
            produce_output();
            //reset sample flag
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
 *      Produce output based on specified delay of DIP switch and
 *      the old data in SRAM
 * Parameters:
 *      None
 * Return:
 *      None
 */
void produce_output(){
    //read DIP switch value
    Uint16 new_sw=readSwitch();
    //update switch value and delay
    if(new_sw!=curr_sw){
        curr_delay=(Uint32)((int32)(new_sw*increment_s*fs));
        curr_sw=new_sw;
    }
    float32 delayedL;
    float32 delayedR;
    //switch value is 0, no delay, play original sound
    if(curr_sw==0){
        delayedL=sampleInL;
        delayedR=sampleInR;
    }
    //there is delay, so fetch data from SRAM
    else{
        delayedL=readFromSRAM((counter-curr_delay)&BUF_MASK);
        delayedR=readFromSRAM((counter-curr_delay+1)&BUF_MASK);
    }
    //mix current sample with old sample from SRAM
    sampleOutL=(1-a)*sampleInL+a*delayedL;
    sampleOutR=(1-a)*sampleInR+a*delayedR;
    //record current samples
    writeToSRAM(counter, sampleInL);
    counter=(counter+1)&BUF_MASK;
    writeToSRAM(counter, sampleInR);
    counter=(counter+1)&BUF_MASK;
}
