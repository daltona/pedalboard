#include <MIDI.h>
#include "config.h"
#include "display.h"
#include "kemper.h"
#include "types.h"


//#define DEBUG_KEMPER
#define SHOW_RIG_TIME 2000

extern button_data buttons_config[];
extern uint32_t refresh_display;
extern uint32_t refresh_aux_display;

const char * note_names[] = {
    "C ",
    "C#",
    "D ",
    "D#",
    "E ",
    "F ",
    "F#",
    "G ",
    "G#",
    "A ",
    "A#",
    "B ",  
};

/*********************************************************************/
/*********            KEMPER SYSEX HANDLING                    *******/
/*********************************************************************/
struct sys_ex {
    char header[5];
    unsigned char fn;
    char id;
    unsigned char data[64];
} sysex_buffer = {{ 0x00, 0x20, 0x33, 0x02, 0x7f}, 0, 0, {0}, };

struct st {
    uint8_t ack : 1;
    uint8_t sense : 1;
} st = {0, 0};
#define ack_received st.ack
#define sense_received st.sense

uint32_t last_ack = 0;
uint32_t last_tune;
uint32_t perf_display;


#define KEMPER_STATE_WAIT_SENSE         0
#define KEMPER_STATE_WAIT_INITIAL_DATA  1
#define KEMPER_STATE_REQUEST_PARAMS     2
#define KEMPER_STATE_RUN                3

static int state = KEMPER_STATE_WAIT_SENSE;

static uint8_t last_ack_value;


/**
 * Handle MIDI Active sense message.
 *
 */
void handle_sense(void) {
    sense_received = 1;
#ifdef DEBUG_KEMPER
    Serial.println("Sense");
#endif
}

struct stmp_cfg_ {
    uint8_t type;
    uint8_t enabled;
    char name[7]; 
} scfg[8];

struct kpa_state_ kpa_state = { 
    0, 
    0,
    {0, },
    {0, },
    {0, },
    0,
    0,
    0x1f    
};

#define SET_EFFECTS(_val_, _idx_) ((_val_) ? kpa_state.effects |= (_idx_) : kpa_state.effects &= ~(_idx_))


byte current_program;
byte bank_select = 0;

void handle_cc(byte chan, byte num, byte value)
{
#ifdef DEBUG_KEMPER
    Serial.print("Control Change: "); Serial.print(num); Serial.print(" : "); Serial.println(value);
#endif
    if (num = 0x32) {
        bank_select = value;
    }
}

int get_selected_slot() {
    int selected_rig = (bank_select * 128) + current_program;
    byte idx = (selected_rig % 5);

    return idx;
}

void handle_pc(byte chan, byte number)
{
    int selected_rig = (bank_select * 128) + number;
    byte idx = (selected_rig % 5);

#ifdef DEBUG_KEMPER
    Serial.print("idx: "); Serial.println(idx, HEX);
    Serial.print("enabled: "); Serial.println(kpa_state.enabled_slots, HEX);
    Serial.print("Program change: "); Serial.println(number);
#endif

    if (kpa_state.enabled_slots & (1 << idx)) {
        current_program = number;
    }
}



/**
 * Dump a kemper sysex message.
 *
 */
void sysex_dump(struct sys_ex * s, int len) {
    Serial.print(s->fn, HEX);
    Serial.print(" ");
    Serial.println(len);
    for (int i = 0; i < len - 6; i++) {              
              if (isalnum(s->data[i])) {
                Serial.print((char)s->data[i]);
              } else {
                  switch(s->data[i]) {
                      case ' ' :
                      case '+' :
                         Serial.print(s->data[i]);
                        break;
                     default: 
                        Serial.print(" ");
                        Serial.print((unsigned int)s->data[i], HEX);
                        Serial.print(" ");
                        break;
                  }
              }
            }
            Serial.println(" ");
}

/**
 * Confifure stomp.
 *
 * @param: param the parameter from the message
 * @value: the value from the message
 * @name: the name of the effect
 */
void config_stomp(uint16_t param, uint16_t value, char * name, char enabled) 
{
    char idx = 0;

    if(param == PARAM_STOMP_A_TYPE)
        idx = 0;
    else if (param == PARAM_STOMP_B_TYPE)
        idx = 1;
    else if ( param == PARAM_STOMP_C_TYPE)
        idx = 2;
    else if ( param == PARAM_STOMP_D_TYPE)
        idx = 3;
    else if ( param == PARAM_STOMP_X_TYPE)
        idx = 4;
    else if ( param == PARAM_STOMP_MOD_TYPE)
        idx = 5;
        
    scfg[idx].enabled = enabled;
    scfg[idx].type   = value;
    strncpy(scfg[idx].name, name, 6);
}

/**
 * Handle incomming parameter message.
 *
 */
char handle_param(int param, int value ) 
{
    char handled = 1;
    switch(param) {
    case PARAM_STOMP_A_STATE:
        buttons_config[5].state = value;
        SET_EFFECTS(value, 1);
        sprintf(kpa_state.stomps, "%s %s", scfg[0].name, value ? "ON" : "OFF");
        value ? set_led_mask(0x80) : clear_led_mask(0x80);
        break;      
    case PARAM_STOMP_B_STATE:
        buttons_config[6].state = value;
        SET_EFFECTS(value, 2);
        sprintf(kpa_state.stomps, "%s %s", scfg[1].name, value ? "ON" : "OFF");
        value ? set_led_mask(0x100) : clear_led_mask(0x100);
        break;      
    case PARAM_STOMP_C_STATE:
        buttons_config[7].state = value;
        SET_EFFECTS(value, 4);
        sprintf(kpa_state.stomps, "%s %s", scfg[2].name, value ? "ON" : "OFF");
        value ? set_led_mask(0x200) : clear_led_mask(0x200);
        break;      
    case PARAM_STOMP_D_STATE:
        buttons_config[8].state = value;
        SET_EFFECTS(value, 8);
        sprintf(kpa_state.stomps, "%s %s", scfg[3].name, value ? "ON" : "OFF");
        value ? set_led_mask(0x400) : clear_led_mask(0x400);
        break;      
    case PARAM_STOMP_X_STATE:
        SET_EFFECTS(value, 0x10);
        buttons_config[10].state = value;
        sprintf(kpa_state.stomps, "%s %s", scfg[4].name, value ? "ON" : "OFF");
        value ? set_led_mask(0x1000) : clear_led_mask(0x1000);
        break;      
    case PARAM_STOMP_MOD_STATE:
        SET_EFFECTS(value, 0x20);
        buttons_config[11].state = value;
        sprintf(kpa_state.stomps, "%s %s", scfg[5].name, value ? "ON" : "OFF");
        value ? set_led_mask(0x2000) : clear_led_mask(0x2000);
        break;      
    case PARAM_DELAY_STATE:
        SET_EFFECTS(value, 0x40);
        sprintf(kpa_state.stomps, "%s %s", scfg[6].name, value ? "ON" : "OFF");
        value ? set_led_mask(0x4000) : clear_led_mask(0x4000);
        break;      
    case PARAM_REVERB_STATE:
        SET_EFFECTS(value, 0x80);
        sprintf(kpa_state.stomps, "%s %s", scfg[7].name, value ? "ON" : "OFF");
        value ? set_led_mask(0x8000) : clear_led_mask(0x8000);
        break;      
    default:
        handled = 0;
        break;
    }
    for (int i = strlen(kpa_state.stomps); i < 16; i++) {
        kpa_state.stomps[i] = ' ';
    }
    refresh_display = millis() + 100;
    return handled;
}

/**
 * Handle sysex function.
 *
 * TODO: generic sysex function that will call the Kemper
 * specific one when the correct header matched.
 */
void handle_sysex(byte * data, byte len)
{
    char handled = 0;
    static struct sys_ex * s = (struct sys_ex *) (data+1);
    static int param;
    static int value;
    switch(s->fn) {
        case SYSEX_FN_STRING_PARAM:
            param = (s->data[0] << 7 | s->data[1]);
#ifdef DEBUG_KEMPER
            Serial.print("String: ");
            Serial.print(param, HEX);
            Serial.print(" ");
            Serial.println((char *) &s->data[2]);
#endif
            if ( param == 1 ) {
                //aux_disp_print((char *)&s->data[2]);
                strcpy(kpa_state.slot_name, (char*) &s->data[2]);
                if (!(kpa_state.showing_rig))
                    refresh_aux_display = millis() + 100;
            } 
            break;
        case SYSEX_FN_PARAM:
            param = (s->data[0] << 7) | s->data[1];
            value = (s->data[2] << 7) | s->data[3];
            if (param == 0x3e0f) {
                kpa_state.tune = (value - 8000);
                handled = 1;
            } else if (param == 0x3ed4) {
                kpa_state.note = (char *)note_names[value%12];
                last_tune = millis();
                handled = 1;
            } else if (param == 0x3e00) { 
                value ? set_led(11) : clear_led(11);
                handled = 1;
            } else { 
                handled = handle_param(param, value);
            } 
#ifdef DEBUG_KEMPER
            if (!handled) {
                Serial.print("Param: ");
                Serial.print(param, HEX);
                Serial.print(" ");
                Serial.println(value, HEX);
            }
#endif
            break;
            
        case SYSEX_FN_ACK:
#ifdef DEBUG_KEMPER
            Serial.print("Ack: ");
            Serial.print(s->data[0], HEX);
            Serial.print(" ");
            Serial.println(s->data[1], HEX);
#endif
            if (s->data[0] == 0x7f) {
                 if (ack_received && (last_ack_value + 1 != s->data[1])) {
//                    last_ack = 0;
//                    ack_received = 0;
                } else {
                    ack_received = 1;
                }
                last_ack = millis();
                last_ack_value = s->data[1];
            }
            break;
        
        case SYSEX_FN_EFFECT_CONFIG:
            /* Stomp config...
             * ID: (=PARAM) s->data[0] << 7 | s->data[1]
             * VALUE: (=TYPE) s->data[2] << 7 | s->data[3]
             * LABEL: s->data[4]
             * ENABLED: after short LBL
             */
            param = ((s->data[0]<<7) | s->data[1]);
            value = ((s->data[2]<<7) | s->data[3]);
#if 0
            sysex_dump(s, len);
            Serial.print(param, HEX); /* ID */
            Serial.print(" ");
            Serial.print(value, HEX); /* TYPE */
            Serial.print(" ");
            Serial.println((char *)&s->data[4]);
#endif
            {
                char * str = (char *)&s->data[4];
                config_stomp(param, value, str, *(str + strlen(str)));
            }
            break;
            
        case SYSEX_FN_EXT_PARAM: /* Set slot names s->data[4] is the slot (0 == performance) */
            {
                uint32_t param;
                uint32_t value;
                param = (s->data[0] << 28) | (s->data[1] << 21) | (s->data[2] << 14) | s->data[3] << 7 | s->data[4] ;
                value = (s->data[5] << 28) | (s->data[6] << 21)  | (s->data[7] << 14) | (s->data[8] << 7) | s->data[9];
#ifdef DEBUG_KEMPER
                Serial.print("P:");
                Serial.print(param, HEX);
                Serial.print(" ");
                Serial.println(value, HEX);                
#endif
                if (param >= 0x4000 && param <= 0x4004) {
                    value ? kpa_state.enabled_slots |= 1 << (param - 0x4000) :
                    kpa_state.enabled_slots &= ~(1 << (param - 0x4000));
                }
            }
            break;

        case SYSEX_FN_EXT_STRING_PARAM:
            uint32_t param;
            param = (s->data[0] << 28) | (s->data[1] << 21) | (s->data[2] << 14) | s->data[3] << 7 | s->data[4] ;

            if (param == 0x4000) { /* 0x4001 => 0x4005 the slots names. */
                strcpy(kpa_state.perf_name, (const char *)&s->data[5]);
                refresh_aux_display = millis() + SHOW_RIG_TIME;
                kpa_state.showing_rig = 1;
                aux_disp_print(kpa_state.perf_name);
            }
#ifdef DEBUG_KEMPER
            Serial.print("S:");
            Serial.print(param, HEX);
            Serial.print(" ");
            Serial.println((char *)&s->data[5]);
#endif
            break;
        default:
            sysex_dump(s, len);
            break;
    }

}

/**
 * Send a request param message to kemper.
 *
 * @param param the parameter number to request.
 */
void request_param(int param)
{
    int sz = 2;
#ifdef DEBUG_KEMPER
    Serial.print("Requesting param : "); Serial.println(param, HEX);
#endif
    if (param < 0x4000) {
        sysex_buffer.fn = SYSEX_FN_REQUEST_PARAM;
        sysex_buffer.data[0] = (param >> 7) & 0xff;
        sysex_buffer.data[1] = param & 0x7f;
    } else {
        sysex_buffer.fn = SYSEX_FN_REQUEST_EXT_PARAM;
        sysex_buffer.data[0] = (param >> 28) & 0x7f;
        sysex_buffer.data[1] = (param >> 21) & 0x7f;
        sysex_buffer.data[2] = (param >> 14) & 0x7f;
        sysex_buffer.data[3] = (param >> 07) & 0x7f;
        sysex_buffer.data[4] = (param) & 0x7f;
        sz = 5;
    }
#ifdef DEBUG_KEMPER
    for (int i = 0; i < sz; i++) {
        Serial.print(sysex_buffer.data[i], HEX); Serial.print(" ");
    } 
    Serial.println("");
#endif
    MIDI.sendSysEx(SYSEX_HEADER_SIZE+sz, (const byte *)&sysex_buffer, 0);
}

/**
 * Animate the buttons LEDs
 *
 * the annimation is shown while we are connecting to the kemper.
 *
 */
void animate_led() {
    static uint32_t last_led_anim;
    if ((millis() - last_led_anim) > 200) {
        last_led_anim = millis();
        int b = get_leds();
        if (b == 0) b = 1;
        if (b == 0x8000) {
            b = 0x1;
        } else {
            b *= 2;
        }
        set_leds(b);
    }
}

/**
 * Kemper main processing loop function.
 *
 */
void kemper_process() 
{
    static long last_sent = 0;
    static char current_slot = 0;
    
    switch (state) {
        case KEMPER_STATE_WAIT_SENSE:
            aux_disp_print("KempWait");
            main_disp_print(0, 0, "Wait for Kemper");
            if ( sense_received ) {
                main_disp_clear();
                state = KEMPER_STATE_WAIT_INITIAL_DATA;
            }
            animate_led(); break;
        case KEMPER_STATE_WAIT_INITIAL_DATA:
            if (millis() - last_sent > 2000) {
#ifdef DEBUG_KEMPER
                Serial.println("Init kemper...");
#endif
                sysex_buffer.fn = 0x03;
                sysex_buffer.data[0] = 0x7f;
                sysex_buffer.data[1] = 0x7f;
                strcpy((char *)sysex_buffer.data+2, "KempBoard ");
                MIDI.sendSysEx(SYSEX_HEADER_SIZE+2+10, (const byte *)&sysex_buffer, 0);

                sysex_buffer.fn = 0x7e;
                sysex_buffer.data[0] = 0x40;
                sysex_buffer.data[1] = 2;
                sysex_buffer.data[2] = 0x6e | (ack_received ? 0 : 1);
                sysex_buffer.data[3] = 5;
                MIDI.sendSysEx(SYSEX_HEADER_SIZE+4, (const byte *)&sysex_buffer, 0);
                last_sent = millis();
            }
            animate_led();
            if (ack_received) {
                current_slot = 0;
                set_leds(0);
                state = KEMPER_STATE_REQUEST_PARAMS;
            }
            break;
       case KEMPER_STATE_REQUEST_PARAMS:
            if (current_slot < 5 ) {
                if (millis() - last_sent > 200) {
					last_sent = millis();
					request_param(0x4000 + current_slot++);
                }
           } else {
               current_slot = 0;
               state = KEMPER_STATE_RUN;
           }
           break;

       case KEMPER_STATE_RUN:
            if (millis() - last_sent > 5000) {
                sysex_buffer.fn = 0x7e;
                sysex_buffer.data[0] = 0x40;
                sysex_buffer.data[1] = 2;
                sysex_buffer.data[2] = 0x6e | (ack_received ? 0 : 1);
                sysex_buffer.data[3] = 5;
#ifdef DEBUG_KEMPER
                Serial.println("Send beacon");
#endif
                MIDI.sendSysEx(SYSEX_HEADER_SIZE+4, (const byte *)&sysex_buffer, 0);
                last_sent = millis();
            }
            if (millis() - last_ack > 5000) {
                state = KEMPER_STATE_WAIT_INITIAL_DATA;
                ack_received = 0;
                sense_received = 0;
            }
            break;       
    }

}


