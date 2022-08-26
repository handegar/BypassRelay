/*
 * Bypass-Relay logic
 *  - Inspired by the BypassRelay by Coda
 *    https://www.coda-effects.com/2017/02/relay-bypass-final-code.html
 *  - Has support for "hold" to change mode from latch to momentary (and back)
 *  - Blinks LED to indicate mode change. 
 *  - Has persistent "On as default"-mode switching (hold at switch @ powerup)
 *  - Has an optional extra input "USE_OPTIONSWITCH" which can toggle the 
 *    latch/momentary mode via an SPST to GND.
 */


#include <xc.h>
#include <eeprom_routines.h>

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
    TRISIO3 = 1; // Extra button/switch (optional, not used at the moment)
    TRISIO4 = 0; // GND pin for the relay
    TRISIO5 = 0; // Relay
    GPIO = 0; // all the GPIOs are in low state (0V) when starting    
}


void toggle_LED(uint8_t onoff) {
    LED_OUT = onoff == 1 ? ON : OFF;
}


void toggle_relay(uint8_t onoff) {
    MUTE_OUT = ON; // mute signal       
    __delay_ms(MUTE_TIME);
    toggle_LED(onoff);
    RELAY_OUT = onoff == 1 ? ON : OFF; // (de)activate the relay                 
    // The other pin for the relay. Set to 0 to ensure it is GND
    RELAY_GND = OFF; 
    __delay_ms(MUTE_TIME);
    MUTE_OUT = OFF; // unmute signal
    // Give the PIC time to update it's IO (might not be needed, though...)
    __delay_ms(PIC_CHANGE_TIME); 
}


// Blinks the LED a couple of times
void blink_LED(int times) {
    uint8_t state = OFF;
    toggle_LED(0);
    for (int i = 0; i < times; ++i) { 
        __delay_ms(BLINK_INTERVAL);
        state = !state;
        toggle_LED(state);        
    }
    if (state == ON) { // Did we end on "ON"?
        toggle_LED(0); 
    }
}


void setup(void) {
    unsigned char on_at_startup = eeprom_read(0);
        
    if (FOOTSWITCH_IN == PRESSED) {
        on_at_startup = !on_at_startup;
        // Writing to the same address each time. The EEPROM is supposed to 
        // guarantee 100k writes. This is more than enough for a lifetime of 
        // "on at startup" switching :-)
        eeprom_write(0, on_at_startup);
        __delay_ms(GRACE_TIME);           
    }
     
    blink_LED(6); // Say hello!
   
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
    
    unsigned long loopcounter = 0;    
    unsigned long mode_change_counter = 0;
    uint8_t was_pressed = FALSE;
    
    do {             
        loopcounter++;
        
#if USE_OPTIONSWITCH
        // The option-switch is a regular SPST to ground               
        uint8_t m = OPTIONSWITCH_IN == PRESSED ? MOMENTARY : LATCHING;
        if (relay_mode == MOMENTARY && m != relay_mode) {
            toggle_relay(OFF);
            __delay_ms(GRACE_TIME);
        }        
        relay_mode = m;        
#endif
                
        /*
        if (loopcounter < pauseconst) {
            continue;        
        }*/
        
        if (FOOTSWITCH_IN == PRESSED) { // Foot switch pressed      
            if (relay_mode == LATCHING) {
                if (mode_change_counter == 0) {
                    relay_state = !relay_state;            
                    toggle_relay(relay_state);    
                    __delay_ms(DEBOUNCE_TIME);                
                }
            }
            else {            
                
                if (relay_state != ON) {
                    relay_state = ON;  
                    toggle_relay(relay_state);                            
                    __delay_ms(DEBOUNCE_TIME);
                }
            }
                                 
            
#if !USE_OPTIONSWITCH
            mode_change_counter++;
            
            // Shall we enter or exit momentary-mode?
            unsigned long threshold = relay_mode == MOMENTARY ?
                MODE_CHANGE_PERIODS * 2 : MODE_CHANGE_PERIODS;
            if (mode_change_counter == threshold && was_pressed) {
                relay_mode = !relay_mode;      
                blink_LED(6);
                if (relay_mode == LATCHING) {
                    relay_state = OFF;
                    toggle_relay(relay_state);         
                    __delay_ms(DEBOUNCE_TIME);                    
                }
            }
#endif
            was_pressed = TRUE;
        }
        else { // Foot switch NOT pressed           
            mode_change_counter = 0;            
            if (relay_mode == MOMENTARY && relay_state == ON) {
                relay_state = OFF;
                toggle_relay(relay_state);                  
            }
            __delay_ms(DEBOUNCE_TIME);            
            was_pressed = FALSE;
        }
        
    } while(1); // Keep trucking!
    
}
