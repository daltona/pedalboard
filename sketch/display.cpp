#include <SPI.h>
#include "display.h"
#include "config.h"

uint16_t getdata(char);
uint16_t led_state = 0;
uint8_t dp;

void shiftout(uint8_t val) {
    int ret = SPI.transfer(val);
//    Serial.print(val, HEX); Serial.print(" "); Serial.println(ret, HEX);
}

extern int get_selected_slot();
/***********************************************************************/
/******                 LEDS MANAGEMENT                             ****/
/***********************************************************************/

/**
 * Send led state to the led shift registers.
 */
void update_leds() {    
    led_state &= ~0x7c;
    led_state |= (1 << (get_selected_slot() + 2));
    uint16_t tmp = led_state >> 1;
    shiftout(tmp & 0xff);
    shiftout((tmp >> 8) & 0xff);
    /*latch*/
    digitalWrite(LED_LATCH, HIGH);
    digitalWrite(LED_LATCH, HIGH);
    digitalWrite(LED_LATCH, LOW);
//    Serial.print("update_leds ");
//    Serial.println(led_state, HEX);
}

uint16_t get_leds() {
    return led_state;
}

void set_leds(uint16_t leds) {
    led_state = leds;
//    Serial.print("ini led: "); Serial.print(leds, HEX); Serial.print(" "); Serial.println(led_state, HEX);
}

void set_led_mask(uint16_t mask) {
    led_state |= mask;
//    Serial.print("Set msk: "); Serial.print(mask, HEX); Serial.print(" "); Serial.println(led_state, HEX);
}

void clear_led_mask(uint16_t mask) {
    led_state &= ~mask;
//    Serial.print("Clr msk: "); Serial.print(mask, HEX); Serial.print(" "); Serial.println(led_state, HEX);
}

void set_led(int led) {
    led_state |= (1<<led);    
//    Serial.print("Set led: "); Serial.print(led); Serial.print(" "); Serial.println(led_state, HEX);
}

void clear_led(int led) {
    led_state &= ~(1<<led);
//    Serial.print("Clr led: "); Serial.print(led); Serial.print(" "); Serial.println(led_state, HEX);
}

/***********************************************************************/
/******                 AUX DISPLAY MANAGEMENT                      ****/
/***********************************************************************/

char aux_disp_buf[32];
int aux_disp_offset;
int string_len;
int string_offset;
static uint32_t last;
int current_pos = 0;
char enable_updates = 1;

/**
 * Set buffer to be displayed (and scrolled) in the 8 char 
 * 14 digit display. 
 */
void aux_disp_print(char * str) {
    strncpy(aux_disp_buf, str, 32);
    aux_disp_offset = 0;
    string_len = strlen(str);
    string_offset = 0;
    enable_updates = 1;
}

/**
 * Enabled scrolling in the 14 seg display.
 */
void aux_disp_refresh() {
    enable_updates = 1;
    aux_disp_offset = 0;    
}

/**
 * Send data to the 14seg display.
 */
void aux_disp_write(char * str) {
    int len = strlen(str) > 8 ? 8 : strlen(str);
    int i;
    for (i = 0; i < len; i++) {
        uint16_t tmp = getdata(str[i]);
        if (dp & (1 << i)) tmp |= 0x4000;
        shiftout(tmp >> 8);
        shiftout(tmp & 0xff);
    }
    for ( ; i<8; i++) {
        shiftout(dp & (1 << i) ? 0x40 : 0x0);
        shiftout(0);
    }
    digitalWrite(AUX_DISP_LATCH, LOW);
    digitalWrite(AUX_DISP_LATCH, HIGH);
    digitalWrite(AUX_DISP_LATCH, HIGH);
    digitalWrite(AUX_DISP_LATCH, HIGH);
    digitalWrite(AUX_DISP_LATCH, HIGH);
    digitalWrite(AUX_DISP_LATCH, HIGH);
    digitalWrite(AUX_DISP_LATCH, HIGH);
    digitalWrite(AUX_DISP_LATCH, LOW);
}

/**
 * Set text to show on 14seg display and disable
 * scrolling.
 */
void aux_disp_fixed(char * str) {
    enable_updates = 0;
    aux_disp_write(str);
}

void aux_disp_update() {
    if (enable_updates == 0) {
        return;
    }
    if (millis() - last > 300) {
        last = millis();
#if DEBUG
        Serial.print("len: "); Serial.print(string_len);
        Serial.print("offset: ");Serial.println(aux_disp_offset);
#endif
        if (string_len - aux_disp_offset >= 8) {
            aux_disp_write(aux_disp_buf + aux_disp_offset);
            aux_disp_offset++;
        } else if (string_len <= 8) {
            aux_disp_write(aux_disp_buf);        
        }
    }
}

/**
 * Converts a character to the corresponding bit sequence to
 * send to the 14segments display.
 */
uint16_t getdata(char c) {        
    uint16_t result = 0;
    uint16_t mask =0;
    int i;
    
    switch (toupper(c)) {
        case 0 : 
        case ' ': result = 0b0000000000000000; break;
        case '0': result = 0b0010010000111111; break;
        case '1': result = 0b0000000000000110; break;
        case '2': result = 0b0000000011011011; break;
        case '3': result = 0b0000000010001111; break;
        case '4': result = 0b0001001011100000; break;
        case '5': result = 0b0000000011101101; break;
        case '6': result = 0b0000000011111101; break;
        case '7': result = 0b0000000000000111; break;
        case '8': result = 0b0000000011111111; break;
        case '9': result = 0b0000000011101111; break;
        case 'A': result = 0b0000000011110111; break;
        case 'B': result = 0b0001001010001111; break;
        case 'C': result = 0b0000000000111001; break;
        case 'D': result = 0b0001001000001111; break;
        case 'E': result = 0b0000000001111001; break;
        case 'F': result = 0b0000000001110001; break;
        case 'G': result = 0b0000000010111101; break;
        case 'H': result = 0b0000000011110110; break;
        case 'I': result = 0b0001001000001001; break;
        case 'J': result = 0b0000000000011110; break;
        case 'K': result = 0b0000110001110000; break;
        case 'L': result = 0b0000000000111000; break;
        case 'M': result = 0b0000010100110110; break;
        case 'N': result = 0b0000100100110110; break;
        case 'O': result = 0b0000000000111111; break;
        case 'P': result = 0b0000000011110011; break;
        case 'Q': result = 0b0000100000111111; break;
        case 'R': result = 0b0000100011110011; break;
        case 'S': result = 0b0000000011101101; break;
        case 'T': result = 0b0001001000000001; break;
        case 'U': result = 0b0000000000111110; break;
        case 'V': result = 0b0010010000110000; break;
        case 'W': result = 0b0010100000110110; break;
        case 'X': result = 0b0010110100000000; break;
        case 'Y': result = 0b0001010100000000; break;
        case 'Z': result = 0b0010010000001001; break;
        case '*': result = 0b0011111111000000; break;
        case '-': result = 0b0000000011000000; break;
        case '<': result = 0b0000110000000000; break;
        case '>': result = 0b0010000100000000; break;
        case '#': result = 0b0000000011011100; break;
    }

    return result;
}
