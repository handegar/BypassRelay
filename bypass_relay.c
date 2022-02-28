/*
 * Bypass-Relay logic
 *  - Inspired by the BypassRelay by Coda
 *    https://www.coda-effects.com/2017/02/relay-bypass-final-code.html
 *  - Has support for "hold" to change mode from latch to momentary (and back)
 *  - Blinks LED to indicate mode change. 
 */

/*
 * TODO:
 *   - Store "Auto-on" preferences (hold footswitch on startup to toggle)
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

    // Setup IO. 1=input, 0=output (? double check this)
    TRISIO0 = 0; // LED
    TRISIO1 = 1; // Footswitch
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
    uint8_t state = 0;
    toggle_LED(0);
    for (int i = 0; i < times; ++i) { 
        __delay_ms(BLINK_INTERVAL);
        state ^= 1;
        toggle_LED(state);        
    }
    if (state == ON) {
        toggle_LED(0); 
    }
}


void main(void) {
    init();
    blink_LED(6); // Say hello!

    do { 

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
                blink_LED(6);
            }
        }
#else           
        if (OPTIONSWITCH_IN == PRESSED && relay_mode != MOMENTARY) {
            __delay_ms(GRACE_TIME);
            relay_mode = MOMENTARY;
            mode_change_counter =
                    MODE_CHANGE_PERIODS * MODE_CHANGE_RETURN_FACTOR;
            blink_LED(6);
        } else if (OPTIONSWITCH_IN == OPEN && relay_mode != LATCHING) {
            __delay_ms(GRACE_TIME);
            relay_mode = LATCHING;
            mode_change_counter = MODE_CHANGE_PERIODS;
            blink_LED(6);
        }
#endif
        // =================================================================
#endif

        if (FOOTSWITCH_IN == PRESSED) { // Switch pressed
            __delay_ms(DEBOUNCE_TIME); // debounce pause

            // == Latching mode ===========================================
            if (relay_mode == LATCHING) { 
                if (FOOTSWITCH_IN == PRESSED) { // Switch STILL pressed?
                    __delay_ms(GRACE_TIME);
                    // User has lifted foot. Activate/deactivate.
                    if (FOOTSWITCH_IN == OPEN) { 
                        relay_state ^= 1;                        
                        toggle_relay(relay_state);                                                
                    } else { // Is the user trying to change mode?
#if OPTIONSWITCH_IS_MOMENTARY
                        mode_change_counter -= 1;
                        if (mode_change_counter == 0) {
                            relay_mode = MOMENTARY;
                            mode_change_counter = MODE_CHANGE_PERIODS;

                            relay_state = OFF;                            
                            toggle_relay(relay_state);                                                        
                            blink_LED(6);
                        }
#endif
                    }
                }
            }
            // == Momentary mode ============================================                
            else if (relay_mode == MOMENTARY) { 
                if (FOOTSWITCH_IN == PRESSED && relay_state != ON) {
                    relay_state = ON;                    
                    toggle_relay(relay_state);                                       
                }

#if OPTIONSWITCH_IS_MOMENTARY
                if (FOOTSWITCH_IN == PRESSED) { // User is pressing
                    __delay_ms(GRACE_TIME / 5);
                    mode_change_counter -= 1;
                    if (mode_change_counter == 0) {
                        relay_mode = LATCHING;
                        mode_change_counter = MODE_CHANGE_PERIODS;
                        relay_state = OFF;                        
                        toggle_relay(relay_state);                                                
                        blink_LED(6);
                    }
                } else {
                    mode_change_counter =
                            MODE_CHANGE_PERIODS * MODE_CHANGE_RETURN_FACTOR;
                }
#endif
            }
            else {
               // Unknown mode!
            }
        }


        if (relay_mode == MOMENTARY) { // Momentary mode            
            if (FOOTSWITCH_IN == OPEN && relay_state != OFF) {
                relay_state = OFF;                
                toggle_relay(relay_state);                                
            }
        }

    } while(1); // Keep trucking!
    
}
