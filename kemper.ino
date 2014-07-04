/**
 * Code for the custom made MIDI pedalBoard from Alex.
 *
 */
#include <Keypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Deuligne.h>
#include <EEPROM.h>
#include <MIDI.h>
#include <SPI.h>

#include "config.h"
#include "display.h"
#include "kemper.h"
#include "types.h"

extern struct kpa_state_ kpa_state;

#define MENU_ENTER      0xA0
#define MENU_EXIT       0xA1

#define MENU_CONTINUE   0xB0

#define FUNCTION_KEY_OFFSET 0x40

#define KEY_UP          0x41
#define KEY_DOWN        0x42
#define KEY_LEFT        0x43
#define KEY_RIGHT       0x40
#define KEY_ENTER       0x44

//#define DEBUG
//#define DEBUG_MIDI

int brightness;

/** display buffer. */
extern unsigned char dp;

/* KEYPAD SECTION */
const byte ROWS = 3;		//three rowsq
const byte COLS = 5;		//five columns
char keys[ROWS][COLS] = {
    { 1,  2,  3,  4,  5},
    { 6,  7,  8,  9, 10},
    {11, 12, 13, 14, 15},
//    {KEY_LEFT, KEY_RIGHT, KEY_DOWN, KEY_UP, KEY_ENTER},
};

byte rowPins[ROWS] = { 2, 3, 4 };				//connect to the row pinouts of the keypad
byte colPins[COLS] = { 13, 12, 11, 10, 9};		//connect to the column pinouts of the keypad

Keypad keypad = Keypad (makeKeymap (keys), rowPins, colPins, ROWS, COLS);

LiquidCrystal_I2C lcd(0x38, 16, 2);  // set the LCD address to 0x20 for a 16 chars and 2 line display
//static Deuligne lcd;

byte midi_channel = 1;

const char DATA_TYPES_CHARS[] = "CPST1";
#define BUTTON_TYPE_CC        0
#define BUTTON_TYPE_PC        1
#define BUTTON_TYPE_CC_STATE  2
#define BUTTON_TYPE_CC_TOGGLE 3
#define BUTTON_TYPE_CC_ONE    4
#define TYPE_MAX              4

#define EXPRESSION_COUNT 4
#define EXPR_DATA_LEN    1
#define EXPR_DATA_OFFSET 32	//(16buttons * 2)

struct expression_data
{
    int pin;
    int value;
    int cc;
    int enable;
} exp_data[EXPRESSION_COUNT] =
{
  {A1, 0, 0, 0},
  {A2, 0, 0, 0},
  {A3, 0, 0, 0},
  {A4, 0, 0, 0},
};

struct button_data buttons_config[15] = {
    {1, CC_SLOT_1,         0, '1'},
    {2, CC_SLOT_2,         0, '1'},
    {3, CC_SLOT_3,         0, '1'},
    {4, CC_SLOT_4,         0, '1'},
    {5, CC_SLOT_5,         0, '1'},
    {6, CC_STOMP_A,        0, 'T'},
    {6, CC_STOMP_B,        0, 'T'},
    {6, CC_STOMP_C,        0, 'T'},
    {6, CC_STOMP_D,        0, 'T'},
    {10, CC_TAP_TEMPO,     0, 'T'},
    {11, CC_EFFECT_X,      0, 'T'},
    {12, CC_EFFECT_MOD,    0, 'T'},
    {13, CC_INCREASE_PERF, 0, 'S'},
    {14, CC_DECREASE_PERF, 0, 'S'},
    {15, CC_TUNER,         0, 'T'},
};

#define STATE_RUN 0
#define STATE_MENU 1

int patch = 0;
int bank = 0;

int (*current_menu) (int) = NULL;
int (*prev_menu) (int) = NULL;


void main_disp_print(char pos, char line, char * str) 
{
    lcd.setCursor(pos, line);
    lcd.print(str);
}

void main_disp_clear(void)
{
    lcd.clear();
}

/***********************************************************************/
/******                 NVM MANAGEMENT                              ****/
/***********************************************************************/
void load_button_data (int key)
{
#ifdef DEBUG
    Serial.print ("Load data for: ");
    Serial.print (key);
    Serial.print(" ");
    Serial.print ((char)EEPROM.read (key * 2));
    Serial.print (" ");
    Serial.println (EEPROM.read (key * 2 + 1), HEX);
#endif
    buttons_config[key - 1].key = key;
    buttons_config[key - 1].type = EEPROM.read (key * 2);
    if (buttons_config[key - 1].type != 'P' && buttons_config[key - 1].type != 'C' && buttons_config[key - 1].type != 'T')
        buttons_config[key - 1].type = 'P';
    buttons_config[key - 1].number = EEPROM.read (key * 2 + 1);
    if (buttons_config[key - 1].number > 127 || buttons_config[key - 1].number < 0)
        buttons_config[key - 1].number = 0;
//    buttons_config[key - 1] = cc_data;
}

void save_data (int key)
{
#ifdef DEBUG
    Serial.print ("Save data for: ");
    Serial.print (key);
    Serial.print (" ");
    Serial.print ((char)buttons_config[key - 1].type);
    Serial.print (" ");
    Serial.println (buttons_config[key - 1].number, HEX);
#endif
    EEPROM.write (key * 2, buttons_config[key - 1].type);
    EEPROM.write (key * 2 + 1, buttons_config[key - 1].number);
//    buttons_config[key - 1] = cc_data;
}

void load_exp_data() {
    for (int i=0; i < EXPRESSION_COUNT; i++) {
        exp_data[i].cc = EEPROM.read(EXPR_DATA_OFFSET + i*2);
        if (exp_data[i].cc > 127) exp_data[i].cc = 0;
        exp_data[i].enable = EEPROM.read(EXPR_DATA_OFFSET + i*2 + 1);
        if (exp_data[i].enable) exp_data[i].enable = 1;
    }
}

void save_exp_data(int i) {
    EEPROM.write (EXPR_DATA_OFFSET + i * 2, exp_data[i].cc);
    EEPROM.write (EXPR_DATA_OFFSET + i * 2 + 1, exp_data[i].enable);
}

void keypadevent(KeypadEvent evt);
void active_sense(void);

extern void handle_cc(byte chan, byte cc, byte value);
extern void handle_pc(byte chan, byte num);

void setup ()
{
    pinMode(SERIAL_CLOCK,    OUTPUT);      

    pinMode(SERIAL_DATA_OUT, OUTPUT);      
    pinMode(AUX_DISP_LATCH,  OUTPUT);      
    pinMode(LED_LATCH,  OUTPUT);      
    digitalWrite(AUX_DISP_LATCH, LOW);
    //SERIAL_CLOCK = MCLR


    Wire.begin ();
    lcd.begin (16,2);
//    lcd.backLight (true);

    lcd.clear ();
    lcd.setCursor (0, 0);
    lcd.print ("KempBoard 1.0");
    
    MIDI.begin ();

#if 0
    for (int i = 1; i < 16; i++) {
        load_button_data (i);
        buttons_config[i - 1] = cc_data;
    }
#endif
    load_exp_data ();

#if (defined DEBUG_MIDI || defined DEBUG)
    while(!Serial) ;
    Serial.print("Ram end: "); Serial.println(RAMEND);
#endif

    MIDI.setInputChannel (MIDI_CHANNEL_OMNI);
    MIDI.turnThruOff ();
    MIDI.setHandleClock (handleClock);
    MIDI.setHandleSystemExclusive (handle_sysex);
    MIDI.setHandleActiveSensing(handle_sense);
    MIDI.setHandleControlChange(handle_cc);
    MIDI.setHandleProgramChange(handle_pc);
    keypad.addEventListener(keypadevent);
    
    SPI.begin();
    SPI.setBitOrder(MSBFIRST);
    SPI.setDataMode(SPI_MODE1);
    SPI.setClockDivider(SPI_CLOCK_DIV16);


    digitalWrite(SERIAL_CLOCK, LOW);
    delay (100);
    digitalWrite(SERIAL_CLOCK, HIGH);
    delay (1000);
    lcd.clear ();
}

void handleClock (void)
{
    static int clock = 0;

    lcd.setCursor (0, 0);
    lcd.print (clock++);
}



/***********************************************************************/
/******           MENU IMPLEMENTATION AND MANAGEMENT                ****/
/***********************************************************************/
int expression_menu (int key)
{
    static int index = 0;
    static int param = 0;
    static int changed = 0;

#ifdef DEBUG
    Serial.print ("select_cc_menu: ");
    Serial.println (key);
#endif
    switch (key)
    {
    case MENU_ENTER:
		/** Init data */
        index = 0; param = 0;
        changed = 1;
        lcd.cursor();
      break;
    case KEY_UP:
    case 14:
      switch ( param ) {
          case 0: index++; if (index > (EXPRESSION_COUNT-1)) index = (EXPRESSION_COUNT-1); break;
          case 1: exp_data[index].cc++; if (exp_data[index].cc > 127) exp_data[index].cc = 127; break;
          case 2: if (exp_data[index].enable == 0) exp_data[index].enable = 1; else exp_data[index].enable = 0; break;
          case 3: /* GO TO CALIB MENU */ break;          
      }
      changed = 1;
      break;
    case KEY_DOWN:
    case 13:
      switch ( param ) {
          case 0: index--; if (index < 0) index = 0; break;
          case 1: exp_data[index].cc--; if (exp_data[index].cc < 0) exp_data[index].cc = 0; break;
          case 2: if (exp_data[index].enable == 0) exp_data[index].enable = 1; else exp_data[index].enable = 0; break;
          case 3: /* GO TO CALIB MENU */ break;          
      }
      changed = 1;
      break;
    case KEY_LEFT:
    case KEY_RIGHT:
    case 11:
      param--; if (param < 0) param = 0;
        changed = 1;
      break;
    case 12:
      param++; if (param > 4) param = 4;
        changed = 1;
      break;
    case KEY_ENTER:
    case 15:
        if (param == 4) {
          lcd.noCursor();          
          return MENU_EXIT;
        }
    default:
      break;
//      if (key > 0 && key < FUNCTION_KEY_OFFSET) load_button_data (key);
    }

    if ( changed ) {
        char dispbuf[17];
        changed = 0;
        snprintf (dispbuf, 16, "%1d CC%3d E%d CA EX", index, exp_data[index].cc, exp_data[index].enable);
        lcd.setCursor (0, 1);
        lcd.print (dispbuf);
        switch(param) {
            case 0: lcd.setCursor(0,1); break;
            case 1: lcd.setCursor(2,1); break;
            case 2: lcd.setCursor(8,1); break;
            case 3: lcd.setCursor(11,1); break;
            case 4: lcd.setCursor(14,1); break;
        }
        save_exp_data(index);
    }
#ifdef DEBUG
    Serial.println ("select_cc_menu exit CONTINUE");
#endif
    return MENU_CONTINUE;
}

int select_cc_menu (int key)
{
    struct button_data *data = &buttons_config[key - 1];
    char dispbuf[17];
#ifdef DEBUG
    Serial.print ("select_cc_menu: ");
    Serial.println (key);
#endif
    memset (dispbuf, ' ', 16);
    switch (key)
    {
    case MENU_ENTER:
		/** Init data */
      break;
    case KEY_UP:
    case 14:
      data->number++;
      if (data->number > 127)
	    data->number = 127;
      break;
    case KEY_DOWN:
    case 13:
      data->number--;
      if (data->number < 0)
	    data->number = 0;
      break;
    case KEY_LEFT:
    case KEY_RIGHT:
    case 11:
      data->type--; if (data->type < 0) data->type = 0; 
      break;
    case 12:
      data->type++; if (data->type > TYPE_MAX) data->type = TYPE_MAX;
      break;
    case KEY_ENTER:
    case 15:
      save_data (data->key);
      return MENU_EXIT;
    default:
      break;
//      if (key > 0 && key < FUNCTION_KEY_OFFSET) load_button_data (key);
    }
    snprintf (dispbuf, 16, "%cC# %3d         ", DATA_TYPES_CHARS[data->type], data->number);
    lcd.setCursor (0, 1);
    lcd.print (dispbuf);
#ifdef DEBUG
    Serial.println ("select_cc_menu exit CONTINUE");
#endif
    return MENU_CONTINUE;
}

int assign_menu (int key)
{
    int ret = MENU_CONTINUE;
    static int button = -1;
    char dispbuf[17];
    switch (key) {
    case MENU_ENTER:
        button = -1;
        lcd.clear ();
        lcd.setCursor (0, 0);
        lcd.print ("Select key      ");
        break;

    default:
        if (key < FUNCTION_KEY_OFFSET && key > 0 && button == -1) {
            button = key;
            load_button_data(key);
	} else if (button != -1) {
            ret = select_cc_menu (key);
#ifdef DEBUG
            Serial.print ("return: ");
            Serial.println (ret);
#endif
            if (ret == MENU_EXIT)
                return MENU_EXIT;
	}
        break;
    }

    if (button != -1) {
        snprintf (dispbuf, 16, "Button: %2d      ", button);
    } else {
        sprintf (dispbuf, "Button: <Press>");
    }
    lcd.setCursor (0, 0);
    lcd.print (dispbuf);
    return ret;
}

int brightness_menu(int key)
{
    char dispbuf[17];
    int ret = MENU_CONTINUE;

    switch(key) {
        case MENU_ENTER: break;
        case 11: /* left */
            brightness --; if (brightness < 0) brightness = 15; break;
        case 12: /* right */
            brightness ++; if (brightness > 15) brightness = 0; break;
        case 13: /* down */
            break;
        case 14: /* up */
            break;
        case 15: /* enter */
            return MENU_EXIT;
            break;
    }
#ifdef DEBUG
    Serial.print("brightness: "); Serial.println(brightness);
#endif
    lcd.setCursor (0, 0);
    lcd.print ("LCD Brightness  ");

    snprintf (dispbuf, 16, "%2d                ", brightness);
    lcd.setCursor (0, 1);
    lcd.print (dispbuf);

    return ret;
}

int menu_exit (int key)
{
    return MENU_EXIT;
}

struct menu_item {
    char * name;
    int (*menu)(int);
};

struct menu_item main_items[] = {
    {"Assign", assign_menu},
    {"Expression", expression_menu},
    {"Brightness", brightness_menu},
    {"Exit", menu_exit},
};

#define MENU_SIZE (sizeof(main_items)/sizeof(struct menu_item))
int main_menu(int key)
{
    char dispbuf[17];
    int ret = MENU_CONTINUE;
    static int index;

    if ( key != -1) lcd.clear();
    switch(key) {
        case MENU_ENTER: index = 0; break;
        case 11: /* left */
            index --; if (index < 0) index = 0; break;
        case 12: /* right */
            index ++; if (index > MENU_SIZE - 1) index = MENU_SIZE - 1; break;
        case 13: /* down */
            break;
        case 14: /* up */
            break;
        case 15: /* enter */
            if ( index == (MENU_SIZE -1) )
                return MENU_EXIT;
            current_menu = main_items[index].menu;
            prev_menu = main_menu;
            return current_menu(MENU_ENTER);
            break;
    }
    lcd.setCursor (0, 0);
    lcd.print ("Main Menu       ");
    snprintf (dispbuf, 16, "%s", main_items[index].name);
    lcd.setCursor (0, 1);
    lcd.print (dispbuf);
    return ret;
}

int run_menu (int key)
{
    int ret;

    if (current_menu == NULL)
        current_menu = main_menu;

    ret = current_menu(key);
    if (ret == MENU_EXIT && current_menu == main_menu) {
        return MENU_EXIT;
    } else if (ret == MENU_EXIT) {
        lcd.clear();
        current_menu = prev_menu;
    } else {
        return MENU_CONTINUE;
    }
}


byte keyState;
byte currentKey;
uint32_t refresh_display;
uint32_t refresh_aux_display;

void keypadevent(KeypadEvent evt)
{
    keyState = keypad.getState();
#define KEYPAD_STATE_DEFAULT 0
#define KEYPAD_STATE_HELD    1
    static int8_t state = KEYPAD_STATE_DEFAULT;


    if (keyState == PRESSED) {
//        digitalWrite(AUX_DISP_LATCH, HIGH);
    } else if (keyState == RELEASED || keyState == IDLE) {
//        digitalWrite(AUX_DISP_LATCH, LOW);
    }

#ifdef DEBUG
//    Serial.print("keyevent : "); Serial.print(evt, HEX); Serial.print(" "); Serial.println(keyState, HEX);
#endif
    if (keyState == RELEASED  && state == KEYPAD_STATE_DEFAULT) {
        currentKey = evt;
    } else if (keyState == HOLD) {
        currentKey = evt;
        if (state == KEYPAD_STATE_DEFAULT && currentKey == 15) {
             keyState = RELEASED;
             currentKey = KEY_ENTER;
             state = KEYPAD_STATE_HELD;
         }
    } else if (state == KEYPAD_STATE_HELD && keyState == IDLE) {
        state = KEYPAD_STATE_DEFAULT;
    }
}


int read_key ()
{
    static int8_t oldkey = -1;
    char key = keypad.getKey ();

    if (currentKey != -1 && keyState == RELEASED) {
        key = currentKey;
        currentKey = -1;
#ifdef DEBUG
        if (key != -1) {
            Serial.print("Key: "); Serial.println(key, HEX);
        }
#endif
        return key;
    }

    return -1;
}

void handle_analog ()
{
    static int newValue[EXPRESSION_COUNT];
    char dispbuf[16];
    for (int i = 0; i < EXPRESSION_COUNT; i++) {
        if ( ! exp_data[i].enable ) continue;
        newValue[i] = ((analogRead (exp_data[i].pin) / 8) + 4 * newValue[i]) / 5;
        if (exp_data[i].value != newValue[i]) {
#ifdef DEBUG
            Serial.print("a"); Serial.print(i); Serial.print("=");Serial.println(newValue[i]);
#endif
            exp_data[i].value = newValue[i];
            lcd.setCursor (10, 0);
            sprintf (dispbuf, "E%d:%3d",
                    i, exp_data[i].value * 3);
            lcd.print(dispbuf);
            MIDI.sendControlChange (exp_data[i].cc,
                    exp_data[i].value * 3, midi_channel);
	}
    }
}
#define DEBUG_BUTTON
void handle_button (struct button_data *d)
{ 
    if (d->key == 13) {
        MIDI.sendControlChange (49, 0, midi_channel);
        lcd.clear();
        return;
    } else if (d->key == 14) {
        MIDI.sendControlChange (48, 0, midi_channel);
        lcd.clear();
        return;
    }
    if (d->type == 'P') {
        MIDI.sendProgramChange (d->number, midi_channel);
#ifdef DEBUG_BUTTON
        Serial.print("Send program change " ); Serial.println(d->number);
#endif
    } else if (d->type == 'C' || d->type == '1') {
        MIDI.sendControlChange (d->number, 1, midi_channel);
#ifdef DEBUG_BUTTON
        Serial.print("Send control change " ); Serial.println(d->number);
#endif
    } else if (d->type == 'T') {
        MIDI.sendControlChange (d->number, d->state == 0 ? 1 : 0, midi_channel);
        d->state = d->state ? 0 : 1;
#ifdef DEBUG_BUTTON
        Serial.print("Send toggle control change " ); Serial.print(d->state); Serial.print(" " ); Serial.println(d->number);
#endif
    }
}

static int toggle = 0;
static uint32_t old = 0;
static int g_state = STATE_RUN;
static uint32_t last_disp_update = 0; 
extern unsigned char dp;
uint16_t old_leds;

void loop ()
{
    static char changed = 1;
    
    int key = read_key ();
    
    if(millis() - last_disp_update > 50) {
        aux_disp_update();
        update_leds();
        last_disp_update = millis();    
/*
lcd.setCursor (10, 1);
            sprintf(dispbuf, "%04x", led_state);
            lcd.print(dispbuf);
            */
    }
    MIDI.read ();
    kemper_process();
    
    switch (g_state) {
    case STATE_RUN:
        if (key == KEY_ENTER) {
            g_state = STATE_MENU;
            run_menu (MENU_ENTER);
            lcd.clear ();
            break;
        } else if (key > 0 && key < FUNCTION_KEY_OFFSET) {
            patch = key;
#ifdef SHOW_MSG
            lcd.setCursor (10, 0);
            sprintf (dispbuf, "%cC:%3d",
                    buttons_config[key - 1].type,
                    buttons_config[key - 1].number);
            lcd.print (dispbuf);
#endif
            handle_button(&buttons_config[key-1]);
            if(key == 15) {
                if(buttons_config[14].state == 1) {
                    old_leds = get_leds();
                    set_leds(2);
                } else {
                    set_leds(old_leds & ~2);
                }
                refresh_aux_display = refresh_display = millis() + 100;
                lcd.clear();
            }
        }
        if (buttons_config[14].state == 1) {
#define TUNE_STEP 200
            int t = ((t / 4 * 3) + kpa_state.tune / 4);
            if (t < 0) {
                if ( t < -(TUNE_STEP * 4))
                    dp = 0xf;
                else if (t < -(TUNE_STEP * 3))
                    dp = 0xe;
                else if (t < -(TUNE_STEP * 2))
                    dp = 0xc;
                else if (t < -(TUNE_STEP))
                    dp = 0x8;
                else dp = 0;
            } else if (t > 0) {
                if (t > (TUNE_STEP * 4))
                    dp = 0xf0;
                else if (t > (TUNE_STEP * 3))
                    dp = 0x70;
                else if (t > (TUNE_STEP * 2))
                    dp = 0x30;
                else if (t > (TUNE_STEP))
                    dp = 0x10;
                else dp = 0;
            } else dp = 0;
            
            if (millis() - old > 300) {
                char dispbuf[17];
                old = millis();      
                char left, right;

                if (t < 0) 
                    left = '>';
                else if (t < 200)
                    left = '>';
                else left = ' ';
                
                if (t > 0)
                    right = '<';
                else if (t > -200)
                    right = '<';
                else right = ' ';
                
                snprintf(dispbuf, 8, " %c %s %c ", left, kpa_state.note, right);
                aux_disp_fixed(dispbuf);
                lcd.setCursor(0,0);
                lcd.print(dispbuf);
                lcd.print(t);
            }
        } else {
            if (millis() > refresh_display) {
                refresh_display = millis() + 5000;
                lcd.setCursor (0, 0);
                lcd.print(kpa_state.perf_name);
                lcd.setCursor (0, 1);
                lcd.print(kpa_state.stomps);
            }
            if (millis() > refresh_aux_display) {
                refresh_aux_display = millis() + 5000;
                kpa_state.showing_rig = 0;
//                dp = kpa_state.effects & 0xc0;
                aux_disp_print(kpa_state.slot_name);
            }
        }
        break;
    case STATE_MENU:
        if (MENU_EXIT == run_menu (key)) {
            changed = 1;
            g_state = STATE_RUN;
            lcd.clear ();
        }
        break;
    }

    handle_analog ();
}

