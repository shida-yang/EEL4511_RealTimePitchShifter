/*
 * Name:        Shida Yang
 * Lab #:       23195
 * Description: This program creates a real-time band-pass FIR filter
 */

#include <F28x_Project.h>

#include "SPIB.h"
#include "SPIA.h"
#include "CodecGPIO.h"
#include "AIC23.h"
#include "Mcbsp.h"
#include "interrupt.h"

#include "fpu_filter.h"

//buffer mask
#define BUF_MASK 0x3FFFF
#define FIR_ORDER       42

//FIR filters for left and right channels
FIR_FP  firFPL = FIR_FP_DEFAULTS;
FIR_FP  firFPR = FIR_FP_DEFAULTS;

FIR_FP_Handle hnd_firFPL = &firFPL;
FIR_FP_Handle hnd_firFPR = &firFPR;

//delay buffer for left and right channels
float dbufferL[FIR_ORDER+1];
float dbufferR[FIR_ORDER+1];

// Size of the coefficient
float const coeff[FIR_ORDER+1]= {
#include "coeffs.h"
};

volatile Uint32 counter;
volatile bool sampleFlag, isLeft;
volatile float32 sampleInL, sampleInR, sampleOutL, sampleOutR;

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

    // FIR Filter Initialization
    hnd_firFPL->order       = FIR_ORDER;
    hnd_firFPL->dbuffer_ptr = dbufferL;
    hnd_firFPL->coeff_ptr   = (float *)coeff;
    hnd_firFPL->init(hnd_firFPL);

    hnd_firFPR->order       = FIR_ORDER;
    hnd_firFPR->dbuffer_ptr = dbufferR;
    hnd_firFPR->coeff_ptr   = (float *)coeff;
    hnd_firFPR->init(hnd_firFPR);

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
    GpioDataRegs.GPASET.bit.GPIO2=1;
    Uint32 temp=0;
    //left channel
    if(isLeft){
        //put input in sampleInL
        temp=McbspbRegs.DRR2.all;
        temp=temp<<16;
        temp=temp|McbspbRegs.DRR1.all;
        sampleInL=(int32)temp;
        //transmit left output
        temp=(int32)sampleOutL;
        McbspbRegs.DXR2.all=(int)(temp>>16);
        McbspbRegs.DXR1.all=0;
    }
    //right channel
    else{
        //put input in sampleInR
        temp=McbspbRegs.DRR2.all;
        temp=temp<<16;
        temp=temp|McbspbRegs.DRR1.all;
        sampleInR=(int32)temp;
        //transmit right output
        temp=(int32)sampleOutR;
        McbspbRegs.DXR2.all=(int)(temp>>16);
        McbspbRegs.DXR1.all=0;
        //both channel received
        sampleFlag=1;
    }
    //toggle isLeft flag
    isLeft=!isLeft;
    //ack interrupt
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP6);
    GpioDataRegs.GPACLEAR.bit.GPIO2=1;
}

/*
 * Description:
 *      Produce output based on the IIR filter generated in MATLAB
 * Parameters:
 *      None
 * Return:
 *      None
 */
void produce_output(){
    GpioDataRegs.GPASET.bit.GPIO0=1;

    //calculate left
    hnd_firFPL->input=sampleInL;
    hnd_firFPL->calc(&firFPL);
    sampleOutL=hnd_firFPL->output;
    //calculate right
    hnd_firFPR->input=sampleInR;
    hnd_firFPR->calc(&firFPR);
    sampleOutR=hnd_firFPR->output;

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
