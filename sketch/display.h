#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include <Arduino.h>
#ifdef __cplusplus
extern "C" {
#endif

void update_leds();
void set_led(int led);
void clear_led(int led);
void set_led_mask(uint16_t mask);
void clear_led_mask(uint16_t mask);
uint16_t get_leds();
void set_leds(uint16_t leds);

void aux_disp_print(char * );
void aux_disp_fixed(char * );
void aux_disp_update(void);
void aux_disp_refresh(void);


void main_disp_print(char pos, char line, char * str);
void main_disp_clear(void);

#ifdef __cplusplus
}
#endif
#endif
