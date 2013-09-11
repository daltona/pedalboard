#include <Wire.h>
#include <Deuligne.h>
#include <Keypad.h>
#include <EEPROM.h>

const byte ROWS = 3; //four rows
const byte COLS = 5; //three columns
char keys[ROWS][COLS] = {
  {1,2,3,4,5},
  {6,7,8,9,10},
  {11,12,13,14, 15},
};
byte rowPins[ROWS] = {4, 3, 2}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {13, 12, 11, 10, 9}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );
static Deuligne lcd;

struct cc_data {
  int key;
  int number;
  char type;
} cc_data;

struct cc_data buttons_config[15];

void load_data(int key) {
  Serial.print("Load data for: "); Serial.print(key);
  Serial.print(EEPROM.read(key*2), HEX);
  Serial.print(" ");
  Serial.println(EEPROM.read(key*2+1), HEX);

  cc_data.key = key;
  cc_data.type = EEPROM.read(key * 2);  
  if (cc_data.type != 'P' && cc_data.type != 'C')
      cc_data.type = 'P';
  cc_data.number = EEPROM.read(key*2+1);
  if (cc_data.number > 127 || cc_data.number < 0) cc_data.number = 0;
}

void save_data(int key) {
  Serial.print("Save data for: "); Serial.print(key);
  Serial.print(cc_data.type, HEX);
  Serial.print(" ");
  Serial.println(cc_data.number, HEX);

  EEPROM.write(key*2, cc_data.type);
  EEPROM.write(key*2+1, cc_data.number);
  buttons_config[key - 1] = cc_data;
}

void setup(){
  Wire.begin(); // join i2c
  lcd.init(); // LCD init

  lcd.clear(); // Clear Display

  lcd.backLight(true); // Backlight ON

  lcd.setCursor(0,0); // Place cursor row 6, 1st line (counting from 0)
  lcd.print("Pedalboard 1.0");
  delay(1000);

  Serial.begin(9600);

  for (int i =1; i < 16; i++) {
    load_data(i);
    buttons_config[i-1] = cc_data;
  }
  lcd.clear();
}
  
static char tmp[16];


/**
 * Code for the custom made MIDI pedalBoard from Alex.
 *
 */

#define MENU_ENTER      0xA0
#define MENU_EXIT       0xA1

#define MENU_CONTINUE   0xB0
#define KEY_UP          0x81
#define KEY_DOWN        0x82
#define KEY_LEFT        0x83
#define KEY_RIGHT       0x80
#define KEY_ENTER       0x84

int select_cc_menu(int key) 
{
    struct cc_data * data = &cc_data;

    memset(tmp, ' ', 16);
    switch(key) {
    case MENU_ENTER:
        /** Init data */
         break;
    case KEY_UP: 
        data->number++;
        if (data->number > 127) data->number = 127;
        break;
    case KEY_DOWN:
        data->number--;
        if (data->number < 0) data->number = 0;
        break;
    case KEY_LEFT: 
        if (data->type == 'P') data->type = 'C'; else data->type = 'P';
        break;
    case KEY_RIGHT:
        if (data->type == 'P') data->type = 'C'; else data->type = 'P';
        break;
    case KEY_ENTER:
        save_data(data->key);
        return MENU_EXIT;        
    default:
        if (key > 0 && key < 0x80) 
          load_data(key);
    }
    sprintf(tmp, "%cC# %3d         ", data->type, data->number);
    lcd.setCursor(0, 1);
    lcd.print(tmp);
    return MENU_CONTINUE;    
}

int selected_key = -1;

int main_menu(int key) 
{
    switch (key) {
    case MENU_ENTER:
        selected_key = -1;
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Select key      ");
        break;

    default:
        if (key < 0x80 && key > 0) {
            selected_key = key;
            select_cc_menu(key);
        } else if ( selected_key != -1 ){
            int ret = select_cc_menu(key);
            if (ret == MENU_EXIT) return MENU_EXIT;
        }
        break;
    }    
    if (selected_key != -1) {
        snprintf(tmp, 16, "Button: %2d", selected_key);
    } else {
      sprintf(tmp, "Button: ??");
    }
    lcd.setCursor(0, 0);
    lcd.print(tmp);
}

static char * menu_items[] = {
    "Midi Chnl",
    "Brightness"
};
#define MENU_ITEM_COUNT (sizeof(menu_items) / sizeof(char *))

int (*current_menu)(int) = NULL;

int run_menu(int key) 
{
    if (current_menu == NULL)
      current_menu = main_menu;
    
    return current_menu(key);
}

uint16_t leds_state;

int getKey() {
  static int8_t oldkey = -1;
  char key = keypad.getKey();
  
  if (key != 0){
    Serial.println(key);
    return key;
  }

  key = lcd.get_key();		        // read the value from the sensor & convert into key press

  if (key != oldkey)				    // if keypress is detected
  {
    delay(50);		// wait for debounce time
    key = lcd.get_key();	   // read the value from the sensor & convert into key press
    if (key != oldkey)				
    {			
      oldkey = key;
      if (key >=0){
        return key + 0x80;
      }
    }
  }

  return -1;
}

struct midi_msg {
  int type;
};
#define MIDI_CC 0x80

struct midi_msg * midiRead() {
  return 0;
}

#define STATE_RUN 0
#define STATE_MENU 1

int patch = 0;
int bank = 0;

void loop() {
    struct midi_msg * midimsg;
    static int state = STATE_RUN;
    char tmp[16];
    int key = getKey();    

      switch(state) {
        case STATE_RUN:
          if (key == KEY_ENTER) {
            state = STATE_MENU;
            run_menu(MENU_ENTER);
            lcd.clear();
            break;
          } else if ( key > 0 && key < 0x80 ){
            patch = key;
            lcd.setCursor(10, 0);
            sprintf(tmp, "%cC:%3d", buttons_config[key-1].type, buttons_config[key-1].number);
            lcd.print(tmp);
          }
          lcd.setCursor(0,0); 
          lcd.print("RUN ");
          lcd.setCursor(0,1);
          snprintf(tmp, 16, "B %02d P %02d", bank, patch);
          lcd.print(tmp);        
          break;
       case STATE_MENU:
          if (MENU_EXIT == run_menu(key)) {
            state = STATE_RUN;        
            lcd.clear();
          }
          break;
      }

    if (midimsg = midiRead()) {
        switch (midimsg->type) {
            case MIDI_CC:
                break;
        }       
    }
}
