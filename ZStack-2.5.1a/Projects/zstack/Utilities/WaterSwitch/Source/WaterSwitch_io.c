#include "WaterSwitch_io.h"

void WaterSwitch_InitIO(void){
  
#if DEVICE_TYPE==WS_COORDINATOR
  
  //Input
  /* configure tristates */
  //3-state for input
  //P0 Ports
  P0INP |= (FIRE_ON_DETECT_BV);
  P0SEL &= ~(FIRE_ON_DETECT_BV);    /* Set pin function to GPIO */
  P0DIR &= ~(FIRE_ON_DETECT_BV);    /* Set pin direction to Input */
  //P1 Ports
  P1INP |= (FIRE_USING_DETECT_BV);
  P1SEL &= ~(FIRE_USING_DETECT_BV);    /* Set pin function to GPIO */
  P1DIR &= ~(FIRE_USING_DETECT_BV);    /* Set pin direction to Input */
  
  //Output
  P2INP |= 7<<5;//pull-down for output
  
  P0INP &= ~(FIRE_TEMP_UP_BV|PUMP_POWER_BV|PUMP_DIRECTION_BV);    /*pull-up/pull-down*/
  P0SEL &= ~(FIRE_TEMP_UP_BV|PUMP_POWER_BV|PUMP_DIRECTION_BV);    /* Set pin function to GPIO */
  P0DIR|= (FIRE_TEMP_UP_BV|PUMP_POWER_BV|PUMP_DIRECTION_BV);   /* Set pin direction to Output */
  
  P1INP &= ~(FIRE_TEMP_DOWN_BV);    /*pull-up/pull-down*/
  P1SEL &= ~(FIRE_TEMP_DOWN_BV);    /* Set pin function to GPIO */
  P1DIR|= (FIRE_TEMP_DOWN_BV);   /* Set pin direction to Output */
  
  P2INP &= ~(FIRE_SWITCH_BV);    /*pull-up/pull-down*/
  P2SEL &= ~(FIRE_SWITCH_BV);    /* Set pin function to GPIO */
  P2DIR|= (FIRE_SWITCH_BV);   /* Set pin direction to Output */
  
#elif DEVICE_TYPE==WS_PUMP
#elif DEVICE_TYPE==WS_TEMP
    //Input
  /* configure tristates */
  //3-state for input
  //P1 Ports
  P1INP |= (WATER_ENTERING_DETECT_BV);
  P1SEL &= ~(WATER_ENTERING_DETECT_BV);    /* Set pin function to GPIO */
  P1DIR &= ~(WATER_ENTERING_DETECT_BV);    /* Set pin direction to Input */
  
#elif DEVICE_TYPE==WS_GATEWAY
#else
#endif
}
