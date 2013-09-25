pedalboard
==========


The board:
<pre>
 _____________________________________
|     _______              _______    |
|    |_______|            |_______|   |
|                                     |
|    o      o      o      o      o    |
|   11     12     13     14     15    |
|                                     |
|    o      o      o      o      o    |
|    6      7      8      9     10    |
|                                     |
|    o      o      o      o      o    |
|    1      2      3      4      5    |
|_____________________________________|
</pre>

   * Left display is a 2x16 LCD 

     For configuration purpose

     This display is connected throught an I2C expander chip.

     The prototype is using a snootlab deuligne for now.

   * Right display is a 1x8 14digit Led display.

     For patch name display / Tuner display

     This display is connected with 3 GPIO in a Synchronous serial mode.

   * Buttons

     Arranged in a 3x15 matrix and uses Keypad library.

   * One Led per button.

     Connected throuh 595 shift registers. 

     Use same SCLK / SDATA as the 14 digit led display with a second latch gpio.



