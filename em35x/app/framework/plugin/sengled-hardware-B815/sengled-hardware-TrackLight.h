// *****************************************************************************
// * sengled-hardware-TrackLight.h
// *
// * This code provides support for managing the hardware for TrackLight
// *
// * Copyright 2015 by Sengled Corporation. All rights reserved.              
// *****************************************************************************

#define BUTTON0_PRINT "button0 press"
#define BUTTON1_PRINT "button1 press"

#define DOUBLE_CLICK BUTTON0 
#define DIMMER_DETECT BUTTON1 

#define DOUBLE_CLICK_OFF BUTTON_RELEASED
#define DOUBLE_CLICK_ON  BUTTON_PRESSED

#ifdef  SUPPORT_DOUBLE_CLICK
#define BREATH_START         1
#define BREATH_END           2
#define REJOIN               5
#define EZMODE               7
#define RESETF               10

#else
#define RESETF               5	// according Eric product change 0718
#endif

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

#define PREVENT_SIGNAL_SHAKING_TIME 30
