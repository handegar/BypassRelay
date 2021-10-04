/*
 * Bypass-Relay logic
 *  - Inspired by the BypassRelay by Coda
 *    https://www.coda-effects.com/2017/02/relay-bypass-final-code.html
 *  - Has support for "hold" to change mode from latch to momentary (and back)
 *  - Blinks LED to indicate mode change. 
 */


#include <xc.h>
#include "bypass_relay.h"

void init() {
    relay_state = OFF; // Default "Off"
    relay_mode = LATCHING; // Default "latching"
    mode_change_counter = MODE_CHANGE_PERIODS;

    ANSEL = 0; // no analog GPIO
    CMCON = 0x07; // comparator off 
    ADCON0 = 0; // ADC and DAC converters off

    // Setup IO. 1=input, 2=output
    TRISIO0 = 0; // LED
    TRISIO1 = 1; // Footswitch
    TRISIO2 = 0; // TGP222A muter
    TRISIO3 = 1; // Extra button/switch (optional, not used at the moment)
    TRISIO4 = 0; // Not in use, optional switch
    TRISIO5 = 0; // Relay
    GPIO = 0; // all the GPIOs are in low state (0V) when starting    
}

void toggle_LED(uint8_t onoff) {
    LED_OUT = onoff == 1 ? ON : OFF;
}

void toggle_relay(uint8_t onoff) {
    MUTE_OUT = ON; // photoFET on -> mute signal    
    RELAY_OUT = onoff == 1 ? ON : OFF; // (de)activate the relay                 
    // The other pin for the relay. Set to 0 to ensure it is GND
    RELAY_GND = OFF;
    __delay_ms(MUTE_TIME);
    MUTE_OUT = OFF; // photoFET off -> unmute    
}

// Blinks the LED a couple of times

void indicate_mode_switch() {
    uint8_t state = 0;
    toggle_LED(0);
    for (int i = 0; i < 6; ++i) {
        __delay_ms(20);
        state ^= 1;
        toggle_LED(state);
        __delay_ms(70);
    }
}

void main(void) {
    init();
    indicate_mode_switch(); // Say hello!

    while (1) {

#if USE_OPTIONSWITCH
        // =================================================================
        // This part is optional and can be removed if no "optional switch" 
        // is used or hooked up.
#if OPTIONSWITCH_IS_MOMENTARY
        if (OPTIONSWITCH_IN == PRESSED) { // Mode switch pressed?
            __delay_ms(DEBOUNCE_TIME);
            if (OPTIONSWITCH_IN == PRESSED) { // Still pressed?
                __delay_ms(GRACE_TIME);
                relay_mode = relay_mode == MOMENTARY ? LATCHING : MOMENTARY;
                mode_change_counter = relay_mode == MOMENTARY ?
                        MODE_CHANGE_PERIODS * MODE_CHANGE_RETURN_FACTOR :
                        MODE_CHANGE_PERIODS;
                indicate_mode_switch();
            }
        }
#else           
        if (OPTIONSWITCH_IN == PRESSED && relay_mode != MOMENTARY) {
            __delay_ms(GRACE_TIME);
            relay_mode = MOMENTARY;
            mode_change_counter =
                    MODE_CHANGE_PERIODS * MODE_CHANGE_RETURN_FACTOR;
            indicate_mode_switch();
        } else if (OPTIONSWITCH_IN == OPEN && relay_mode != LATCHING) {
            __delay_ms(GRACE_TIME);
            relay_mode = LATCHING;
            mode_change_counter = MODE_CHANGE_PERIODS;
            indicate_mode_switch();
        }
#endif
        // =================================================================
#endif

        if (FOOTSWITCH_IN == PRESSED) { // Switch pressed
            __delay_ms(DEBOUNCE_TIME); // debounce pause

            if (relay_mode == LATCHING) { // Latching mode
                if (FOOTSWITCH_IN == PRESSED) { // Switch STILL pressed?
                    __delay_ms(GRACE_TIME);
                    if (FOOTSWITCH_IN == OPEN) { // User has lifted foot. Activate/deactivate.
                        relay_state ^= 1;
                        toggle_LED(relay_state);
                        toggle_relay(relay_state);                        
                        __delay_ms(PIC_CHANGE_TIME);
                    } else { // Is the user trying to change mode?
#if OPTIONSWITCH_IS_MOMENTARY
                        mode_change_counter -= 1;
                        if (mode_change_counter == 0) {
                            relay_mode = MOMENTARY;
                            mode_change_counter = MODE_CHANGE_PERIODS;

                            relay_state = OFF;
                            toggle_LED(relay_state);
                            toggle_relay(relay_state);                            
                            __delay_ms(PIC_CHANGE_TIME);
                            indicate_mode_switch();
                        }
#endif
                    }
                }
            }

            if (relay_mode == MOMENTARY) { // Momentary mode                
                if (FOOTSWITCH_IN == PRESSED && relay_state != ON) {
                    relay_state = ON;
                    toggle_LED(relay_state);
                    toggle_relay(relay_state);                   
                    __delay_ms(PIC_CHANGE_TIME);
                }

#if OPTIONSWITCH_IS_MOMENTARY
                if (FOOTSWITCH_IN == PRESSED) { // User is pressing
                    __delay_ms(GRACE_TIME / 5);
                    mode_change_counter -= 1;
                    if (mode_change_counter == 0) {
                        relay_mode = LATCHING;
                        mode_change_counter = MODE_CHANGE_PERIODS;
                        relay_state = OFF;
                        toggle_LED(relay_state);
                        toggle_relay(relay_state);                        
                        __delay_ms(PIC_CHANGE_TIME);
                        indicate_mode_switch();
                    }
                } else {
                    mode_change_counter =
                            MODE_CHANGE_PERIODS * MODE_CHANGE_RETURN_FACTOR;
                }
#endif
            }
        }


        if (relay_mode == MOMENTARY) { // Momentary mode            
            if (FOOTSWITCH_IN == OPEN && relay_state != OFF) {
                relay_state = OFF;
                toggle_LED(relay_state);
                toggle_relay(relay_state);                
                __delay_ms(PIC_CHANGE_TIME);
            }
        }

    } // WHILE(1)

    return;
}
