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
#define CC_STOMP_A            17
#define CC_STOMP_B            18
#define CC_STOMP_C            19
#define CC_STOMP_D            20

#define CC_EFFECT_X           22
#define CC_EFFECT_MOD         24
#define CC_DELAY              26
#define CC_REVERB             28

#define CC_TAP_TEMPO          30
#define CC_TUNER              31

#define CC_INCREASE_PERF      48
#define CC_DECREASE_PERF      49

#define CC_SLOT_1             50
#define CC_SLOT_2             51
#define CC_SLOT_3             52
#define CC_SLOT_4             53
#define CC_SLOT_5             54

#define CC_SET_DELAY_MIX               68
#define CC_SET_DELAY_FBK               69
#define CC_SET_REVERB_MIX              70
#define CC_SET_REVERB_TIME             71
#define CC_SET_AMPLIFIER_GAIN          72

#define SYSEX_HEADER_SIZE              7

#define SYSEX_FN_PARAM                 0x01
#define SYSEX_FN_M_PARAM               0x02
#define SYSEX_FN_STRING_PARAM          0x03
#define SYSEX_FN_BLOB                  0x04
#define SYSEX_FN_EXT_PARAM             0x06
#define SYSEX_FN_EXT_STRING_PARAM      0x07
#define SYSEX_FN_EFFECT_CONFIG         0x3c
#define SYSEX_FN_ACK                   0x7e

#define SYSEX_FN_REQUEST_PARAM         0x41
#define SYSEX_FN_REQUEST_M_PARAM       0x42
#define SYSEX_FN_REQUEST_STRING        0x43
#define SYSEX_FN_REQUEST_EXT_PARAM     0x46
#define SYSEX_FN_REQUEST_EXT_STRING    0x47

#define STRING_ID_RIG_NAME             0x0001
#define STRING_ID_PERF_NAME            0x4000
#define STRING_ID_SLOT1_NAME           0x4001
#define STRING_ID_SLOT2_NAME           0x4002
#define STRING_ID_SLOT3_NAME           0x4003
#define STRING_ID_SLOT4_NAME           0x4004
#define STRING_ID_SLOT5_NAME           0x4005


#define STOMP_A_PAGE                   50
#define STOMP_B_PAGE                   51
#define STOMP_C_PAGE                   52
#define STOMP_D_PAGE                   53

#define ON_OFF_PARAM                   03


#define PARAM_STOMP_A_TYPE    0x1900
#define PARAM_STOMP_A_STATE   0x1903
#define PARAM_STOMP_B_TYPE    0x1980
#define PARAM_STOMP_B_STATE   0x1983
#define PARAM_STOMP_C_TYPE    0x1A00
#define PARAM_STOMP_C_STATE   0x1A03
#define PARAM_STOMP_D_TYPE    0x1A80
#define PARAM_STOMP_D_STATE   0x1A83

#define PARAM_STOMP_X_TYPE    0x1C00
#define PARAM_STOMP_X_STATE   0x1C03
#define PARAM_STOMP_MOD_TYPE  0x1D00
#define PARAM_STOMP_MOD_STATE 0x1D03
#define PARAM_DELAY_STATE     0x2502
#define PARAM_REVERB_STATE    0x2582

void handle_sysex(byte * data, byte len);
void handle_sense(void);
void kemper_process(void);

struct kpa_state_ {
    int tune;                /* Holds the current tune value */
    char * note;             /* Holds the current tunner note */
    char stomps[17];         /* Used to display the state changes of the stomps */
    char perf_name[20];      /* Name of the current rig */
    char slot_name[32];      /* Name of the current performance */
    uint8_t effects;         /* Current state of the stomps */
    uint8_t showing_rig:1;   /* Used to indicate the rig name when rig is switched */
    uint8_t enabled_slots:7; /* Used to indicated wich slots are defined */
};

#endif
