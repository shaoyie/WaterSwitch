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
int canTurnOnPump=0;
int drivePump=0;
uint32 startWaterUsingCheckTime=0;

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
  
  P0INP &= ~(PUMP_POWER_BV|PUMP_DIRECTION_BV|PUMP_SWITCH_BV);    /*pull-up/pull-down*/
  P0SEL &= ~(PUMP_POWER_BV|PUMP_DIRECTION_BV|PUMP_SWITCH_BV);    /* Set pin function to GPIO */
  P0DIR|= (PUMP_POWER_BV|PUMP_DIRECTION_BV|PUMP_SWITCH_BV);   /* Set pin direction to Output */
  
  //Clear the output
  PUMP_POWER = 0;
  PUMP_DIRECTION = 0;
  PUMP_SWITCH = 0;
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
  
  // Turn on
  if ( cmd == COMMAND_ON ) {
    zclWATERSWITCH_OnOff = PUMP_ON;
  }
  // Turn off
  else if ( cmd == COMMAND_OFF ){
    zclWATERSWITCH_OnOff = PUMP_OFF;
    canTurnOnPump = 0;
  }
  // Toggle the light
  else
  {
    //We don't have this case
  }
  LOG_OUTPUT(LOG_DEBUG, "Pump: %d\r\n", zclWATERSWITCH_OnOff);
  TurnOnOffValve(zclWATERSWITCH_OnOff);
}

void RegularTask( void )
{
  //Send flow report
  zclWATERSWITCH_Flow = ACTIVE_HIGH(WATER_USING_DETECT);
  SendFlowReport();
}

void waterFlowMeterTrigger(){
  
  //Turn on the pump
  if(zclWATERSWITCH_OnOff == PUMP_ON&&canTurnOnPump
     //&& now - lastTurnOffTime>10000
     ){
       
       if(drivePump){
         //If we are driving pump, then keep the pump on and delay shut down the pump
         PUMP_SWITCH = 1;
         //LOG_OUTPUT(LOG_DEBUG, "Pump turned on\n\r");
         osal_stop_timerEx( WaterSwitch_TaskID, WATERSWITCH_TURN_OFF_PUMP_EVT );
         osal_start_timerEx( WaterSwitch_TaskID,
                            WATERSWITCH_TURN_OFF_PUMP_EVT,
                            WATERSWITCH_DELAY_TIMEOUT*1.5);
       } else if(ACTIVE_HIGH(WATER_USING_DETECT)){
         //The water flow is big enough, so we needn't turn on
         //Do nothing
         
       } else {
         uint32 now=osal_GetSystemClock();
         //Triggered, and the pump is not on, we need to prepare for work
         if(startWaterUsingCheckTime==0||now-startWaterUsingCheckTime>4000){
           //Not start check or the check time is too long ago, then let's start to check
           startWaterUsingCheckTime=now;
           LOG_OUTPUT(LOG_DEBUG, "Start check water using\n\r");
         } else if(now-startWaterUsingCheckTime<3000){
           //Debounce 3 seconds
           //Do nothing
         } else {
           //Turn on the pump
           drivePump=1;
           LOG_OUTPUT(LOG_DEBUG, "Pump turned on\n\r");
         }
       }
     }
}


void turnOffPump(){
  
  PUMP_SWITCH = 0;
  drivePump = 0;
  startWaterUsingCheckTime=0;
  //Debounce, avoid the shut down water flow trigger on the pump again
  LOG_OUTPUT(LOG_DEBUG, "Pump turned off\n\r");
}
#endif