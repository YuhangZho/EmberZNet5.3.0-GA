


/**
* Custom Application Tokens
*/
// Define token names here
#define CREATOR_THE_GATEWAY_TYPE (0x0001)
#define CREATOR_THE_APSNUM_TYPE  (0x0002)

#ifdef DEFINETOKENS
// Define the actual token storage information here
DEFINE_BASIC_TOKEN(THE_GATEWAY_TYPE, boolean, FALSE)
DEFINE_BASIC_TOKEN(THE_APSNUM_TYPE, int8u, 0)
#endif