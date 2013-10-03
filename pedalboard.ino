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

#include "config.h"
#include "display.h"
#include "kemper.h"


#define MENU_ENTER      0xA0
#define MENU_EXIT       0xA1

#define MENU_CONTINUE   0xB0

#define FUNCTION_KEY_OFFSET 0x40

#define KEY_UP          0x41
#define KEY_DOWN        0x42
#define KEY_LEFT        0x43
#define KEY_RIGHT       0x40
#define KEY_ENTER       0x44

#define DEBUG



int brightness;

/** display buffer. */
static char dispbuf[20];

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

/** Switch configuration data */
struct button_data
{
    int key;
    int number;
    int state;
    char type;
} cc_data;

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

struct button_data buttons_config[15];

#define STATE_RUN 0
#define STATE_MENU 1

int patch = 0;
int bank = 0;

uint16_t leds_state;
int (*current_menu) (int) = NULL;
int (*prev_menu) (int) = NULL;

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
    cc_data.key = key;
    cc_data.type = EEPROM.read (key * 2);
    if (cc_data.type != 'P' && cc_data.type != 'C' && cc_data.type != 'T')
        cc_data.type = 'P';
    cc_data.number = EEPROM.read (key * 2 + 1);
    if (cc_data.number > 127 || cc_data.number < 0)
        cc_data.number = 0;
    buttons_config[key - 1] = cc_data;
}

void save_data (int key)
{
#ifdef DEBUG
    Serial.print ("Save data for: ");
    Serial.print (key);
    Serial.print (" ");
    Serial.print ((char)cc_data.type);
    Serial.print (" ");
    Serial.println (cc_data.number, HEX);
#endif
    EEPROM.write (key * 2, cc_data.type);
    EEPROM.write (key * 2 + 1, cc_data.number);
    buttons_config[key - 1] = cc_data;
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

void setup ()
{
    pinMode(SERIAL_CLOCK,    OUTPUT);      
    pinMode(SERIAL_DATA_OUT, OUTPUT);      
    pinMode(AUX_DISP_LATCH,  OUTPUT);      
    digitalWrite(SERIAL_CLOCK, LOW);
    digitalWrite(SERIAL_DATA_OUT, LOW);


    Wire.begin ();
    lcd.begin (16,2);
//    lcd.backLight (true);

    lcd.clear ();
    lcd.setCursor (0, 0);
    lcd.print ("Pedalboard 1.0");
    delay (1000);
    
    MIDI.begin ();

    for (int i = 1; i < 16; i++) {
        load_button_data (i);
        buttons_config[i - 1] = cc_data;
    }

    load_exp_data ();

    lcd.clear ();
    MIDI.setInputChannel (MIDI_CHANNEL_OMNI);
//    MIDI.turnThruOff ();
//    MIDI.setHandleClock (handleClock);
    MIDI.setHandleSystemExclusive (handle_sysex);
    keypad.addEventListener(keypadevent);

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
    memset (dispbuf, ' ', 16);
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
    struct button_data *data = &cc_data;

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
      data->type = data->type == 'P' ? 'P' : 
           data->type == 'C' ? 'P' : 'C';
      break;
    case 12:
      data->type = data->type == 'P' ? 'C' : 
           data->type == 'C' ? 'T' : 'T';
      break;
    case KEY_ENTER:
    case 15:
      save_data (data->key);
      return MENU_EXIT;
    default:
      break;
//      if (key > 0 && key < FUNCTION_KEY_OFFSET) load_button_data (key);
    }
    snprintf (dispbuf, 16, "%cC# %3d         ", data->type, data->number);
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
    switch (key) {
    case MENU_ENTER:
        cc_data.key = -1;
        lcd.clear ();
        lcd.setCursor (0, 0);
        lcd.print ("Select key      ");
        break;

    default:
        if (key < FUNCTION_KEY_OFFSET && key > 0 && cc_data.key == -1) {
            cc_data.key = key;
            load_button_data(key);
	} else if (cc_data.key != -1) {
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

    if (cc_data.key != -1) {
        snprintf (dispbuf, 16, "Button: %2d      ", cc_data.key);
    } else {
        sprintf (dispbuf, "Button: <Press>");
    }
    lcd.setCursor (0, 0);
    lcd.print (dispbuf);
    return ret;
}

extern int offset;

int config_menu(int key)
{
    int ret = MENU_CONTINUE;

    switch(key) {
        case MENU_ENTER: break;
        case 11: /* left */
            offset --; if (offset < 0) offset = 15; break;
        case 12: /* right */
            offset ++; if (offset > 15) offset = 0; break;
        case 13: /* down */
            break;
        case 14: /* up */
            break;
        case 15: /* enter */
            return MENU_EXIT;
            break;
    }
#ifdef DEBUG
    Serial.print("Offset: "); Serial.println(offset);
#endif
    lcd.setCursor (0, 0);
    lcd.print ("14 seg Offset   ");

    snprintf (dispbuf, 16, "Offset %2d       ", offset);
    lcd.setCursor (0, 1);
    lcd.print (dispbuf);

    return ret;
}

int brightness_menu(int key)
{
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
    {"Config", config_menu},
    {"Brightness", brightness_menu},
    {"Exit", menu_exit},
};

#define MENU_SIZE (sizeof(main_items)/sizeof(struct menu_item))
int main_menu(int key)
{
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

void keypadevent(KeypadEvent evt)
{
    keyState = keypad.getState();
#define KEYPAD_STATE_DEFAULT 0
#define KEYPAD_STATE_HELD    1
    static int8_t state = KEYPAD_STATE_DEFAULT;

#ifdef DEBUG
    Serial.print("keyevent : "); Serial.print(evt, HEX); Serial.print(" "); Serial.println(keyState, HEX);
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
        Serial.print("Key: "); Serial.println(key, HEX);
        return key;
    }

    return -1;
}

void handle_analog ()
{
    static int newValue[EXPRESSION_COUNT];
    for (int i = 0; i < EXPRESSION_COUNT; i++) {
        if ( ! exp_data[i].enable ) continue;
        newValue[i] = ((analogRead (exp_data[i].pin) / 8) + 4 * newValue[i]) / 5;
        if (exp_data[i].value != newValue[i]) {
            Serial.print("a"); Serial.print(i); Serial.print("=");Serial.println(newValue[i]);
            exp_data[i].value = newValue[i];
            lcd.setCursor (10, 0);
            sprintf (dispbuf, "E%d:%3d",
                    i, newValue[i]);
            lcd.print(dispbuf);
            MIDI.sendControlChange (exp_data[i].cc,
                    exp_data[i].value, midi_channel);
	}
    }
}

/*********************************************************************/
/*********            KEMPER SYSEX HANDLING                    *******/
/*********************************************************************/
struct sys_ex {
    char header[5];
    unsigned char fn;
    char id;
    char data[128];
} sysex_buffer = {{ 0x00, 0x20, 0x33, 0x02, 0x7f}, 0, 0, {0}, };

void handle_sysex(byte * data, byte len)
{
    struct sys_ex * s = (struct sys_ex *) (data+1);
    switch(s->fn) {
        case SYSEX_FN_STRING_PARAM:
            Serial.print("String: ");
            Serial.print(s->data[0] << 8 | s->data[1], HEX);
            Serial.print(" ");
            Serial.println((char *) &s->data[2]);
            aux_disp_print((char *)&s->data[2]);
            break;
        case SYSEX_FN_PARAM:
            Serial.print("Param: ");
            Serial.print((s->data[0] << 8) | s->data[1], HEX);
            Serial.print(" ");
            Serial.println((s->data[2] << 8) | s->data[3], HEX);
            break;
        default:
            Serial.print("Unknow sysex ");
            Serial.println(s->fn, HEX);
            break;
    }
}

void request_rig_name()
{
    sysex_buffer.fn = SYSEX_FN_REQUEST_STRING;
    * ((uint16_t*)sysex_buffer.data) = STRING_ID_RIG_NAME << 8;
    MIDI.sendSysEx(SYSEX_HEADER_SIZE+2, (const byte *)&sysex_buffer, 0);
}

void request_stomp_state()
{
    sysex_buffer.fn = SYSEX_FN_REQUEST_PARAM;
    sysex_buffer.data[0] = STOMP_A_PAGE;
    sysex_buffer.data[1] = ON_OFF_PARAM;
    MIDI.sendSysEx(SYSEX_HEADER_SIZE+2, (const byte *)&sysex_buffer, 0);
}

void handle_button (struct button_data *d)
{
    if (d->type == 'P') {
        MIDI.sendProgramChange (d->number, midi_channel);
        Serial.print("Send program change " ); Serial.println(d->number);
    } else if (d->type == 'C') {
        MIDI.sendControlChange (d->number, 1, midi_channel);
        Serial.print("Send control change " ); Serial.println(d->number);
    } else if (d->type == 'T') {
        MIDI.sendControlChange (d->number, d->state == 0 ? 1 : 0, midi_channel);
        d->state = d->state ? 0 : 1;
        Serial.print("Send toggle control change " ); Serial.print(d->state); Serial.print(" " ); Serial.println(d->number);
    }
    request_rig_name();
    request_stomp_state();
}

static int toggle = 0;
static uint32_t old = 0;

void loop ()
{
    static int state = STATE_RUN;
    static char changed = 1;
    
    int key = read_key ();

    if ( (millis() - old) > 2000 ) {
      old = millis();
      toggle++;
//      aux_disp_print(toggle & 1 ? (char*)"COUCOU  " : (char*)"LOUISE  AMOUR");
    }
    
    aux_disp_update();
    
    switch (state) {
    case STATE_RUN:
        if (key == KEY_ENTER) {
            state = STATE_MENU;
            run_menu (MENU_ENTER);
            lcd.clear ();
            break;
        } else if (key > 0 && key < FUNCTION_KEY_OFFSET) {
            patch = key;
            lcd.setCursor (10, 0);
            sprintf (dispbuf, "%cC:%3d",
                    buttons_config[key - 1].type,
                    buttons_config[key - 1].number);
            lcd.print (dispbuf);
            handle_button(&buttons_config[key-1]);
            changed = 1;
        } else if ( key == KEY_UP ) {
          bank++;
            changed = 1;
        } else if ( key == KEY_DOWN ) {
          bank--;
            changed = 1;
        }
        if (changed) {
            lcd.setCursor (0, 0);
            lcd.print ("RUN ");
            lcd.setCursor (0, 1);
            snprintf (dispbuf, 16, "B%02d P%02d", bank, patch);
            lcd.print (dispbuf);
            changed = 0;
        }
        //aux_disp_print(dispbuf);
        break;
    case STATE_MENU:
        if (MENU_EXIT == run_menu (key)) {
            changed = 1;
            state = STATE_RUN;
            lcd.clear ();
        }
        break;
    }

    handle_analog ();
    MIDI.read ();
}

