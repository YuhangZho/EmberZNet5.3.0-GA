// *****************************************************************************
// * sengled-hardware-TrackLight.h
// *
// * This code provides support for managing the hardware for Plugbase
// *
// * Copyright 2015 by Sengled Corporation. All rights reserved.              
// *****************************************************************************
#define DOUBLE_CLICK BUTTON0 
#define DIMMER_DETECT BUTTON1 

#define DOUBLE_CLICK_OFF BUTTON_RELEASED
#define DOUBLE_CLICK_ON  BUTTON_PRESSED

#define BUTTON_PRESSED 1
#define BUTTON_RELEASED 0

#define SINGLE_PRESS_LOWER_BOUND 0
#define SINGLE_PRESS_UPPER_BOUND 2  // <=1s we think it is a single press

#define REJOIN_LOWER_BOUND		 6  
#define REJOIN_UPPER_BOUND		 10 // > 3 && <= 5 s rejoin

#define RESET_PRESS_LOWER_BOUND  16 
#define RESET_PRESS_UPPER_BOUND  30 // > 8 <= 15s reset

#define EZ_MODE_LOWER_BOUND		 40 // > 20s ezmode
#define EZ_MODE_UPPER_BOUND      0xff 

#define ENCRYPTION 1 // 1 means do encryption for rejoin

#define NO_EVENT 0
#define REJOIN_EVENT_BLINK 1
#define RESET_EVENT_BLINK  2

#define REJOIN_BLINK_TIMES 4    // rejoin blink twice
#define RESET_BLINK_TIMES  10

#define PREVENT_SIGNAL_SHAKING_TIME 50