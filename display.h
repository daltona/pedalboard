#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include <Arduino.h>
#ifdef __cplusplus
extern "C" {
#endif

void update_leds();
void set_led(int led);
void clear_led(int led);


void aux_disp_print(char * );
void aux_disp_update(void);
#ifdef __cplusplus
}
#endif
#endif
