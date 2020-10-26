// *****************************************************************************
// * sengled-hardware-A19EUE27.h
// *
// * This code provides support for managing the hardware for CIA19.
// *
// * Copyright 2015 by Sengled Corporation. All rights reserved.              
// *****************************************************************************

#define BUTTON0_PRINT "button0 press"
#define BUTTON1_PRINT "button1 press"

//#define ONE_LINE_POWER_AVOID

#define DOUBLE_CLICK BUTTON0 
#define DIMMER_DETECT BUTTON1 

#define DOUBLE_CLICK_OFF BUTTON_PRESSED
#define DOUBLE_CLICK_ON  BUTTON_RELEASED

#define BREATH_START         1
#define BREATH_END           2
#define REJOIN               5
#define EZMODE               7
#define RESETF               10

// whenever the current level stay on, first up to 100%, count++
// then down to 1%, count++ , and then while...
// so 3 means from x% to 100%, and then from 100% to 1%, and then from 1% to 100%
#define BREATH_TOTAL_TIMES   3 
#define BREATH_UP            1
#define BREATH_DOWN          0

#define ERROR_BLINK          2
#define REJOIN_BLINK         4   //  off ->on->off->on  total 4
#define NETWORK_JOINED_BLINK 6
// reboot will cause one more "blink", so we should blink 9 times 
// off->on->off->on->off->on->off->on->off
#define RESETF_BLINK         10    
