/*
 * Name:        Shida Yang
 * Lab #:       23195
 * Description: This program uses DIP switch to select different sampling
 *              and playback rates.
 */

#include <F28x_Project.h>

#include "Mcbsp.h"
#include "AIC23.h"
#include "interrupt.h"
#include "SPIA.h"
#include "CodecGPIO.h"

//command for different sampling rates
#define SAMPLING_48KHz  ((8<<9)|0)
#define SAMPLING_32KHz  ((8<<9)|(12<<1))
#define SAMPLING_8KHz   ((8<<9)|(6<<1))


__interrupt void McbspbISR();

volatile bool isLeft, sampleFlag;
volatile Uint16 curr_sampling_rate;
volatile float32 sampleInL, sampleInR, sampleOutL, sampleOutR;

int main(){

    //init system clocks and get board speed running at 200 MHz
    InitSysCtrl();

    EALLOW; //enable protective reg write

    //disable all interrupt
    Interrupt_initModule();
    //init interrupt table
    Interrupt_initVectorTable();

    //init GPIO PB,SW,LED
    initGpio();
    //init McBSPB as I2S
    initMcbspbI2S();
    //init SPIA
    InitSPIA();
    //init AIC23 board
    InitAIC23_SR(SR48);

    //first frame is left
    isLeft=0;
    //default sampling rate is 48kHz
    curr_sampling_rate=SAMPLING_48KHz;
    //reset sampleFlag
    sampleFlag=0;
    //initialize sampleIn/sampleOut
    sampleInL=0;
    sampleOutL=0;
    sampleInR=0;
    sampleOutR=0;

    //point McBSPB RX INT to my ISR
    Interrupt_register(INT_MCBSPB_RX, &McbspbISR);
    //enable McBSPB RX INT
    Interrupt_enable(INT_MCBSPB_RX);
    //enable global interrupt
    Interrupt_enableMaster();

    //SW0=8kHz
    //SW1=32kHz
    //SW2=48kHz
    while(1){
        //if both channel received
        if(sampleFlag){
            //output channel left only
            sampleOutL=sampleInL;
            sampleOutR=0;
            //reset sample flag
            sampleFlag=0;
        }
        //get current switch value
        Uint16 sw_value=readSwitch()&7;
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
    //toggle a GPIO (GPIO0)
    writeToLED((Uint16)isLeft);
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
