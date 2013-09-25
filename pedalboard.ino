/**
 * Code for the custom made MIDI pedalBoard from Alex.
 *
 */
#include <Keypad.h>
#include <Wire.h>
#include <Deuligne.h>
#include <EEPROM.h>
#include <MIDI.h>

#include "config.h"
#include "display.h"


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

static Deuligne lcd;

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
} exp_data[EXPRESSION_COUNT] =
{
  {A1, 0, 0},
  {A2, 0, 0},
  {A3, 0, 0},
  {A4, 0, 0},
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

void load_expression_data (int expr)
{
    exp_data[expr].cc = EEPROM.read (EXPR_DATA_OFFSET + expr * EXPR_DATA_LEN);
}

void load_button_data (int key)
{
#ifdef DEBUG
    Serial.print ("Load data for: ");
    Serial.print (key);
    Serial.print (EEPROM.read (key * 2), HEX);
    Serial.print (" ");
    Serial.println (EEPROM.read (key * 2 + 1), HEX);
#endif
    cc_data.key = key;
    cc_data.type = EEPROM.read (key * 2);
    if (cc_data.type != 'P' && cc_data.type != 'C')
        cc_data.type = 'P';
    cc_data.number = EEPROM.read (key * 2 + 1);
    if (cc_data.number > 127 || cc_data.number < 0)
        cc_data.number = 0;
}

void save_data (int key)
{
#ifdef DEBUG
    Serial.print ("Save data for: ");
    Serial.print (key);
    Serial.print (cc_data.type, HEX);
    Serial.print (" ");
    Serial.println (cc_data.number, HEX);
#endif
    EEPROM.write (key * 2, cc_data.type);
    EEPROM.write (key * 2 + 1, cc_data.number);
    buttons_config[key - 1] = cc_data;
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
    lcd.init ();

    lcd.clear ();
    lcd.backLight (true);
    lcd.setCursor (0, 0);
    lcd.print ("Pedalboard 1.0");
    delay (1000);
    
    MIDI.begin ();

    for (int i = 1; i < 16; i++) {
        load_button_data (i);
        buttons_config[i - 1] = cc_data;
    }

    for (int i = 0; i < EXPRESSION_COUNT; i++) {
        load_expression_data (i);
    }

    lcd.clear ();
    MIDI.setInputChannel (MIDI_CHANNEL_OMNI);
    MIDI.turnThruOff ();
    MIDI.setHandleClock (handleClock);
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
    case 12:
      data->type = data->type == 'P' ? 'C' : 'P';
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
    {"Config", config_menu},
    {"Brightness", brightness_menu},
    {"Exit", menu_exit},
};

int main_menu(int key)
{
    int ret = MENU_CONTINUE;
    static int index;

    switch(key) {
        case MENU_ENTER: index = 0; break;
        case 11: /* left */
            index --; if (index < 0) index = 0; break;
        case 12: /* right */
            index ++; if (index > 2) index = 2; break;
        case 13: /* down */
            break;
        case 14: /* up */
            break;
        case 15: /* enter */
            if ( index == 2 )
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

    key = lcd.get_key ();		// read the value from the sensor & convert into key press

    if (key != oldkey) {
        delay (50);		// wait for debounce time
        key = lcd.get_key ();	// read the value from the sensor & convert into key press
        if (key != oldkey) {
            oldkey = key;
            if (key >= 0) {
    	        return key + FUNCTION_KEY_OFFSET;
            }
	}
    }
    return -1;
}

void handle_analog ()
{
    for (int i = 0; i < 1/*EXPRESSION_COUNT*/; i++) {
        int newValue = analogRead (exp_data[i].pin);
        //Serial.print("a"); Serial.print(i); Serial.print("=");Serial.println(newValue);
        if (exp_data[i].value != newValue) {
            exp_data[i].value = newValue;
            lcd.setCursor (10, 0);
            sprintf (dispbuf, "E%d:%3d",
                    i, newValue);
            lcd.print(dispbuf);
            MIDI.sendControlChange (exp_data[i].cc,
                    exp_data[i].value, midi_channel);
	}
    }
}

void handle_button (struct button_data *d)
{
    if (d->type == 'P') {
        MIDI.sendProgramChange (d->number, midi_channel);
    } else if (d->type == 'C') {
        MIDI.sendControlChange (d->number, d->state == 0 ? 1 : 0, midi_channel);
    }
}

static int toggle = 0;
static uint32_t old = 0;

void loop ()
{
    static int state = STATE_RUN;


    int key = read_key ();

    if ( (millis() - old) > 2000 ) {
      old = millis();
      toggle++;
      aux_disp_print(toggle & 1 ? (char*)"COUCOU  " : (char*)"LOUISE  ");
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
        } else if ( key == KEY_UP ) {
          bank++;
        } else if ( key == KEY_DOWN ) {
          bank--;
        }
        lcd.setCursor (0, 0);
        lcd.print ("RUN ");
        lcd.setCursor (0, 1);
        snprintf (dispbuf, 16, "B%02d P%02d", bank, patch);
        lcd.print (dispbuf);
        //aux_disp_print(dispbuf);
        break;
    case STATE_MENU:
        if (MENU_EXIT == run_menu (key)) {
            state = STATE_RUN;
            lcd.clear ();
        }
        break;
    }

    handle_analog ();
    MIDI.read ();
}

