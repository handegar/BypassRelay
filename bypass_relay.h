/* 
 * File:   bypass_relay.h
 * Author: handegar
 *
 * Created on October 3, 2021, 7:38 PM
 */

#ifndef BYPASS_RELAY_H
#define	BYPASS_RELAY_H


// PIC12F675 Configuration Bit Settings
// CONFIG
#pragma config FOSC = INTRCIO   // Oscillator Selection bits (INTOSC oscillator: I/O function on GP4/OSC2/CLKOUT pin, I/O function on GP5/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)
#pragma config PWRTE = OFF      // Power-Up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = OFF      // GP3/MCLR pin function select (GP3/MCLR pin function is digital I/O, MCLR internally tied to VDD)
#pragma config BOREN = OFF      // Brown-out Detect Enable bit (BOD disabled)
#pragma config CP = OFF         // Code Protection bit (Program Memory code protection is disabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection is disabled)

#define _XTAL_FREQ 4000000      // Run at 4 Mhz

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <stdint.h>
#include <xc.h>

#define LATCHING 0
#define MOMENTARY 1
#define PRESSED 0
#define OPEN 1
#define OFF 0
#define ON 1
#define FALSE 0
#define TRUE 1

// Wait time before action
#define GRACE_TIME 300          
// How many GRACE_TIMEs to wait before changing mode
#define MODE_CHANGE_PERIODS 8   
// How much longer must we hold to get back from momentary?
#define MODE_CHANGE_RETURN_FACTOR 10 
// Time to mute to avoid "click" when toggling relay
#define MUTE_TIME 20
// Pause-time to filter out switch-bounce noise
#define DEBOUNCE_TIME 15
// Pause to let the PIC finish its IO changes
#define PIC_CHANGE_TIME 10
// Interval for LED blinking
#define BLINK_INTERVAL 100


// Shall we compile in logic for the option switch?
#define USE_OPTIONSWITCH 0

// What kind of switch are we using for the option-switch?
// NB: Set to 1 if USE_OPTIONSWITCH is 0 (no optionswitch used)
#define OPTIONSWITCH_IS_MOMENTARY (1 && USE_OPTIONSWITCH) // Alternative is a SPST switch


#define LED_OUT GP0
#define FOOTSWITCH_IN GP1
#define MUTE_OUT GP2
#define OPTIONSWITCH_IN GP3
#define RELAY_GND GP4 // Shall always be 0.
#define RELAY_OUT GP5

uint8_t relay_state; // Off / On
uint8_t relay_mode;  // Momentary / Latching

// To keep track of number of GRACE_TIME periods the user has pressed
// the footswitch.
uint8_t mode_change_counter; 


#ifdef	__cplusplus
extern "C" {
#endif


#ifdef	__cplusplus
}
#endif

#endif	/* BYPASS_RELAY_H */

