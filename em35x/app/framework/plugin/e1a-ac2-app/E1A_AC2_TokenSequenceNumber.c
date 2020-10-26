
#include "E1A_AC2_Public_Head.h"


//****************************************************************
// token
//****************************************************************
void SengledSetSequenceNumber(int8u num);


void GetTheApsNumType(void)
{ 
  int8u num;

  halCommonGetToken(&num, TOKEN_THE_APSNUM_TYPE);
  SengledSetSequenceNumber(num+1);
}
void SetTheApsNumType(int8u num)
{ 
  halCommonSetToken(TOKEN_THE_APSNUM_TYPE, &num);
}


