/*
 * Bypass-Relay logic
 *  - Inspired by the BypassRelay by Coda
 *    https://www.coda-effects.com/2017/02/relay-bypass-final-code.html
 *  - Has support for "hold" to change mode from latch to momentary (and back)
 *  - Blinks LED to indicate mode change. 
 *  - Has persistent "On as default"-mode switching (hold at switch @ power-up)
 *  - Has an optional extra input "USE_OPTIONSWITCH" which can toggle the 
 *    latch/momentary mode via an SPST to GND.
 */

/* 
 * MCU Pin usage:
 * 
 *            +---_---+
 *       5V --| 1   8 |-- GND
 *   Relay+ --| 2   7 |-- LED (via clr)
 *   Relay- --| 3   6 |-- Foot switch (to ground)
 *   Opt sw --| 4   5 |-- Opto-coupler via 1k5 ohm. 
 *            +-------+ 
 * 
 */

/*
 * Alternative scheme (From "Flexi-Switching")
 * - https://www.guitarscanada.com/threads/flexi-switch.226962/
 */


#include <xc.h>
#include <htc.h> // Eeprom read/write

#include "bypass_relay.h"

void init() {
    relay_state = OFF; // Default "Off"
    relay_mode = LATCHING; // Default "latching"    

    ANSEL = 0; // no analog GPIO
    CMCON = 0x07; // comparator off 
    ADCON0 = 0; // ADC and DAC converters off

    // Setup IO. 1=input, 0=output 
    TRISIO0 = 0; // LED
    TRISIO1 = 1; // Foot-switch
    TRISIO2 = 0; // TLP222G/A muter
    
#if !USE_OPTIONSWITCH
    TRISIO3 = 0; // Set as output as it is not used
#else
    TRISIO3 = 1;     
#endif
    
    TRISIO4 = 0; // GND pin for the relay
    TRISIO5 = 0; // Relay
    GPIO = 0; // all the GPIOs are in low state (0V) when starting    
    
    // The other pin for the relay. Set to 0 to ensure it is GND
    RELAY_GND = OFF;   
    
#if 1
    // Enable weak (internal) pull-up for the foot-switch pin.
    // NOTE: There is no WPU3 for the optional switch so here we'll
    // have to rely on an external pull-up resistor.
    OPTION_REG = 0x00;
    WPU = 0b00000010;
#endif
    
    __delay_ms(GRACE_TIME);        
}


void toggle_LED(uint8_t onoff) {
    LED_OUT = onoff == 1 ? ON : OFF;
}


void toggle_mute(uint8_t onoff) {
#if MUTE_BEFORE_SWITCH
    MUTE_OUT = onoff == 1 ? ON : OFF; // mute signal (TLP222A has a t-on of 0.8 milliseconds)      
    __delay_ms(MUTE_TIME);
#endif    
}


void toggle_relay(uint8_t onoff) {         
#if MUTE_MODE_TEST    
    toggle_mute(onoff == 1 ? TRUE : FALSE);    
    toggle_LED(onoff);    
#else   
    toggle_mute(TRUE);
    toggle_LED(onoff);
    RELAY_OUT = onoff == 1 ? ON : OFF; // (de)activate the relay                     
    __delay_ms(RELAY_ACTION_TIME);    
    toggle_mute(FALSE);
#endif
}


// Blinks the LED a couple of times
void blink_LED(int times) {
    toggle_mute(TRUE);
    
    for (int i = 0; i < times*2; ++i) {                 
        toggle_LED(ON);        
        __delay_ms(BLINK_INTERVAL);
        toggle_LED(OFF);        
        __delay_ms(BLINK_INTERVAL);
    }
    
    toggle_mute(FALSE);
}


void setup(void) {
    unsigned char on_at_startup = EEPROM_READ(ON_AT_STARTUP_ADDR);
    if (FOOTSWITCH_IN == PRESSED) {
        on_at_startup = on_at_startup > 0 ? FALSE : TRUE;
        // Writing to the same address each time. The EEPROM is supposed to 
        // guarantee 100k writes. This is more than enough for a lifetime of 
        // "on at startup" switching :-)
        EEPROM_WRITE(ON_AT_STARTUP_ADDR, on_at_startup);
        __delay_ms(GRACE_TIME);           
    }
         
    blink_LED(2); // Say hello!
   
    if (on_at_startup == TRUE) {        
        relay_state = ON;
    }
    else {        
        relay_state = OFF;
    }
    
    toggle_relay(relay_state);
    __delay_ms(GRACE_TIME);   
    
}

void main(void) {
    init();
    setup();
        
    unsigned long mode_change_counter = 0;
    
    // Main loop
    do {                              
#if USE_OPTIONSWITCH
        // The option-switch is a regular SPST to ground               
        uint8_t m = OPTIONSWITCH_IN == PRESSED ? MOMENTARY : LATCHING;
        if (relay_mode == MOMENTARY && m != relay_mode) {
            toggle_relay(OFF);
            __delay_ms(GRACE_TIME);
        }        
        relay_mode = m;        
#endif
                        
        if (FOOTSWITCH_IN == PRESSED) { // Foot switch pressed     
            if (relay_mode == LATCHING) {
                if (mode_change_counter == 0) {                    
                    relay_state = !relay_state;                                
                    toggle_relay(relay_state);
                    __delay_ms(GRACE_TIME*2); // Wait a while before we register a new switch state.                                                                                  
                }
            }
            else { // Momentary mode                           
                if (relay_state != ON) {
                    relay_state = ON;  
                    toggle_relay(relay_state);
                    __delay_ms(GRACE_TIME);        
                }
            }
                                                        
#if !USE_OPTIONSWITCH 
            // We are not using an external option-switch. A long-press will
            // therefore toggle the momentary-mode on/off.
            mode_change_counter++;
            
            // Shall we enter or exit momentary-mode?
            unsigned long threshold = relay_mode == MOMENTARY ?
                MODE_CHANGE_PERIODS * 2 : MODE_CHANGE_PERIODS;
            if (mode_change_counter >= threshold) {
                relay_mode = !relay_mode;      
                blink_LED(3);
                if (relay_mode == LATCHING) { // Going to latching? Toggle off.
                    relay_state = OFF;
                    toggle_relay(relay_state);  
                    mode_change_counter = 0;                       
                }
            }
#endif            
        }
        else { // Foot switch NOT pressed           
            mode_change_counter = 0;            
            if (relay_mode == MOMENTARY && relay_state == ON) {
                relay_state = OFF;
                toggle_relay(relay_state);                  
            }                            
        }
        
    } while(1); // Keep trucking (forever)!
    
}
