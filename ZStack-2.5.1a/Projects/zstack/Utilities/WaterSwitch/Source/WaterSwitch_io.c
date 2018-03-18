#include <string.h>
#include "WaterSwitch.h"
#include "WaterSwitch_io.h"
#include "MT_UART.h"
#include "MT_APP.h"
#include "MT.h"


#if DEVICE_TYPE==WS_COORDINATOR || DEVICE_TYPE==WS_PUMP
void TurnOnOffValve(uint8 onoff){
#ifdef DEBUG
      uchar strTemp[40];  
      sprintf(strTemp, "Start drive valve\r\n");
      HalUARTWrite(1, strTemp, strlen(strTemp));
#endif
  //Decide the direction
  PUMP_DIRECTION=ACTIVE_HIGH(onoff);
  //Output power
  PUMP_POWER=1;
  //Start timer to stop the valve driver
  osal_stop_timerEx( WaterSwitch_TaskID, WATERSWITCH_VALVE_SERVICE_EVT );
  osal_start_timerEx( WaterSwitch_TaskID, WATERSWITCH_VALVE_SERVICE_EVT, WATERSWITCH_VALVE_TIMEOUT );  
} 

void StopValueOutput(void){
#ifdef DEBUG
      uchar strTemp[40];  
      sprintf(strTemp, "Stop drive valve\r\n");
      HalUARTWrite(1, strTemp, strlen(strTemp));
#endif
  //Stop power output
  PUMP_POWER=0;
  //Stop direction output
  PUMP_DIRECTION=0;
}
#endif