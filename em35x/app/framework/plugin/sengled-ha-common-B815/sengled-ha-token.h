// *******************************************************************
// * sengled-ha-token.h
// *
// *
// * Copyright 2015 by Sengled Corporation. All rights reserved.              
// *******************************************************************
//#define CREATOR_THE_GATEWAY_TYPE (0x0001)

#ifdef DEFINETOKENS

#define CREATOR_THE_APSNUM_TYPE  (0x0001)
#define CREATOR_THE_FIRST_JOINED_FLAG (0x0002)

//#define CREATOR_AUTO_RESET_MARK (0x0010)



// Define the actual token storage information here
//DEFINE_BASIC_TOKEN(THE_GATEWAY_TYPE, boolean, FALSE)
DEFINE_BASIC_TOKEN(THE_FIRST_JOINED_FLAG, int8u, 0x00)
DEFINE_BASIC_TOKEN(THE_APSNUM_TYPE, int8u, 0)

//DEFINE_BASIC_TOKEN(AUTO_RESET_MARK, int8u, 0x00)

#endif

