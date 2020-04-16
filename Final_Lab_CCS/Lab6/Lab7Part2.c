/*
 * Name:        Shida Yang
 * Lab #:       23195
 * Description: This program creates a real-time high-pass FIR filter
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
//length of FIR
#define FIR_length 23

volatile Uint32 counter;
volatile bool sampleFlag, isLeft;
volatile float32 sampleInL, sampleInR, sampleOutL, sampleOutR;

//FIR filter coefficient, copied from MATLAB
volatile float filter_coefficients[FIR_length]={
                                                0.005902401183384577,
                                                0.076846394023601231,
                                                0.006215876034889533,
                                                0.003262112032670176,
                                                -0.011287659072465926,
                                                -0.030321815756076797,
                                                -0.052757546449996362,
                                                -0.076285184602404660,
                                                -0.098267251174580933,
                                                -0.116316651994173376,
                                                -0.128062786328539874,
                                                0.867789179332174587,
                                                -0.128062786328539874,
                                                -0.116316651994173376,
                                                -0.098267251174580933,
                                                -0.076285184602404660,
                                                -0.052757546449996362,
                                                -0.030321815756076797,
                                                -0.011287659072465926,
                                                0.003262112032670176,
                                                0.006215876034889533,
                                                0.076846394023601231,
                                                0.005902401183384577
};

//buffer
volatile int recent_samples[FIR_length*2];
//buffer pointer
volatile int32 recent_samples_ptr=0;

__interrupt void McbspbISR();
void produce_output();
void echo_back();

int main(){

    //init system clocks and get board speed running at 200 MHz
    InitSysCtrl();

    EALLOW; //enable protective reg write

    //disable all interrupt
    Interrupt_initModule();
    //init interrupt table
    Interrupt_initVectorTable();

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
            Uint16 sw_value=readSwitch();
            //if SW0=1 (up)
            if(sw_value&1==1){
                //produce filtered output
                produce_output();
            }
            //if not
            else{
                //play back input sound
                echo_back();
            }

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
 *      Produce output based on the FIR filter generated in MATLAB
 * Parameters:
 *      None
 * Return:
 *      None
 */
void produce_output(){
    GpioDataRegs.GPASET.bit.GPIO0=1;
    //save current input
    recent_samples[recent_samples_ptr]=(int)sampleInL;  //left
    recent_samples[recent_samples_ptr+1]=(int)sampleInR;//right
    //recent_samples_ptr=latest left sample

    //reset output sample
    sampleOutL=0;
    sampleOutR=0;

    //calculate current output sample
    for(int i=0; i<FIR_length*2; i+=2){
        //calculate current delay left sample
        int32 curr_delay_index=recent_samples_ptr-i+FIR_length*2;
        int32 left_ptr;
        if(curr_delay_index>=FIR_length*2){
            left_ptr=curr_delay_index-FIR_length*2;
        }
        else{
            left_ptr=curr_delay_index;
        }
        //calculate current delay right sample
        int32 right_ptr=left_ptr+1;
        //get current filter coefficient
        float curr_coefficient=filter_coefficients[i/2];
        //add current left and right delay sample*coefficient to sample output
        sampleOutL=sampleOutL+recent_samples[left_ptr]*curr_coefficient;
        sampleOutR=sampleOutR+recent_samples[right_ptr]*curr_coefficient;
    }

    //move buffer pointer
    int32 next_recent_samples_ptr=recent_samples_ptr+2;
    if(next_recent_samples_ptr>=FIR_length*2){
        recent_samples_ptr=next_recent_samples_ptr-FIR_length*2;
    }
    else{
        recent_samples_ptr=next_recent_samples_ptr;
    }
    //recent_samples_ptr=oldest left sample
    GpioDataRegs.GPACLEAR.bit.GPIO0=1;
}

/*
 * Description:
 *      Echo back input
 * Parameters:
 *      None
 * Return:
 *      None
 */
void echo_back(){
    sampleOutL=sampleInL;
    sampleOutR=sampleInR;
}
