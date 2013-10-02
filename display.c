#include "display.h"
#include "config.h"

uint16_t getdata(char);
uint16_t led_state;
int offset = 0;

#define MSB_FIRST
void shiftout(uint8_t data) {
  int i;
  for (i = 0; i < 8; i++) {
#ifdef MSB_FIRST
        digitalWrite(SERIAL_DATA_OUT, (data & 0x80) ? HIGH : LOW);
        data <<= 1;
#else
        digitalWrite(SERIAL_DATA_OUT, (data & 1) ? HIGH : LOW);
        data >>= 1;
#endif
        digitalWrite(SERIAL_CLOCK, HIGH);
        digitalWrite(SERIAL_CLOCK, HIGH);
        digitalWrite(SERIAL_CLOCK, LOW);
        digitalWrite(SERIAL_CLOCK, LOW);
    }
}


/***********************************************************************/
/******                 LEDS MANAGEMENT                             ****/
/***********************************************************************/

void update_leds() {
    /*latch*/
    digitalWrite(LED_LATCH, HIGH);
    digitalWrite(LED_LATCH, LOW);
}

void set_led(int led) {
    led_state |= (1<<led);    
    update_leds();
}

void clear_led(int led) {
    led_state &= ~(1<<led);
    update_leds();    
}

/***********************************************************************/
/******                 AUX DISPLAY MANAGEMENT                      ****/
/***********************************************************************/

char aux_disp_buf[32];
int aux_disp_offset;
int string_len;
int string_offset;
    static uint32_t last;

void aux_disp_print(char * str) {
    strncpy(aux_disp_buf, str, 32);
    aux_disp_offset = 0;
    string_len = strlen(str);
    string_offset = 0;
    last = millis();
}

void aux_disp_update() {
    int i;

    if (string_len <= 8) {   
        if (aux_disp_offset == 8) return;
        
        if (millis() - last > 80) {
            last = millis();
            aux_disp_offset++;
            if (aux_disp_offset > 8) aux_disp_offset = 8;
        }
    
        for (i = 0; i <= (8-aux_disp_offset); i++) {
            shiftout(0);
            shiftout(0);
        }
        for (i = 0; i < aux_disp_offset; i++) {
            uint16_t tmp = getdata(aux_disp_buf[i]);
            shiftout(tmp >> 8);
            shiftout(tmp & 0xff);    
        }
    } else {
        if (string_len - string_offset == 8) return;
        for (i = 0; i < 8; i++) {
            uint16_t tmp = getdata(aux_disp_buf[i+string_offset]);
            shiftout(tmp >> 8);
            shiftout(tmp & 0xff);    
        }
        if (millis() - last > 200) {
            last = millis();
            string_offset++;
            if (string_len - string_offset < 8)  (string_offset = string_len - 8);
        }
    }
    digitalWrite(AUX_DISP_LATCH, LOW);
    digitalWrite(AUX_DISP_LATCH, HIGH);
    digitalWrite(AUX_DISP_LATCH, LOW);
}

uint16_t getdata(char c) {        
    uint16_t result = 0;
    uint16_t mask =0;
    int i;
    
    switch (c) {
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
    }

    for (i=0; i<(16 -offset); i++) mask |= 1<<i;
    return (result << offset) | (result >> (16-offset)) & mask;
}


