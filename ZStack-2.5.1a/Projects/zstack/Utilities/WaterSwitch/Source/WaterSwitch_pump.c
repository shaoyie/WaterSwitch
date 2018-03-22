#include <string.h>

#include "OSAL.h"
#include "AF.h"
#include "ZDApp.h"
#include "ZDObject.h"
#include "ZDProfile.h"

#include "zcl.h"
#include "zcl_general.h"
#include "zcl_ha.h"

#include "WaterSwitch.h"
#include "WaterSwitch_io.h"
#include "DebugTrace.h"

#if !defined( WIN32 )
#include "OnBoard.h"
#endif

/* HAL */
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_key.h"
#include "hal_uart.h"
//#include "sapi.h"
#include "MT_UART.h"
#include "MT_APP.h"
#include "MT.h"

#if DEVICE_TYPE==WS_PUMP

void WaterSwitch_InitIO(void){
  //Input
  /* configure tristates */
  //3-state for input
  //P1 Ports
  P1INP |= (WATER_USING_DETECT_BV);
  P1SEL &= ~(WATER_USING_DETECT_BV);    /* Set pin function to GPIO */
  P1DIR &= ~(WATER_USING_DETECT_BV);    /* Set pin direction to Input */
  
  //Output
  P2INP |= 1<<5;//pull-down for output
  
  P0INP &= ~(PUMP_POWER_BV|PUMP_DIRECTION_BV);    /*pull-up/pull-down*/
  P0SEL &= ~(PUMP_POWER_BV|PUMP_DIRECTION_BV);    /* Set pin function to GPIO */
  P0DIR|= (PUMP_POWER_BV|PUMP_DIRECTION_BV);   /* Set pin direction to Output */
  
  //Clear the output
  PUMP_POWER = 0;
  PUMP_DIRECTION = 0;
}

void CheckPendingTaskCB(){
}

/*********************************************************************
 * @fn      zclSampleLight_OnOffCB
 *
 * @brief   Callback from the ZCL General Cluster Library when
 *          it received an On/Off Command for this application.
 *
 * @param   cmd - COMMAND_ON, COMMAND_OFF or COMMAND_TOGGLE
 *
 * @return  none
 */
void zclWATERSWITCH_OnOffCB( uint8 cmd )
{
#ifdef DEBUG
      uchar strTemp[40];
#endif
   
  // Turn on
  if ( cmd == COMMAND_ON ) {
    zclWATERSWITCH_OnOff = PUMP_ON;
  }
  // Turn off
  else if ( cmd == COMMAND_OFF ){
    zclWATERSWITCH_OnOff = PUMP_OFF;
  }
  // Toggle the light
  else
  {
    //We don't have this case
  }
#ifdef DEBUG
  sprintf(strTemp, "Pump: %d\r\n", zclWATERSWITCH_OnOff);
  HalUARTWrite(1, strTemp, strlen(strTemp));
#endif
  TurnOnOffValve(zclWATERSWITCH_OnOff);
}

void RegularTask( void )
{
  uchar strTemp[40];
  //Send flow report
  zclWATERSWITCH_Flow = ACTIVE_HIGH(WATER_USING_DETECT);
  SendFlowReport();
}
#endif