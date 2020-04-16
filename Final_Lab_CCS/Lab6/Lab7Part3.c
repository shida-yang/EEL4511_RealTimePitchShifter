/*
 * Name:        Shida Yang
 * Lab #:       23195
 * Description: This program creates a real-time band-pass IIR filter
 */

#include <F28x_Project.h>

#include "SPIB.h"
#include "SPIA.h"
#include "CodecGPIO.h"
#include "AIC23.h"
#include "Mcbsp.h"
#include "interrupt.h"
#include <filter.h>

//buffer mask
#define BUF_MASK 0x3FFFF

//IIR filters for left and right channels
IIR5BIQ32  iirL = IIR5BIQ32_DEFAULTS;
IIR5BIQ32  iirR = IIR5BIQ32_DEFAULTS;

//delay buffer for left and right channels
int32_t dbufferL[2*IIR32_NBIQ];
int32_t dbufferR[2*IIR32_NBIQ];

// Size of the coefficient array = 5 * nbiq
const int32_t coeff[5*IIR32_NBIQ] = IIR32_COEFF;

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

    // IIR Filter Initialization
    iirL.dbuffer_ptr = dbufferL;
    iirL.coeff_ptr   = (long *)coeff;
    iirL.qfmat       = IIR32_QFMAT;
    iirL.nbiq        = IIR32_NBIQ;
    iirL.isf         = IIR32_ISF;
    iirL.init(&iirL);

    iirR.dbuffer_ptr = dbufferR;
    iirR.coeff_ptr   = (long *)coeff;
    iirR.qfmat       = IIR32_QFMAT;
    iirR.nbiq        = IIR32_NBIQ;
    iirR.isf         = IIR32_ISF;
    iirR.init(&iirR);

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
    iirL.input=(int32)sampleInL;
    iirL.calc(&iirL);
    sampleOutL=iirL.output32;
    //calculate right
    iirR.input=(int32)sampleInR;
    iirR.calc(&iirR);
    sampleOutR=iirR.output32;

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
