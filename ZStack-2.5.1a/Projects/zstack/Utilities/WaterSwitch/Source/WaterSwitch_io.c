#include <string.h>
#include "WaterSwitch.h"
#include "WaterSwitch_io.h"
#include "MT_UART.h"
#include "MT_APP.h"
#include "MT.h"


#if DEVICE_TYPE==WS_COORDINATOR || DEVICE_TYPE==WS_PUMP
void TurnOnOffValve(uint8 onoff){
  //Decide the direction
  PUMP_DIRECTION=ACTIVE_HIGH(onoff);
  
  LOG_OUTPUT(LOG_INFO,  "Start drive valve\r\n");
  //Output power
  PUMP_POWER=ACTIVE_HIGH(PUMP_ON);
  //Start timer to stop the valve driver
  osal_stop_timerEx( WaterSwitch_TaskID, WATERSWITCH_VALVE_SERVICE_EVT );
  osal_start_timerEx( WaterSwitch_TaskID, WATERSWITCH_VALVE_SERVICE_EVT, WATERSWITCH_VALVE_TIMEOUT );  
} 

void StopValueOutput(void){
  //Stop power output
  PUMP_POWER=ACTIVE_HIGH(PUMP_OFF);
  
  LOG_OUTPUT(LOG_INFO,  "Stop drive valve\r\n");
  //Stop direction output
  PUMP_DIRECTION=ACTIVE_HIGH(PUMP_OFF);
#if DEVICE_TYPE==WS_PUMP
  //If the salor is on
  if(zclWATERSWITCH_OnOff == PUMP_ON){
    //Then we can turn on the pump
    canTurnOnPump = 1;
  }
#endif
}
#endif