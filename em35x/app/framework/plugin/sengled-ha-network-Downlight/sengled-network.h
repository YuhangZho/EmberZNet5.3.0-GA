// *******************************************************************
// * sengled-network.h
// *
// *
// * Copyright 2015 by Sengled Corporation. All rights reserved.              
// *******************************************************************

#include "app/framework/include/af.h"

// use SetDelayQS:
// The 'quarter seconds' are actually 256 milliseconds long
// so 3min=180,000 ms / 256 = 703
#define NETWORK_SEARCH_TIME_UPPER_BOUND 703 // 3min

enum{
	NETWORK_SEARCH_STOP = 0,
	NETWORK_SEARCHING   = 1,
};
void JoinNetworkRequest(void);

