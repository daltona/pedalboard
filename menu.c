/**
 * Code for the custom made MIDI pedalBoard from Alex.
 *
 */

#define MENU_ENTER  0x80
#define MENU_EXIT   0x81

#define KEY_UP      0x90
#define KEY_DOWN    0x91
#define KEY_LEFT    0x92
#define KEY_RIGHT   0x93
#define KEY_ENTER   0x92

void print_header(char * str) 
{
    char tmp[16];

    memset(tmp, ' ', 16);
    strncpy(tmp, str, 16);
    lcd.setCursor(0, 0);
    lcd.print(tmp);
}

int select_cc_menu(int key, struct cc_data * data) 
{
    char tmp[16];

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
    case KEY_ENTER:
        return MENU_EXIT;        
    }
    snprintf(tmp, 16, "CC# %3d", data->number);
    lcd.setCursor(0, 1);
    return MENU_CONTINUE;    
}

int main_menu(int key) {
    char tmp[16];

    memset(tmp, ' ', 16);
    switch (key) {
    case MENU_ENTER:
        selected_key = -1;
        print_header("Select key...");
        break;
    case KEY_UP:
        break;
    case KEY_DOWN:
        break;
    default:
        selected_key = key;
        break;
    }    
    if (selected_key != -1) {
        snprintf(tmp, 16, "Selected key: %d", key);
    } 
}



