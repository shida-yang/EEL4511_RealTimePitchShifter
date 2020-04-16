/*
 * Name:        Shida Yang
 * Lab #:       23195
 * Description: This program creates a real time 256-point DFT
 */

#include <F28x_Project.h>

#include "SPIB.h"
#include "SPIA.h"
#include "CodecGPIO.h"
#include "AIC23.h"
#include "Mcbsp.h"
#include "interrupt.h"
#include "I2CLCD.h"
#include "math.h"

//buffer mask
#define BUF_MASK 0xFF
//length of DFT
#define DFT_length 256
#define PI 3.1415926
#define Fs 48000
#define error 0.000001

volatile Uint32 counter;
volatile bool sampleFlag, isLeft, calcFlag, bufferSwitch;
volatile float32 sampleInL, sampleInR, sampleOutL, sampleOutR;
volatile Uint16 bufferCounter=0;
volatile float32 binRes=((float)Fs)/DFT_length;
volatile float32 peakFreq, lastFreq;
volatile float32 peakdB, lastdB;

//DFT input buffers
volatile float buffer0L[DFT_length];
volatile float buffer0R[DFT_length];
volatile float buffer1L[DFT_length];
volatile float buffer1R[DFT_length];
volatile float DFTOutput[DFT_length/2];

//DFT input buffer pointers
volatile Uint16 buffer0_ptr=0;
volatile Uint16 buffer1_ptr=0;

__interrupt void McbspbISR();
void calcDFT();
void storeData();
void outputToLCD();
void findPeak();

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
    //initialize LCD
    initLCD();

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
    calcFlag=0;
    bufferSwitch=0; //0=buffer0, 1=buffer1

    //point INT to my ISR
    Interrupt_register(INT_MCBSPB_RX, &McbspbISR);
    //enable INT
    Interrupt_enable(INT_MCBSPB_RX);
    //enable global interrupt
    Interrupt_enableMaster();

    while(1){
        //both channels sampled
        if(sampleFlag){
            //don't output sound
            sampleOutL=0;
            sampleOutR=0;
            //reset sample flag
            sampleFlag=0;
        }
        //256 samples received
        if(calcFlag==1){
            //GpioDataRegs.GPASET.bit.GPIO0=1;
            //calculate DFT
            calcDFT();
            calcFlag=0;
            //find peak
            findPeak();
            //GpioDataRegs.GPACLEAR.bit.GPIO0=1;
            //if peakdB or peakFreq changes, update LCD
            if(abs(peakdB-lastdB)>1 || abs(peakFreq-lastFreq)>1){
                lastdB=peakdB;
                lastFreq=peakFreq;
                outputToLCD();
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
        //store left and right channel results
        storeData();
    }
    //toggle isLeft flag
    isLeft=!isLeft;
    //ack interrupt
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP6);
}

/*
 * Description:
 *      calculate DFT based on the input buffer and store result
 *      in the output buffer
 * Parameters:
 *      None
 * Return:
 *      None
 */
void calcDFT(){
    GpioDataRegs.GPASET.bit.GPIO0=1;
    //set real and img part to 0
    float real=0;
    float img=0;
    //buffer0 is full b/c storeData toggles switch at the end
    if(bufferSwitch){
        //outer loop for each DFT point
        for(int k=0; k<DFT_length/2; k++){
            //inner loop for DFT sum
            for(int n=0; n<DFT_length; n++){
                //real part is cos
                real+=buffer0L[n]*cosf(-2*PI*k*n/DFT_length);
                //img part is sin
                img+=buffer0L[n]*sinf(-2*PI*k*n/DFT_length);
            }
            //calculate magnitude
            DFTOutput[k]=sqrt(real*real+img*img);
            //reset real and img
            real=0;
            img=0;
        }
    }
    //buffer1 is full
    //do same thing for buffer 1
    else{
        for(int k=0; k<DFT_length/2; k++){
            for(int n=0; n<DFT_length; n++){
                real+=buffer1L[n]*cosf(-2*PI*k*n/DFT_length);
                img+=buffer1L[n]*sinf(-2*PI*k*n/DFT_length);
            }
            DFTOutput[k]=sqrt(real*real+img*img);
            real=0;
            img=0;
        }
    }
    GpioDataRegs.GPACLEAR.bit.GPIO0=1;
}

/*
 * Description:
 *      store McbspB data in appropriate buffer
 * Parameters:
 *      None
 * Return:
 *      None
 */
void storeData(){
    //buffer1
    if(bufferSwitch){
        //store left and right data in different buffer
        buffer1L[buffer1_ptr]=sampleInL;
        buffer1R[buffer1_ptr]=sampleInR;
        //increment buffer pointer
        buffer1_ptr++;
        //buffer full, reset buffer pointer, set cala flag
        if(buffer1_ptr==DFT_length){
            buffer1_ptr=0;
            calcFlag=1;
            //switch buffer
            bufferSwitch=!bufferSwitch;
        }
    }
    //buffer0
    //do the same thing for buffer 0
    else{
        buffer0L[buffer0_ptr]=sampleInL;
        buffer0R[buffer0_ptr]=sampleInR;
        buffer0_ptr++;
        if(buffer0_ptr==DFT_length){
            buffer0_ptr=0;
            calcFlag=1;
            bufferSwitch=!bufferSwitch;
            //GpioDataRegs.GPATOGGLE.bit.GPIO2=1;
        }
    }
}

/*
 * Description:
 *      output peak freq and mag to LCD
 * Parameters:
 *      None
 * Return:
 *      None
 */
void outputToLCD(){
    //clear screen
    clearScreen();
    //dB and Freq string buffer
    char dbStr[5], freqStr[5];
    int db=peakdB;
    int freq=peakFreq;
    //convert dB and Freq to string
    for(int i=0; i<4; i++){
        dbStr[i]=(char)((db/((int)pow(10,3-i)))+48);
        freqStr[i]=(char)((freq/((int)pow(10,3-i)))+48);
        db%=(int)pow(10,3-i);
        freq%=(int)pow(10,3-i);
    }
    dbStr[4]='\0';
    freqStr[4]='\0';
    //output result
    sendString("Max Freq=");
    sendString(&freqStr[0]);
    sendString("Hz");
    switchLine();
    sendString("Max Mag=");
    sendString(&dbStr[0]);
    sendString("dBs");
}

/*
 * Description:
 *      find peak dB and Freq in DFT output
 * Parameters:
 *      None
 * Return:
 *      None
 */
void findPeak(){
    float maxFreqIndex=0;
    float maxGain=DFTOutput[0];
    //iterate through the DFT output
    for(int i=1; i<DFT_length/2; i++){
        //update max mag and freq if current mag>max mag
        if(DFTOutput[i]>maxGain){
            maxGain=DFTOutput[i];
            maxFreqIndex=i;
        }
    }
    //calculate freq based on binRes and index
    peakFreq=binRes*maxFreqIndex;
    //calculate dB gain
    peakdB=10*log(maxGain)/log(10);
}
