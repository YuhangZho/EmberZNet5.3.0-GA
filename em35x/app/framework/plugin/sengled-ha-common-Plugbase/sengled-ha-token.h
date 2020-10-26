// *******************************************************************
// * sengled-ha-token.h
// *
// *
// * Copyright 2015 by Sengled Corporation. All rights reserved.              
// *******************************************************************

#ifdef DEFINETOKENS

#define CREATOR_ON_OFF_STATUS  (0x0001)
#define CREATOR_THE_FIRST_JOINED_FLAG (0x0002)

// Define the actual token storage information here
DEFINE_BASIC_TOKEN(ON_OFF_STATUS, int8u, 1)

DEFINE_BASIC_TOKEN(THE_FIRST_JOINED_FLAG, int8u, 0x00)

#endif

