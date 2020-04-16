/*
 * Name:        Shida Yang
 * Lab #:       23195
 * Description: This program creates a real time 256-point DFT with DMA
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

volatile Uint32 counter;
volatile bool sampleFlag, isLeft, calcFlag, bufferSwitch;
volatile float32 binRes=((float)Fs)/DFT_length;
volatile float32 peakFreq, lastFreq;
volatile float32 peakdB, lastdB;

//DFT input buffers
#pragma DATA_SECTION(buffer0, "INBUF0");
#pragma DATA_SECTION(buffer1, "INBUF0");
volatile int32 buffer0[DFT_length*2];
volatile int32 buffer1[DFT_length*2];
volatile float DFTOutput[DFT_length/2];

__interrupt void McbspbISR();
__interrupt void dmaCh1ISR(void);
__interrupt void dmaCh2ISR(void);
void calcDFT();
//void outputToLCD();
void findPeak();
void initDMA();
void initDMA2();

int main(){

    //init system clocks and get board speed running at 200 MHz
    InitSysCtrl();
    EALLOW;
    //init DMA
    initDMA();
    initDMA2();

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
    //initialize sampleFlag
    sampleFlag=0;
    isLeft=0;
    calcFlag=0;
    bufferSwitch=0; //0=buffer0, 1=buffer1

    //point INT to my ISR
    Interrupt_register(INT_DMA_CH1, &dmaCh1ISR);
    //enable INT
    Interrupt_enable(INT_DMA_CH1);
    //enable global interrupt
    Interrupt_enableMaster();

    while(1){
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
                //outputToLCD();
            }
        }
    }

}

/*
 * Description:
 *      ISR for DMA, switch buffer and set calc flag
 * Parameters:
 *      None
 * Return:
 *      None
 */
__interrupt void dmaCh1ISR(void){
    GpioDataRegs.GPATOGGLE.bit.GPIO1=1;
    //switch dest buffer
    volatile int16* src_addr1=&McbspbRegs.DRR2.all;
    volatile int16* dest_addr1=0;

    volatile int16* src_addr2=0;
    volatile int16* dest_addr2=&McbspbRegs.DXR2.all;

    //current is buffer1
    if(bufferSwitch){
        dest_addr1=(int16*)(&buffer0[0])+1;
        src_addr2=(int16*)(&buffer0[0])+1;
    }
    //current is buffer0
    else{
        dest_addr1=(int16*)(&buffer1[0])+1;
        src_addr2=(int16*)(&buffer1[0])+1;
    }
    bufferSwitch=!bufferSwitch;
    //update DMA src/dest addr config
    DMACH1AddrConfig(dest_addr1, src_addr1);
    DMACH2AddrConfig(dest_addr2, src_addr2);

    //set calc flag
    calcFlag=1;

    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP7);
    StartDMACH1();
    StartDMACH2();
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
    //GpioDataRegs.GPASET.bit.GPIO0=1;
    float real=0;
    float img=0;
    //buffer0 is full b/c DMA ISR toggles switch at the end
    if(bufferSwitch){
        //outer loop for each DFT point
        for(int k=0; k<DFT_length/2; k++){
            //inner loop for DFT sum
            //skip every other point to take only one channel
            for(int n=0; n<DFT_length*2; n+=2){
                //real part is cos
                real+=buffer0[n]*cosf(-2*PI*k*n/2/DFT_length);
                //img part is sin
                img+=buffer0[n]*sinf(-2*PI*k*n/2/DFT_length);
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
            for(int n=0; n<DFT_length*2; n+=2){
                real+=buffer1[n]*cosf(-2*PI*k*n/2/DFT_length);
                img+=buffer1[n]*sinf(-2*PI*k*n/2/DFT_length);
            }
            DFTOutput[k]=sqrt(real*real+img*img);
            real=0;
            img=0;
        }
    }
    //StartDMACH1();
    GpioDataRegs.GPACLEAR.bit.GPIO0=1;
    //GpioDataRegs.GPACLEAR.bit.GPIO0=1;
}

/*
 * Description:
 *      output peak freq and mag to LCD
 * Parameters:
 *      None
 * Return:
 *      None
 */
/*
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
*/

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

/*
 * Description:
 *      initialize DMA to transfer data from DRR to buffer0
 *      with little endian format
 * Parameters:
 *      None
 * Return:
 *      None
 */
void initDMA(){
    //get src/dest addr
    volatile int16* src_addr=&McbspbRegs.DRR2.all;
    volatile int16* dest_addr=(int16*)(&buffer0[0])+1;

    //each burst (one 32-bit McbspB sample) has 2 words
    Uint16 burst_size=2;
    int16 src_burst_step=1;     //DRR2->DRR1
    int16 dest_burst_step=-1;   //little endian

    //each transfer has DFT_length*2 bursts (left and right)
    Uint16 trans_size=DFT_length*2;
    int16 src_trans_step=-1;    //DRR1->DRR2
    int16 dest_trans_step=3;    //move to high word of next data

    //disable addr wrapping
    Uint16 src_wrap_size=0xFFFF;
    int16 src_wrap_step=0;
    Uint16 dest_wrap_size=0xFFFF;
    int16 dest_wrap_step=0;

    //trigger source=McbspB receive
    Uint16 per_sel=74;

    //reset DMA
    DMAInitialize();
    //set the src and dest addr
    DMACH1AddrConfig(dest_addr, src_addr);
    //configure burst
    DMACH1BurstConfig(burst_size-1, src_burst_step, dest_burst_step);
    //configure transfer
    DMACH1TransferConfig(trans_size-1, src_trans_step, dest_trans_step);
    //configure wrap
    DMACH1WrapConfig(src_wrap_size, src_wrap_step, dest_wrap_size, dest_wrap_step);
    //configure mode
    DMACH1ModeConfig(
            per_sel,
            PERINT_ENABLE,
            ONESHOT_DISABLE,
            CONT_DISABLE,
            SYNC_DISABLE,
            SYNC_SRC,
            OVRFLOW_DISABLE,
            SIXTEEN_BIT,
            CHINT_END,
            CHINT_ENABLE
    );
    EALLOW;
    CpuSysRegs.SECMSEL.bit.PF2SEL = 1;
    EDIS;
    //start DMA
    StartDMACH1();
}

void initDMA2(){
    //get src/dest addr
    volatile int16* src_addr=(int16*)(&buffer0[0])+1;
    volatile int16* dest_addr=&McbspbRegs.DXR2.all;

    //each burst (one 32-bit McbspB sample) has 2 words
    Uint16 burst_size=2;
    int16 src_burst_step=-1;     //DRR2->DRR1
    int16 dest_burst_step=1;   //little endian

    //each transfer has DFT_length*2 bursts (left and right)
    Uint16 trans_size=DFT_length*2;
    int16 src_trans_step=3;    //DRR1->DRR2
    int16 dest_trans_step=-1;    //move to high word of next data

    //disable addr wrapping
    Uint16 src_wrap_size=0xFFFF;
    int16 src_wrap_step=0;
    Uint16 dest_wrap_size=0xFFFF;
    int16 dest_wrap_step=0;

    //trigger source=McbspB receive
    Uint16 per_sel=74;

    //reset DMA
    //set the src and dest addr
    DMACH2AddrConfig(dest_addr, src_addr);
    //configure burst
    DMACH2BurstConfig(burst_size-1, src_burst_step, dest_burst_step);
    //configure transfer
    DMACH2TransferConfig(trans_size-1, src_trans_step, dest_trans_step);
    //configure wrap
    DMACH2WrapConfig(src_wrap_size, src_wrap_step, dest_wrap_size, dest_wrap_step);
    //configure mode
    DMACH2ModeConfig(
            per_sel,
            PERINT_ENABLE,
            ONESHOT_DISABLE,
            CONT_DISABLE,
            SYNC_DISABLE,
            SYNC_SRC,
            OVRFLOW_DISABLE,
            SIXTEEN_BIT,
            CHINT_END,
            CHINT_ENABLE
    );
    //start DMA
    StartDMACH2();
}
