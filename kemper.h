#ifndef __KEMPER_H_
#define __KEMPER_H_

/* Request rig name:                                 */
/* F0 00 20 33 02 7f 43 00 00 01 F7                  */
/* | Header        |  |  |                           */
/* |               |  |  | ID  |   0001 ==> rig name */
/* |               |  | allways 00                   */
/* |               | Fn (single param request)       */
/*
16 Any Toggles all Stomps between On and Off setting.
       Select Type “Empty” to disable a slot completely.
17     0: Stomp A Off
       1: Stomp A On
18     0: Stomp B Off
       1: Stomp B On
19     0: Stomp C Off
       1: Stomp C On
20     0: Stomp D Off
       1: Stomp D On
22     0: Effect X Off
       1: Effect X On
24     0: Effect Mod Off
       1: Effect Mod On
26     0: Delay Off
       1: Delay On
27 x   Sets Delay Mix to x
28     0: Reverb Off
       1: Reverb On
29 x   Sets Reverb Mix to x
30 1/0 Sets Tempo Tap
31 1/0 1: Show Tuner
       0: Hide Tuner
33 1/0 0: Rotary Speaker slow
       1: Rotary Speaker fast

(Performance Mode only)
-----------------------
48 1/0 Increases performance index. Value triggers two different modes:
        Value 0, increase performance n to n+1
        Value 1 initially increases performance by 1, after a timeout the
         KPA starts to scroll performances upwards. Value 0 stops scrolling.

49 1/0 Decrease performance index. Value triggers two different modes:
        Value 0, increase performance n to n-1
        Value 1 initially decreases performance index by 1, after a timeout
         the KPA starts to scroll performances downwards. Value 0 stops
         scrolling.

50 1 Select Slot 1 of current performance.
51 1 Select Slot 2 of current performance.
52 1 Select Slot 3 of current performance.
53 1 Select Slot 4 of current performance.
54 1 Select Slot 5 of current performance.

68 x Sets Delay Mix to x
69 x Sets Delay Feedback to x
70 x Sets Reverb Mix to x
71 x Sets Reverb Time to x
72 x Sets Amplifier Gain to x
*/

#define CC_INCREASE_PERF      48
#define CC_DECREASE_PERF      49

#define CC_SLOT_1             50
#define CC_SLOT_2             51
#define CC_SLOT_3             52
#define CC_SLOT_4             53
#define CC_SLOT_5             54

#define CC_SET_DELAY_MIX      68
#define CC_SET_DELAY_FBK      69
#define CC_SET_REVERB_MIX     70
#define CC_SET_REVERB_TIME    71
#define CC_SET_AMPLIFIER_GAIN 72

#define SYSEX_HEADER_SIZE 7

#define SYSEX_FN_PARAM                 0x01
#define SYSEX_FN_M_PARAM               0x02
#define SYSEX_FN_STRING_PARAM          0x03
#define SYSEX_FN_BLOB                  0x04
#define SYSEX_FN_EXT_PARAM             0x06
#define SYSEX_FN_EXT_STRING_PARAM      0x07

#define SYSEX_FN_REQUEST_PARAM         0x41
#define SYSEX_FN_REQUEST_M_PARAM       0x42
#define SYSEX_FN_REQUEST_STRING        0x43
#define SYSEX_FN_REQUEST_EXT_STRING    0x43

#define STRING_ID_RIG_NAME           0x0001

#define STOMP_A_PAGE                   50
#define STOMP_B_PAGE                   51
#define STOMP_C_PAGE                   52
#define STOMP_D_PAGE                   53

#define ON_OFF_PARAM                   03

#endif
