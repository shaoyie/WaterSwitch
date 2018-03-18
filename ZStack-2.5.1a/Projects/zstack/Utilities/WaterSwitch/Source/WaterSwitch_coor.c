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


#if DEVICE_TYPE==WS_COORDINATOR

uint32 tick=0;
uint32 lastTempTick=0;
uint32 lastPumpTick=0;
uint32 lastFireOnTick=0;
uint16 waterEntering=0;
uint16 salorWaterUsing=0;
uint8 fireTurnedOn=0;
uint8 fireUsing=0;
uint8 fireOperation=0;

void HandelFireOperationEvents(void);
void SelectWaterSupplier(uint8 supplier);


void WaterSwitch_InitIO(void){
  
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
  
  //Clear the output
  FIRE_TEMP_UP = 0;
  PUMP_POWER = 0;
  PUMP_DIRECTION = 0;
  FIRE_SWITCH = 0;
  
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
  
#if 0
  // Turn on
  if ( cmd == COMMAND_ON )
    zclSampleLight_OnOff = LIGHT_ON;

  // Turn off
  else if ( cmd == COMMAND_OFF )
    zclSampleLight_OnOff = LIGHT_OFF;

  // Toggle the light
  else
  {
    if ( zclSampleLight_OnOff == LIGHT_OFF )
      zclSampleLight_OnOff = LIGHT_ON;
    else
      zclSampleLight_OnOff = LIGHT_OFF;
  }
#endif
#ifdef DEBUG  
  sprintf(strTemp, "Pump: %d\r\n", zclWATERSWITCH_OnOff);
  HalUARTWrite(1, strTemp, strlen(strTemp));
#endif
  //TurnOnOffValve(zclWATERSWITCH_OnOff);
}

/*********************************************************************
* @fn      zclWATERSWITCH_ProcessInReportCmd
*
* @brief   Process the "Profile" Report Command
*
* @param   pInMsg - incoming message to process
*
* @return  none
*/
uint8 zclWATERSWITCH_ProcessInReportCmd( zclIncomingMsg_t *pInMsg )
{
  zclReportCmd_t *reportCmd;
  zclReport_t *reportRec;
  uint8 i;
  uint8 *OnOffState;
  uint16 *pdata;
  reportCmd = (zclReportCmd_t *)pInMsg->attrCmd;
  for (i = 0; i < reportCmd->numAttr; i++)
  {
    // Device is notified of the latest values of the attribute of another device.
    reportRec = &(reportCmd->attrList[i]);
    
    if(pInMsg->clusterId==ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT && 
       reportRec->attrID == ATTRID_MS_TEMPERATURE_MEASURED_VALUE){
         
#ifdef CAPTURE_RAW_DATA
         HalUARTWrite(1, "	1	", 3);
#endif
         
         pdata = (uint16 *)reportRec->attrData;
         zclWATERSWITCH_Temp = *pdata;
         lastTempTick = tick;
         
       } else if(pInMsg->clusterId==ZCL_CLUSTER_ID_MS_OCCUPANCY_SENSING && 
       reportRec->attrID == ATTRID_MS_OCCUPANCY_SENSING_CONFIG_OCCUPANCY){
#ifdef CAPTURE_RAW_DATA
         HalUARTWrite(1, "	2	", 3);
#endif
         
         pdata = (uint16 *)reportRec->attrData;
         zclWATERSWITCH_Occupancy = *pdata;
         
       } else if(pInMsg->clusterId==ZCL_CLUSTER_ID_MS_FLOW_MEASUREMENT && 
       reportRec->attrID == ATTRID_MS_FLOW_MEASUREMENT_MEASURED_VALUE){
#ifdef CAPTURE_RAW_DATA
         HalUARTWrite(1, "	3	", 3);
#endif
         
         pdata = (uint16 *)reportRec->attrData;
         if(pInMsg->srcAddr.addr.shortAddr == WaterSwitch_TempDstAddr.addr.shortAddr){
           //Temp node
           waterEntering = *pdata;
         } else if(pInMsg->srcAddr.addr.shortAddr == WaterSwitch_PumpAddr.addr.shortAddr){
           //Pump node
           salorWaterUsing = *pdata;
           lastPumpTick = tick;
         }
         
       }
#ifdef CAPTURE_RAW_DATA
      uchar strTemp[40];
  
      sprintf(strTemp, "%u", *pdata);
      HalUARTWrite(1, strTemp, strlen(strTemp)); //输出接收到的数据 
      if(pInMsg->clusterId==ZCL_CLUSTER_ID_MS_FLOW_MEASUREMENT){
          HalUARTWrite(1, "\r\n", 2);
      }
#endif
  }
  return TRUE;
}

//Get the specified device's active endpoint list
void ActiveEPReq(uint16 bindAddr){
  zAddrType_t dstAddr;
  
  //uint8 txOptions=0;
  
  dstAddr.addr.shortAddr = bindAddr;//0xFFFE; //bindAddr;
  dstAddr.addrMode = afAddr16Bit;
  
  //Get Request a device's endpoint list
  HalUARTWrite(1,"ZDP_ActiveEPReq\n", sizeof("ZDP_ActiveEPReq\n")); 
  ZDP_ActiveEPReq(&dstAddr, bindAddr, TRUE);
  
  // Send the message
  //zclGeneral_SendOnOff_CmdToggle( WATERSWITCH_ENDPOINT, &dstAddr, false, 0 );
  //zclGeneral_SendIdentifyQuery(WATERSWITCH_ENDPOINT, &dstAddr, false, 0);
  
}

//According to the report sending decide the nodes' status
void CheckNodeStatus(){
  
  if(tick-lastTempTick>2){
    //Temp node is not work
    device_Status |= TEMP_ERROR;
    device_Status &= ~TEMP_WORKING;
  } else if(lastTempTick>0){
    //Temp node is working
    device_Status &= ~TEMP_ERROR;
    device_Status |= TEMP_WORKING;
  }
  
  if(tick-lastPumpTick>2){
    //Temp node is not work
    device_Status |= PUMP_ERROR;
    device_Status &= ~PUMP_WORKING;
  } else if(lastTempTick>0){
    device_Status &= ~PUMP_ERROR;
    device_Status |= PUMP_WORKING;
  }
}

//Check the fire related status
void CheckFireStatus(){
  if(fireTurnedOn){
    if(!FIRE_ON_DETECT && lastFireOnTick - tick>2){
      //It's already turned off
      fireTurnedOn = 0;
    }
  } else if(FIRE_ON_DETECT){
    fireTurnedOn = 1;
  }
  fireUsing = FIRE_USING_DETECT;
}

//Based on the node status and policy to decide the new work mode
uint8 DecideWorkMode(){
  CheckNodeStatus();
  CheckFireStatus();
  
  //Only in auto mode or pending status we need to change the work mode
  if(zclWATERSWITCH_OnOffSwitch == AUTO_CONTROL || zclWATERSWITCH_OnOff == PENDING) {
    //Check whether the solar is good for use
    if((device_Status & PUMP_WORKING) && (device_Status & TEMP_WORKING)){
      if(zclWATERSWITCH_Temp>=60 && zclWATERSWITCH_Occupancy >=3 && !fireUsing){
        //Salor is high and user is not using fire
        return SALOR_ON;
      }
      if(waterEntering && salorWaterUsing){
        //Salor is entering water and user is using it, should use fire
        return SALOR_OFF;
      }
      if((zclWATERSWITCH_Temp<60 || zclWATERSWITCH_Occupancy<=1) && (!salorWaterUsing)){
        //Too cold or too less water, use fire
        return SALOR_OFF;
      }
      if(zclWATERSWITCH_OnOff==SALOR_ON){
        //If it's already solar, remain it
        return zclWATERSWITCH_OnOff;
      }
    }
  } else {
    //return the work mode as is
    return zclWATERSWITCH_OnOff;
  }
  //default use fire
  return SALOR_OFF;
}

#define FIRE_STATE_IDLE 0
#define FIRE_STATE_TO_SET_TEMP 1

static uint8 fireStatus = FIRE_STATE_IDLE;

static void StopFireOperation(){
  //Clear all current operations
  osal_stop_timerEx( WaterSwitch_TaskID, WATERSWITCH_FIRE_OPERATION_EVT );
  FIRE_SWITCH = 0;
  FIRE_TEMP_UP = 0;
  fireOperation = 0;
}

static void PressFireSwitch(){
  StopFireOperation();
  FIRE_SWITCH = 1;
  fireOperation |= KEY_FIRE_SWITCH;
  osal_start_timerEx( WaterSwitch_TaskID,
                               WATERSWITCH_FIRE_OPERATION_EVT,
                               WATERSWITCH_PRESS_KEY_TIMEOUT );
#ifdef DEBUG
      uchar strTemp[40];  
      sprintf(strTemp, "Power pressed");
      HalUARTWrite(1, strTemp, strlen(strTemp));
#endif
}

static void PressTempUpForTime(uint16 timeout){
  StopFireOperation();
  FIRE_TEMP_UP = 1;
  fireOperation |= KEY_FIRE_TEMP_UP;
  osal_start_timerEx( WaterSwitch_TaskID,
                               WATERSWITCH_FIRE_OPERATION_EVT,
                               timeout );
#ifdef DEBUG
      uchar strTemp[40];  
      sprintf(strTemp, "Temp up pressed for %u\r\n", timeout);
      HalUARTWrite(1, strTemp, strlen(strTemp));
#endif
}

//Short press
static void PressTempUp(){
  fireStatus = FIRE_STATE_IDLE;
  PressTempUpForTime(WATERSWITCH_PRESS_KEY_TIMEOUT );
}

//Press 15 seconds
static void LongPressTemUp(){
  PressTempUpForTime(WATERSWITCH_SET_TEMP_TIMEOUT );
}

//Free the keys when set is done, and check whether need to do it again
void HandelFireOperationEvents(void){
#ifdef DEBUG
      uchar strTemp[40];  
#endif
  
  if(fireOperation & KEY_FIRE_SWITCH){
#ifdef DEBUG
      sprintf(strTemp, "Power key released\r\n");
      HalUARTWrite(1, strTemp, strlen(strTemp));
#endif
    //Just pressed the power key
    //Release it
    FIRE_SWITCH=0;
    fireOperation = 0;
    //Wait for set the temp
    fireStatus = FIRE_STATE_TO_SET_TEMP;
    osal_start_timerEx( WaterSwitch_TaskID,
                               WATERSWITCH_FIRE_OPERATION_EVT,
                               WATERSWITCH_FIRE_POWER_DELAY_TIMEOUT);
    return;
  }

  if(fireOperation & KEY_FIRE_TEMP_UP){
#ifdef DEBUG
    sprintf(strTemp, "Temp up released\r\n");
    HalUARTWrite(1, strTemp, strlen(strTemp));
#endif
    FIRE_TEMP_UP=0;
    fireOperation = 0;
    if(fireStatus == FIRE_STATE_TO_SET_TEMP){
      //Finished set the temp, now turn on the pump
      TurnOnOffValve(PUMP_ON);
      if(device_Status & PUMP_WORKING){
        //Turn off the salor pump
        zclGeneral_SendOnOff_CmdOff( WATERSWITCH_ENDPOINT, &WaterSwitch_PumpAddr, false, 0 );
      }
      fireStatus = FIRE_STATE_IDLE;
    }
  }
  
  if(fireStatus == FIRE_STATE_TO_SET_TEMP){
    //We need to set the temp
    LongPressTemUp();
  }
}

//Turn on the fire & watering supplier
static void TurnOnFire(){
  if((!fireTurnedOn) &&(!fireUsing)){
    //Not power on, then turn on it
    PressFireSwitch();
  } else {
    //Otherwise just press the temprature on
    fireStatus = FIRE_STATE_TO_SET_TEMP;
    LongPressTemUp();
  }
}

//Apply the configuration for the given supplier
void SelectWaterSupplier(uint8 supplier){
  if(zclWATERSWITCH_OnOff!=supplier){
    //Changed, so we need to reset
    zclWATERSWITCH_OnOff = supplier;
    
    //Set pump
    if(device_Status & PUMP_WORKING){
      if(zclWATERSWITCH_OnOff == SALOR_ON){
        zclGeneral_SendOnOff_CmdOn( WATERSWITCH_ENDPOINT, &WaterSwitch_PumpAddr, false, 0 );
      }
      //Turn off salor is at the same time as turn on the fire valve
//      else {
//        zclGeneral_SendOnOff_CmdOff( WATERSWITCH_ENDPOINT, &WaterSwitch_PumpAddr, false, 0 );
//      }
    }
    if(zclWATERSWITCH_OnOff == SALOR_ON){
      //Local valve should turn off
      TurnOnOffValve(PUMP_OFF);
    } else {
      //Local valve should turn on
      TurnOnFire();
    }
  } else if(zclWATERSWITCH_OnOff == SALOR_OFF){
    //Fire on, need to keep the fire on
    //Set local/fire
    if((!fireTurnedOn) &&(!fireUsing)){
      //Fire is not turned on
      TurnOnFire();
    } else {
      //Press the temp up button to keep the fire on for each 5 minutes
      if(tick>0 && tick%60==0){
        PressTempUp();
      }
    }
  }
}

void SwitchWorkMode(uint8 workMode){
  zclWATERSWITCH_OnOffSwitch = workMode;
}

//Switch to another work mode
void ToggleWaterSupplier(){
  //Request toggle, then of course it's manual
  SwitchWorkMode(MANUAL_CONTROL);
  SelectWaterSupplier(!zclWATERSWITCH_OnOff);
  UpdateLeds();
}

void ToggleWorkMode(){
  SwitchWorkMode(!zclWATERSWITCH_OnOffSwitch);
  UpdateLeds();
}

void UpdateLeds(){
  static uint16 lastStatus=0;
  //Work mode

  if(zclWATERSWITCH_OnOffSwitch==AUTO_CONTROL){
    HalLedSet( HAL_LED_1, HAL_LED_MODE_ON );
    HalLedSet( HAL_LED_2, HAL_LED_MODE_OFF );
  } else {
    HalLedSet( HAL_LED_2, HAL_LED_MODE_ON );
    HalLedSet( HAL_LED_1, HAL_LED_MODE_OFF );
  }

  if(device_Status&ERROR_MASK){
    
    if(lastStatus!=device_Status){
      lastStatus = device_Status;

      if(device_Status&TEMP_ERROR){
        //The temp node has problem, led 3 blinks
        HalLedBlink (HAL_LED_3, 0, HAL_LED_DEFAULT_DUTY_CYCLE, HAL_LED_DEFAULT_FLASH_TIME);
      } else {
        HalLedSet( HAL_LED_3, HAL_LED_MODE_OFF );
      }
      if(device_Status&PUMP_ERROR){
        //The pump node has problem, led 4 blinks
        HalLedBlink (HAL_LED_4, 0, HAL_LED_DEFAULT_DUTY_CYCLE, HAL_LED_DEFAULT_FLASH_TIME);
      } else {
        HalLedSet( HAL_LED_4, HAL_LED_MODE_OFF );
      }
    }
  } else {
    if(zclWATERSWITCH_OnOff == SALOR_ON){
      HalLedSet( HAL_LED_4, HAL_LED_MODE_ON );
      HalLedSet( HAL_LED_3, HAL_LED_MODE_OFF );
    } else {
      HalLedSet( HAL_LED_3, HAL_LED_MODE_ON );
      HalLedSet( HAL_LED_4, HAL_LED_MODE_OFF );
    }
  }
}

void RegularTask( void )
{
  tick++;
#ifndef CAPTURE_RAW_DATA
  SelectWaterSupplier(DecideWorkMode());
  UpdateLeds();
#endif
}
#endif