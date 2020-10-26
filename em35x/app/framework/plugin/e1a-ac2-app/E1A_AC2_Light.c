#include "E1A_AC2_App.h"
#include "E1A_AC2_Public_Head.h"

void SetLedDimming(int8u level);

//****************************************************************
//on off Control
//****************************************************************
void emberAfOnOffClusterServerAttributeChangedCallback(int8u endpoint, 
                                                       EmberAfAttributeId attributeId)
{
  if (ZCL_ON_OFF_ATTRIBUTE_ID == attributeId)
  {
    boolean isOn;

    emberAfReadServerAttribute(endpoint,
                             ZCL_ON_OFF_CLUSTER_ID,
                             ZCL_ON_OFF_ATTRIBUTE_ID,
                             (int8u *)&isOn,
                             sizeof(boolean));
  
    if (isOn)
    {      
      int8u currentLevel;
      emberAfReadServerAttribute(endpoint,
                              ZCL_LEVEL_CONTROL_CLUSTER_ID,
                              ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                              (int8u *)&currentLevel,
                              sizeof(int8u));
      if (currentLevel > DIMMING_MIN_LEVEL)
      {
        SetLedDimming(currentLevel);
        SET_LAMP_ON();
      }
      else
      { SET_LAMP_OFF();}
    }
    else
    { SET_LAMP_OFF();}
  }
}

//******************************************************************
// level control
//******************************************************************
boolean disableLampOn;

// SetLedDimming
// y = -22.7x+5577.1
void SetLedDimming(int8u level)
{
  int32u dimLevel;

  if (level == DIMMING_MAX_LEVEL)
  { dimLevel = 0;}
  else if (level == 0)
  { dimLevel = TICS_PER_PERIOD;}
  else
  {
    dimLevel = (int32u)level;
    dimLevel = dimLevel*227;
    if (55771 >= dimLevel)
    { dimLevel = (55771-dimLevel)/10;}
    else
    { dimLevel = 0;}
  }

  if (dimLevel > TICS_PER_PERIOD)
  { dimLevel = TICS_PER_PERIOD;}

  powerCount = POWER_COUNT_MAX;  //init power count

  //emberAfGuaranteedPrint("%d\r\n", dimLevel);
  SetPwmLevel((int16u)dimLevel, DIMMING_CTRL_PIN);  
}
//
void emberAfLevelControlClusterServerAttributeChangedCallback(int8u endpoint,
                                                              EmberAfAttributeId attributeId)
{
  if (ZCL_CURRENT_LEVEL_ATTRIBUTE_ID == attributeId)
  {
    boolean isOn;    
    
    emberAfReadServerAttribute(endpoint,
                             ZCL_ON_OFF_CLUSTER_ID,
                             ZCL_ON_OFF_ATTRIBUTE_ID,
                             (int8u *)&isOn,
                             sizeof(boolean));

    
    if (isOn)    
    {
      int8u currentLevel;
      emberAfReadServerAttribute(endpoint,
                              ZCL_LEVEL_CONTROL_CLUSTER_ID,
                              ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                              (int8u *)&currentLevel,
                              sizeof(int8u));

      SetLedDimming(currentLevel);

      if (currentLevel > DIMMING_MIN_LEVEL) 
      { 
        if (disableLampOn == FALSE)
        { SET_LAMP_ON();}
      }
      else
      { SET_LAMP_OFF();}
    }
  }
}

//******************************************************************
// LightInit
//******************************************************************
void SetLedInitialValue(void)
{
  int8u onLevel, currentLevel;
  int8u reset = halGetResetInfo();
  //int16u extendedResetInfo = halGetExtendedResetInfo();
  boolean onOff;

  // onOff attribute
  //if (extendedResetInfo == 0x0301) //zheng chang shang dian 
  if ((0x03==reset) || (0x04==reset))
  { onOff = 1;}
  else
  { emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
                               ZCL_ON_OFF_CLUSTER_ID,
                               ZCL_ON_OFF_ATTRIBUTE_ID,
                               (int8u *)&onOff,
                               sizeof(boolean));    
  }

  // current level attribute and onLevel attribute
  emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
                              ZCL_LEVEL_CONTROL_CLUSTER_ID,
                              ZCL_ON_LEVEL_ATTRIBUTE_ID,
                              (int8u *)&onLevel,
                              sizeof(int8u));

  disableLampOn = TRUE;
  if (0xff != onLevel)  // onLevel is active
  {
    if (onLevel <= DIMMING_MIN_LEVEL)
    {
      onLevel = DIMMING_MAX_LEVEL;
      emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                         ZCL_LEVEL_CONTROL_CLUSTER_ID,
                         ZCL_ON_LEVEL_ATTRIBUTE_ID,
                         (int8u *)&onLevel,
                         ZCL_INT8U_ATTRIBUTE_TYPE);      
    }

    emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                         ZCL_LEVEL_CONTROL_CLUSTER_ID,
                         ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                         (int8u *)&onLevel,
                         ZCL_INT8U_ATTRIBUTE_TYPE);
  }
  else
  {
    emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
                            ZCL_LEVEL_CONTROL_CLUSTER_ID,
                            ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                            (int8u *)&currentLevel,
                            sizeof(int8u));
    
    if (currentLevel <= DIMMING_MIN_LEVEL)  
    { 
      currentLevel = DIMMING_MAX_LEVEL;
      emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                           ZCL_LEVEL_CONTROL_CLUSTER_ID,
                           ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                           (int8u *)&currentLevel,
                           ZCL_INT8U_ATTRIBUTE_TYPE);
    }
  }
  disableLampOn = FALSE;  

  // on-off
  emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                               ZCL_ON_OFF_CLUSTER_ID,
                               ZCL_ON_OFF_ATTRIBUTE_ID,
                               (int8u *)&onOff,
                               ZCL_BOOLEAN_ATTRIBUTE_TYPE);
}


