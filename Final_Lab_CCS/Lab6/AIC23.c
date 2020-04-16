// TI File $Revision:  $
// Checkin $Date:  $
//###########################################################################
//
// FILE:	AIC23.c
//
// TITLE:	TLV320AIC23B Driver Functions
//
// DESCRIPTION: 
//
// This file includes functions for initializing the AIC23 codec. You may
// modify/add to these functions to suit your application if desired.
//
//###########################################################################
// $TI Release:   $
// $Release Date:   $
//###########################################################################


#include "AIC23.h"

Uint16 softpowerdown (void) {          // Power down control 
    return (POWDCTL|(AIC23OUT|AIC23DAC|AIC23ADC|  // Power down: In/Out/DAC/ADC/Mic 
                       AIC23MIC|AIC23LINE));       // Leave AIC23 device, osc, clks on
}

Uint16 fullpowerup (void) {            // Power up all modules
    return (POWDCTL|0x0000);   
}    
Uint16 nomicpowerup (void) {           // Power up all modules except microphone
    return (POWDCTL|AIC23MIC);
}    
Uint16 linput_volctl (Uint16 volume) { // Left Line Input Volume Control 
    return (LRS|LLINVCTL|volume);
}

Uint16 rinput_volctl (Uint16 volume) { // Right Line Input Volume Control 
    return (RLS|RLINVCTL|volume);
}

Uint16 lhp_volctl (Uint16 volume) {    // Left Headphone Volume Control
    //return (LRS|LHVCTL|LZC|volume);
    return (LHVCTL|LZC|volume);
}

Uint16 rhp_volctl (Uint16 volume) {    // Right Headphone Volume Control
    //return (RLS|RHVCTL|RZC|volume);
    return (RHVCTL|RZC|volume);
}

Uint16 nomicaaudpath (void) {          // Analog audio path control
    return 	(AAUDCTL|DAC|MICMUTE);     // Turn on DAC, mute mic
}

Uint16 aaudpath (void){
    return (AAUDCTL|DAC|MICINSEL|MICBOOST); // Turn on DAC, mic
}

Uint16 digaudiopath (void) {           // Digital audio path control
//    return (DAUDCTL|0x0006);           // DAC mute,  ADC HP filter all disabled, de-emph,= 48K
    return (DAUDCTL|0x0000);           // DAC mute,  de-emph ADC HP filter all disabled

}

Uint16 DSPdigaudinterface (void) {     // Digital audio interface format
    return (DAUDINTF|(AIC23MS|         // AIC23 master mode, DSP mode,32-bit data,
            FORDSP|IWL32B|LRP));         // LRP=1
}

Uint16 I2Sdigaudinterface (void) {     // Digital audio interface format
    return (DAUDINTF|(AIC23MS|         // AIC23 master mode, I2S mode,32-bit data,
            FORI2S|IWL32B|LRPZRO));         // LRP=0 right channel high
}


Uint16 USBsampleratecontrol (Uint16 ratecontrol)  {
    return (SMPLRCTL|(USB|ratecontrol)); // USB clock - 48 kHz sample rate in USB mode
}

Uint16 CLKsampleratecontrol (Uint16 ratecontrol)  {
    return (SMPLRCTL|ratecontrol);      // external clock - 48 kHz sample rate in external clock (12.288 MHz) mode
}


Uint16 digact (void) {                 // Digital Interface Activation       
    return (DIGINTFACT|ACT);           // Activate
} 

Uint16 reset (void) {                  // Reset AIC23
    return (RESETREG|RES);
}

/*
 * Description:
 *      Send commands to AIC23 through SPIA to initialize it
 * Parameters:
 *      None
 * Return:
 *      None
 */
void InitAIC23(){
    SmallDelay();
    uint16_t command;
    command = reset();
    SpiaTransmit (command);
    SmallDelay();
    command = softpowerdown();       // Power down everything except device and clocks
    SpiaTransmit (command);
    SmallDelay();
    command = linput_volctl(LIV);    // Unmute left line input and maintain default volume
    SpiaTransmit (command);
    SmallDelay();
    command = rinput_volctl(RIV);    // Unmute right line input and maintain default volume
    SpiaTransmit (command);
    SmallDelay();
    command = lhp_volctl(LHV);       // Left headphone volume control
    SpiaTransmit (command);
    SmallDelay();
    command = rhp_volctl(RHV);       // Right headphone volume control
    SpiaTransmit (command);
    SmallDelay();
    command = nomicaaudpath();      // Turn on DAC, mute mic
    SpiaTransmit (command);
    SmallDelay();
    command = digaudiopath();       // Disable DAC mute, add de-emph
    SpiaTransmit (command);
    SmallDelay();

    // I2S
    command = I2Sdigaudinterface(); // AIC23 master mode, I2S mode,32-bit data, LRP=1 to match with XDATADLY=1
    SpiaTransmit (command);
    SmallDelay();
    command = CLKsampleratecontrol (SR48);
    SpiaTransmit (command);
    SmallDelay();

    command = digact();             // Activate digital interface
    SpiaTransmit (command);
    SmallDelay();
    command = nomicpowerup();      // Turn everything on except Mic.
    SpiaTransmit (command);
}

void InitAIC23_SR(Uint16 SR){
    SmallDelay();
    uint16_t command;
    command = reset();
    SpiaTransmit (command);
    SmallDelay();
    command = softpowerdown();       // Power down everything except device and clocks
    SpiaTransmit (command);
    SmallDelay();
    command = linput_volctl(LIV);    // Unmute left line input and maintain default volume
    SpiaTransmit (command);
    SmallDelay();
    command = rinput_volctl(RIV);    // Unmute right line input and maintain default volume
    SpiaTransmit (command);
    SmallDelay();
    command = lhp_volctl(LHV);       // Left headphone volume control
    SpiaTransmit (command);
    SmallDelay();
    command = rhp_volctl(RHV);       // Right headphone volume control
    SpiaTransmit (command);
    SmallDelay();
    command = nomicaaudpath();      // Turn on DAC, mute mic
    SpiaTransmit (command);
    SmallDelay();
    command = digaudiopath();       // Disable DAC mute, add de-emph
    SpiaTransmit (command);
    SmallDelay();

    // I2S
    command = I2Sdigaudinterface(); // AIC23 master mode, I2S mode,32-bit data, LRP=1 to match with XDATADLY=1
    SpiaTransmit (command);
    SmallDelay();
    command = CLKsampleratecontrol (SR);
    SpiaTransmit (command);
    SmallDelay();

    command = digact();             // Activate digital interface
    SpiaTransmit (command);
    SmallDelay();
    command = nomicpowerup();      // Turn everything on except Mic.
    SpiaTransmit (command);
}
