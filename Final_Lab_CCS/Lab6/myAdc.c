/*
 * Name:        Shida Yang
 * Lab #:       23195
 * Description: This program initializes ADC and SOC based on
 *              given parameters, and provides functions to
 *              interpret the result of ADC conversion.
 */

#include "myAdc.h"

/*
 * Description:
 *      Initialize the ADC module.
 * Parameters:
 *      uint32_t base: base of the ADC you want to init
 *      ADC_ClkPrescale clkPrescale: ACD clock prescaler
 *      ADC_Resolution resolution: ADC resolution
 *      ADC_SignalMode signalMode: ADC signal mode
 * Return:
 *      None
 */
void initAdc(uint32_t base, ADC_ClkPrescale clkPrescale, ADC_Resolution resolution,
             ADC_SignalMode signalMode){
    //set prescaler
    ADC_setPrescaler(base, clkPrescale);

    //resolution and signal mode
    ADC_setMode(base, resolution, signalMode);

    //enable ADC
    ADC_enableConverter(base);

    //delay 1ms to allow ADC to boot up
    DELAY_US(1000);
}

/*
 * Description:
 *      Initialize the SOC module for a specific ADC
 * Parameters:
 *      uint32_t base: base of the ADC
 *      ADC_SOCNumber socNumber: select which SOC you will use
 *      ADC_Trigger trigger: how to trigger the SOC
 *      ADC_Channel channel: which channel of the ADC do you want to convert
 *      uint32_t sampleWindow: sampling window of the SOC
 *      ADC_IntNumber adcIntNum: trigger which ADC interrupt when finish conversion
 * Return:
 *      None
 */
void initAdcSoc(uint32_t base, ADC_SOCNumber socNumber, ADC_Trigger trigger,
                ADC_Channel channel, uint32_t sampleWindow, ADC_IntNumber adcIntNum){
    //set up SOC
    ADC_setupSOC(base, socNumber, trigger, channel, sampleWindow);
    //set adcIntNum interrupt to be triggered by socNumber SOC
    ADC_setInterruptSource(base, adcIntNum, socNumber);
    //enable the selected ADC interrupt
    ADC_enableInterrupt(base, adcIntNum);
    //reset the interrupt flag
    ADC_clearInterruptStatus(base, adcIntNum);
}

/*
 * Description:
 *      Convert ADC digital result to floating number based on the resolution
 * Parameters:
 *      Uint16 data: digital result of ADC
 *      ADC_Resolution resolution: resolution of the ADC
 * Return:
 *      float: floating point result
 */
float convertToFloat(Uint16 data, ADC_Resolution resolution){
    //high reference voltage
    float VREFH=3.0;
    //floating point result
    float dataF=0.0;
    //12-bit resolution (single-ended)
    if(resolution==ADC_RESOLUTION_12BIT){
        //apply formula found in the manual
        dataF=(data/4096.0)*VREFH;
    }
    //16-bit resolution (differential)
    else{
        //apply formula found in the manual
        dataF=(data/65536.0)*VREFH*2-VREFH;
    }
    //return floating point result
    return dataF;
}

/*
 * Description:
 *      Convert a floating point to string
 * Parameters:
 *      float dataF: floating point number input
 *      Uint16 numOfDecimals: number of decimals to keep
 * Return:
 *      char*: string output
 */
char* convertToString(float dataF, Uint16 numOfDecimals){
    char* output;
    bool isNeg=0;
    //convert the input to positive, mark if it is negative
    if(dataF<0){
        dataF=-dataF;
        isNeg=1;
    }
    //based on the number of decimals selected, round the number
    dataF=dataF*pow(10,numOfDecimals)+0.5;
    //discard the numbers after the specified number of decimals
    Uint32 dataInt=(Uint32)dataF;
    //the input is negative
    if(isNeg){
        //allocate space for the string
        //number of decimals+negative sign+integer digit+decimal point+terminating NULL
        output=malloc(sizeof(char)*(numOfDecimals+4));
        //terminating NULL
        output[numOfDecimals+3]='\0';
        //fill in decimal part
        for(Uint16 i=numOfDecimals+2; i>=3; i--){
            //get the least significant digit
            output[i]=(char)(dataInt%10+48);
            //discard the lease significant digit
            dataInt/=10;
        }
        //fill in decimal point
        output[2]='.';
        //fill in integer digit
        output[1]=(char)(dataInt%10+48);
        //fill in the negative sign
        output[0]='-';
    }
    //the input is positive
    else{
        //number of decimals+integer digit+decimal point+terminating NULL
        output=malloc(sizeof(char)*(numOfDecimals+3));
        //terminating NULL
        output[numOfDecimals+2]='\0';
        //fill in decimal part
        for(Uint16 i=numOfDecimals+1; i>=2; i--){
            //get the least significant digit
            output[i]=(char)(dataInt%10+48);
            //discard the lease significant digit
            dataInt/=10;
        }
        //fill in decimal point
        output[1]='.';
        //fill in integer digit
        output[0]=(char)(dataInt%10+48);
    }
    return output;
}
