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
#include "hal_adc.h"
//#include "sapi.h"
#include "MT_UART.h"
#include "MT_APP.h"
#include "MT.h"


#if DEVICE_TYPE==WS_COORDINATOR

uint32 tick=0;
uint32 lastTempTick=0;
uint32 lastPumpTick=0;
uint16 waterEntering=0;
uint16 salorWaterUsing=0;
uint8 fireTurnedOn=0;
uint8 fireUsing=0;
uint8 fireOperation=0;

//Get/set water temperature
#define GET_WATER_TEMP (zclWATERSWITCH_Temp&0xff)
#define SET_WATER_TEMP(x) (zclWATERSWITCH_Temp&=0xff00, zclWATERSWITCH_Temp|=(x&0xff))
//Get/set environment temperature
#define GET_ENV_TEMP ((zclWATERSWITCH_Temp>>8)&0xff)
#define SET_ENV_TEMP(x) (zclWATERSWITCH_Temp&=0xff, zclWATERSWITCH_Temp|=((x)<<8))

static int8 envTemp=0;

void HandelFireOperationEvents(void);
void SelectWaterSupplier(uint8 supplier);

static void ReportStatus();


void WaterSwitch_InitIO(void){
  
  //Input
  /* configure tristates */
  //3-state for input
  //P0 Ports
  //  P0INP |= (FIRE_ON_DETECT_BV);
  //  P0SEL &= ~(FIRE_ON_DETECT_BV);    /* Set pin function to GPIO */
  //  P0DIR &= ~(FIRE_ON_DETECT_BV);    /* Set pin direction to Input */
  //  //P1 Ports
  //  P1INP |= (FIRE_USING_DETECT_BV);
  //  P1SEL &= ~(FIRE_USING_DETECT_BV);    /* Set pin function to GPIO */
  //  P1DIR &= ~(FIRE_USING_DETECT_BV);    /* Set pin direction to Input */
  //  
  //  PICTL &= ~(FIRE_ON_DETECT_BV);    /* Clear the edge bit */
  //
  //  P0IEN |= FIRE_ON_DETECT_BV;
  //  P0IFG = ~(FIRE_ON_DETECT_BV);
  
  //Output
  P2INP |= 7<<5;//pull-down for output
  
  P0INP &= ~(FIRE_TEMP_UP_BV|PUMP_POWER_BV|PUMP_DIRECTION_BV);    /*pull-up/pull-down*/
  P0SEL &= ~(FIRE_TEMP_UP_BV|PUMP_POWER_BV|PUMP_DIRECTION_BV);    /* Set pin function to GPIO */
  P0DIR|= (FIRE_TEMP_UP_BV|PUMP_POWER_BV|PUMP_DIRECTION_BV);   /* Set pin direction to Output */
  
  P2INP &= ~(FIRE_SWITCH_BV);    /*pull-up/pull-down*/
  P2SEL &= ~(FIRE_SWITCH_BV);    /* Set pin function to GPIO */
  P2DIR|= (FIRE_SWITCH_BV);   /* Set pin direction to Output */
  
  //Clear the output
  FIRE_TEMP_UP = ACTIVE_HIGH(RELEASE_KEY);
  FIRE_SWITCH = ACTIVE_HIGH(RELEASE_KEY);
  PUMP_POWER = ACTIVE_HIGH(PUMP_OFF);
  PUMP_DIRECTION = ACTIVE_HIGH(PUMP_OFF);
  
}

void CheckPendingTaskCB(){
  //Turn on/off valve
  if(pendingTask & TURN_ON_OFF_VALVE){
    //Send again
    if(zclWATERSWITCH_OnOff == SALOR_ON){
      zclGeneral_SendOnOff_CmdOn( WATERSWITCH_ENDPOINT, &WaterSwitch_PumpAddr, false, 0 );
    } else {
      zclGeneral_SendOnOff_CmdOff( WATERSWITCH_ENDPOINT, &WaterSwitch_PumpAddr, false, 0 );
    }
    CheckPendingTask(TURN_ON_OFF_VALVE);
  }
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
  if ( cmd == COMMAND_ON && zclWATERSWITCH_OnOff==SALOR_OFF) {
    ToggleWaterSupplier();
  }
  // Turn off
  else if ( cmd == COMMAND_OFF  && zclWATERSWITCH_OnOff==SALOR_ON){
    ToggleWaterSupplier();
  } else if( cmd == COMMAND_TOGGLE )
  {
    ToggleWaterSupplier();
  }
  LOG_OUTPUT(LOG_INFO,  "Salor: %d\r\n", zclWATERSWITCH_OnOff);
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
#ifdef CAPTURE_RAW_DATA
  static char str[20];
  char strPri[10];
#endif
  for (i = 0; i < reportCmd->numAttr; i++)
  {
    // Device is notified of the latest values of the attribute of another device.
    reportRec = &(reportCmd->attrList[i]);
    
    if(pInMsg->clusterId==ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT && 
       reportRec->attrID == ATTRID_MS_TEMPERATURE_MEASURED_VALUE){
         
#ifdef CAPTURE_RAW_DATA
         str[0]=0;
         strcat(str, "	1	");
#endif
         
         pdata = (uint16 *)reportRec->attrData;
         SET_WATER_TEMP(*pdata);
         lastTempTick = tick;
         
       } else if(pInMsg->clusterId==ZCL_CLUSTER_ID_MS_OCCUPANCY_SENSING && 
                 reportRec->attrID == ATTRID_MS_OCCUPANCY_SENSING_CONFIG_OCCUPANCY){
#ifdef CAPTURE_RAW_DATA
                   strcat(str, "	2	");
#endif
                   
                   pdata = (uint16 *)reportRec->attrData;
                   zclWATERSWITCH_Occupancy = *pdata;
                   
                 } else if(pInMsg->clusterId==ZCL_CLUSTER_ID_MS_FLOW_MEASUREMENT && 
                           reportRec->attrID == ATTRID_MS_FLOW_MEASUREMENT_MEASURED_VALUE){
                             
                             pdata = (uint16 *)reportRec->attrData;
                             if(pInMsg->srcAddr.addr.shortAddr == WaterSwitch_TempDstAddr.addr.shortAddr){
                               //Temp node
                               waterEntering = *pdata;
                             } else if(pInMsg->srcAddr.addr.shortAddr == WaterSwitch_PumpAddr.addr.shortAddr){
#ifdef CAPTURE_RAW_DATA
                               str[0]=0;
#endif
                               //Pump node
                               salorWaterUsing = *pdata;
                               lastPumpTick = tick;
                             }
#ifdef CAPTURE_RAW_DATA
                             strcat(str, "	3	");
#endif
                             
                           }
#ifdef CAPTURE_RAW_DATA  
    sprintf(strPri, "%u", *pdata);
    strcat(str, strPri);
    if(pInMsg->clusterId==ZCL_CLUSTER_ID_MS_FLOW_MEASUREMENT){
      strcat(str, "\r\n");
      INFO_OUTPUT( str, strlen(str)); //输出接收到的数据 
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
  //INFO_OUTPUT("ZDP_ActiveEPReq\n", strlen("ZDP_ActiveEPReq\n")); 
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
  uint32 currenttime = osal_GetSystemClock();
  uint32 p0_gap = currenttime - p0_0_time;
  uint32 p1_gap = currenttime - p1_3_time;
  
  //Trigger in 3 seconds take it as on
  if(p0_gap < 3000){
    fireTurnedOn = 1;
  } else {
    fireTurnedOn = 0;
    p0_0_time = currenttime - 5000; //Avoid rolled up
  }
  if(p1_gap < 3000){
    fireUsing = 1;
  } else {
    fireUsing = 0;
    p1_3_time = currenttime - 5000; //Avoid rolled up
  }
  
  //  sprintf(strTemp, "lastTime:%lu, nowTime:%lu, diff: %lu\n\r", lastTimeStamp, now, now-lastTimeStamp);
  //  INFO_OUTPUT(strTemp, strlen(strTemp)); 
  //  lastTimeStamp = now;
  //  sprintf(strTemp, "lastCount:%lu, nowCount:%lu, diff: %lu\n\r", lastCount, currenttime, currenttime-lastCount);
  //  lastCount = currenttime;
  //  INFO_OUTPUT(strTemp, strlen(strTemp)); 
  LOG_OUTPUT(LOG_DEBUG,  "fire on:%d, using: %d\n\r", fireTurnedOn, fireUsing);
}

void GetEnvTemp(){
  envTemp = (ReadAdcValue()>>4)/4.5-587.5 + zclWATERSWITCH_IdentifyTime;
  SET_ENV_TEMP(envTemp);
  LOG_OUTPUT(LOG_DEBUG,  "now water temp:%d, env temp: %d, raw: %u\n\r", GET_WATER_TEMP, GET_ENV_TEMP, ReadAdcValue());
}

//Based on the node status and policy to decide the new work mode
uint8 DecideWorkMode(){
  CheckNodeStatus();
  CheckFireStatus();
  GetEnvTemp();
  
  //Only in auto mode or pending status we need to change the work mode
  if(zclWATERSWITCH_OnOffSwitch == AUTO_CONTROL || zclWATERSWITCH_OnOff == PENDING) {
    //Check whether the solar is good for use
    if((device_Status & PUMP_WORKING) && (device_Status & TEMP_WORKING)){
      if(zclWATERSWITCH_OnOff == SALOR_OFF){
        //Now salor is off
#ifdef WINTER_DIFFERENT_POLICY
        if(((GET_WATER_TEMP>=65) //winter
            ||(envTemp>=20 && GET_WATER_TEMP>=60+1))  //summer
           && zclWATERSWITCH_Occupancy >=3 && (!fireUsing) && (!waterEntering)){
#else
             if(GET_WATER_TEMP>=60+1
                && zclWATERSWITCH_Occupancy >=3 && (!fireUsing) && (!waterEntering)){
#endif
                  //Salor is high and user is not using fire
                  return SALOR_ON;
                }
           } else {
             //Now salor is on or pending
             if(waterEntering && salorWaterUsing){
               //Salor is entering water and user is using it, should use fire
               return SALOR_OFF;
             }
             //        if(zclWATERSWITCH_Occupancy==0 || //Too less water
             //           (envTemp< 20 && (zclWATERSWITCH_Occupancy<=1 || zclWATERSWITCH_Temp<65-2) && (!salorWaterUsing))||     //winter
             //             ((zclWATERSWITCH_Temp<60-1 || zclWATERSWITCH_Occupancy<=1) && (!salorWaterUsing)))   //summer
#ifdef WINTER_DIFFERENT_POLICY
             if(zclWATERSWITCH_Occupancy==0 || //Too less water
                (envTemp< 20 && GET_WATER_TEMP<65-2 && (!salorWaterUsing))||     //winter
                  (GET_WATER_TEMP<60-1 && (!salorWaterUsing)))   //summer
#else
               if(zclWATERSWITCH_Occupancy==0 || //Too less water
                  (GET_WATER_TEMP<60-1 && (!salorWaterUsing)))  //Too cold
#endif
               {
                 //Too cold or too less water, use fire
                 return SALOR_OFF;
               }
           }
        //Return the work mode as is, also handle the PENDING case
        if(zclWATERSWITCH_OnOff == SALOR_OFF) {
          return zclWATERSWITCH_OnOff;
        } else {
          return SALOR_ON;
        }
        
      } else {
        //Salor condition is not good, use fire
        return SALOR_OFF;
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
  
  static void PressFireSwitch(){
    FIRE_SWITCH = ACTIVE_HIGH(PRESS_KEY);
    fireOperation |= KEY_FIRE_SWITCH;
    osal_start_timerEx( WaterSwitch_TaskID,
                       WATERSWITCH_FIRE_OPERATION_EVT,
                       WATERSWITCH_PRESS_KEY_TIMEOUT );
    LOG_OUTPUT(LOG_INFO, "Power pressed\r\n");
  }
  
  static void PressTempUpForTime(uint16 timeout){
    //Output
    FIRE_TEMP_UP = ACTIVE_HIGH(PRESS_KEY);
    fireOperation |= KEY_FIRE_TEMP_UP;
    osal_start_timerEx( WaterSwitch_TaskID,
                       WATERSWITCH_FIRE_OPERATION_EVT,
                       timeout );
    LOG_OUTPUT(LOG_INFO,  "Temp up pressed for %u\r\n", timeout);
  }
  
  //Short press
  static void PressTempUp(){
    //Only allow to press temp up when has no other task
    if(!fireOperation && fireStatus==FIRE_STATE_IDLE){
      PressTempUpForTime(WATERSWITCH_PRESS_KEY_TIMEOUT );
    }
  }
  
  //Press 15 seconds
  static void LongPressTemUp(){
    PressTempUpForTime(WATERSWITCH_SET_TEMP_TIMEOUT );
  }
  
  static uint8 valveStatus = PENDING;
  //Turn on/off local and solar pump, local is oppsite with the solar
  static void TurnOnOffValues(uint salorStatus){
    if(valveStatus!=salorStatus){
      //Only when changed, we turn on/off the valves
      salorStatus=salorStatus;
      if(device_Status & PUMP_WORKING){
        if(salorStatus == SALOR_ON){
          zclGeneral_SendOnOff_CmdOn( WATERSWITCH_ENDPOINT, &WaterSwitch_PumpAddr, false, 0 );
        } else {
          zclGeneral_SendOnOff_CmdOff( WATERSWITCH_ENDPOINT, &WaterSwitch_PumpAddr, false, 0 );
        }
        CheckPendingTask(TURN_ON_OFF_VALVE);
      }
      if(salorStatus == SALOR_ON){
        //Local valve should turn off
        TurnOnOffValve(PUMP_OFF);
      } else {
        TurnOnOffValve(PUMP_ON);
      }
    }
  }
  
  //Free the keys when set is done, and check whether need to do it again
  void HandelFireOperationEvents(void){
    
    if(fireOperation & KEY_FIRE_SWITCH){
      LOG_OUTPUT(LOG_INFO, "Power key released\r\n");
      //Just pressed the power key
      //Release it
      FIRE_SWITCH=ACTIVE_HIGH(RELEASE_KEY);
      fireOperation = 0;
      if(zclWATERSWITCH_OnOffSwitch == AUTO_CONTROL){
        //Wait for set the temp
        fireStatus = FIRE_STATE_TO_SET_TEMP;
        osal_start_timerEx( WaterSwitch_TaskID,
                           WATERSWITCH_FIRE_OPERATION_EVT,
                           WATERSWITCH_FIRE_POWER_DELAY_TIMEOUT);
      } else {
        //Manual mode, turn on/off the valves directly
        TurnOnOffValues(zclWATERSWITCH_OnOff);
      }
      return;
    }
    
    if(fireOperation & KEY_FIRE_TEMP_UP){
      LOG_OUTPUT(LOG_INFO,  "Temp up released\r\n");
      FIRE_TEMP_UP=ACTIVE_HIGH(RELEASE_KEY);
      fireOperation = 0;
      if(fireStatus == FIRE_STATE_TO_SET_TEMP){
        //Finished set the temp, now turn on the pump
        TurnOnOffValues(zclWATERSWITCH_OnOff);
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
      if(zclWATERSWITCH_OnOffSwitch == AUTO_CONTROL){
        //Otherwise just press the temprature on
        fireStatus = FIRE_STATE_TO_SET_TEMP;
        LongPressTemUp();
      } else {
        //Manual mode, don't set temp, turn on/off the valve directly
        TurnOnOffValues(zclWATERSWITCH_OnOff);
      }
    }
  }
  
  static void ResetOperationStatus(){
    osal_stop_timerEx( WaterSwitch_TaskID, WATERSWITCH_FIRE_OPERATION_EVT );
    FIRE_TEMP_UP = ACTIVE_HIGH(RELEASE_KEY);
    FIRE_SWITCH = ACTIVE_HIGH(RELEASE_KEY);
    fireOperation = 0;
    fireStatus = FIRE_STATE_IDLE;
  }
  
  //Apply the configuration for the given supplier
  void SelectWaterSupplier(uint8 supplier){
    if(zclWATERSWITCH_OnOff!=supplier){
      //Changed, so we need to reset
      zclWATERSWITCH_OnOff = supplier;
      LOG_OUTPUT(LOG_INFO,  "Salor mode: %d\r\n", zclWATERSWITCH_OnOff);
      
      
      //Clear runing task
      ResetOperationStatus();
      
      //Set pump
      if(zclWATERSWITCH_OnOff == SALOR_ON){
        if(fireTurnedOn){
          //Turn off the fire
          PressFireSwitch();
        }
        TurnOnOffValues(zclWATERSWITCH_OnOff);
      } else {
        //Local valve should turn on
        TurnOnFire();
      }
      //Report the status change
      ReportStatus();
    } else if(zclWATERSWITCH_OnOffSwitch == AUTO_CONTROL){ 
      if(zclWATERSWITCH_OnOff == SALOR_OFF){
        //Fire on, need to keep the fire on
        //Set local/fire
        if((!fireTurnedOn) &&(!fireUsing)){
          //Fire is not turned on
          TurnOnFire();
        } else {
          //Press the temp up button to keep the fire on for each 5 minutes in auto mode
          /*No use*/
          //        if(tick>0 && tick%60==0){
          //          PressTempUp();
          //        }
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
  
  //Send the work mode and water supplier selection to the remote control
  static void ReportStatus(){ 
    
    //Already has remote control connected
    if(WaterSwitch_RemoteControlAddr.addr.shortAddr!=0){
      
      // Set up the first attribute
      pReportCmd->attrList[0].attrID = ATTRID_ON_OFF;
      pReportCmd->attrList[0].dataType = ZCL_DATATYPE_BOOLEAN;
      pReportCmd->attrList[0].attrData = (uint8 *)&zclWATERSWITCH_OnOff;
      
      //Send the report
      zcl_SendReportCmd( WATERSWITCH_ENDPOINT, &WaterSwitch_RemoteControlAddr,
                        ZCL_CLUSTER_ID_GEN_ON_OFF, pReportCmd,
                        ZCL_FRAME_SERVER_CLIENT_DIR, 1, 0 ); 
      //We can send 2 reports in one command. But to keep the API simple, let's do it twice
      // Set up the first attribute
      pReportCmd->attrList[0].attrID = ATTRID_ON_OFF_SWITCH_ACTIONS;
      pReportCmd->attrList[0].dataType = ZCL_DATATYPE_UINT8;
      pReportCmd->attrList[0].attrData = (uint8 *)&zclWATERSWITCH_OnOffSwitch;
      
      //Send the report
      zcl_SendReportCmd( WATERSWITCH_ENDPOINT, &WaterSwitch_RemoteControlAddr,
                        ZCL_CLUSTER_ID_GEN_ON_OFF_SWITCH_CONFIG, pReportCmd,
                        ZCL_FRAME_SERVER_CLIENT_DIR, 1, 0 ); 
    }
  }
  
  void RegularTask( void )
  {
    tick++;
    RestartAdcConvert();
    SelectWaterSupplier(DecideWorkMode());
    UpdateLeds();
    
  }
#endif