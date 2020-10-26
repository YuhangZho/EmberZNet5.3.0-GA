#ifndef _E1A_AC2_POWER_DEF_
#define _E1A_AC2_POWER_DEF_


#define POWER_COUNT_MAX 2

extern int8u powerCount;
extern int16u saveCurrent;

void PowerAndConsumptionInit(void);
void AdcControlFunction(void);


#endif


