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

#if DEVICE_TYPE==WS_TEMP
#if (HAL_ADC == TRUE)
extern uint16 adcBuf[];
#endif
static uint16 readIndex=0;
static uint8 adcPending=0;
static int lastSentIndex=0;

static uint16 rawdataWriteIndex=0;


void WaterSwitch_InitIO(void){
  
  //Input
  /* configure tristates */
  //3-state for input
  //P1 Ports
  P1INP |= (WATER_ENTERING_DETECT_BV);
  P1SEL &= ~(WATER_ENTERING_DETECT_BV);    /* Set pin function to GPIO */
  P1DIR &= ~(WATER_ENTERING_DETECT_BV);    /* Set pin direction to Input */
  
  
  //Not use ADC, then use timer
  //Config P1.2
  
  PERCFG |= (1<<6); //Use loc 2
  P1SEL &= ~(TEMP_DECT_PIN_BV);    //P1.2 use as GPIO
  P2SEL |= (1<<3);   //Timer1 has priority
  P1INP |= (TEMP_DECT_PIN_BV);    //3 state
  P1DIR &= ~(TEMP_DECT_PIN_BV); 
  
  T1CTL = 5;    //prescaler 8, free running
  //T1CTL = 1;    //prescaler 1, free running
  T1CCTL0 &= ~(1<<2);   //capture mode, capture on all edges
  T1CCTL0 |= (1<<6)|3;  //capture on all edges, interrupt enabled  
  T1CNTL = 0xff;   //Write anything to init the channel 0
  
  T1IE = 1;     //Enable interrupt
  
}

void CheckPendingTaskCB(){
}

#ifdef CAPTURE_RAW_DATA
static uint16 durRaw[200];
#endif
static uint16 durations[3][4];
static uint16 validDuration[4];
static uint8 writeIndex=0;
static uint8 typeIndex=0;
static uint8 dataCount=0;
static uint8 hasValidDuration=0;

static uint8 lastDriveState=255;
static uint16 lastTime=0;

static uint32 lastValidTick = 0;

void ProcessLineData(){
  //A line full, process it
  if(dataCount<3) {dataCount++;}
  if(dataCount>=3){
    //Has enough data to check whether we have valid data or not
    uint8 hasInvalidData=0;
    for(int i=P0_DURATION;i<=P1_DURATION;i++){
      if(i==P2_OUT_DURATION){
        //P2 out don't care
        continue;
      }
      for(int j=0;j<2;j++){
        uint16 deviation = durations[j][i]>durations[j+1][i]?durations[j][i]-durations[j+1][i]:durations[j+1][i]-durations[j][i];
        //The data don't vary too much
        if(deviation>CAPTURE_VALUE_TOLERANCE){
          //Invalid data
          hasInvalidData=1;
          break;
        }
      }
      if(hasInvalidData){
        break;
      }
    }
    if(!hasInvalidData){
      //has valid data
      if(!hasValidDuration){
        hasValidDuration = 1;
      }
      lastValidTick = osal_GetSystemClock();
#ifdef CAPTURE_RAW_DATA
      //Take the last one
      for(int i=P0_DURATION;i<=P1_DURATION;i++){
        validDuration[i]=durations[writeIndex][i];
      }
#else
      //Take the average value as the data
      uint32 temp;
      for(int i=P0_DURATION;i<=P1_DURATION;i++){
        temp=0;
        for(int j=0;j<3;j++){
          temp+=durations[j][i];
        }
        validDuration[i]=temp/3;
      }
#endif
    }
  }  
  //Move to new line
  typeIndex = P0_DURATION;
  writeIndex++;
  if(writeIndex>=3){
    writeIndex = 0;
  }
}

#pragma vector = T1_VECTOR
__interrupt void Timer1_ISR(void)
{
  T1IF = 0;
  if(T1STAT&1){
    //The channel 0 captured something
    //T1STAT &= ~1;
    T1STAT = 0;
    
    uint16 timestamp = T1CC0H<<8|T1CC0L;
    uint8 state=TEMP_DECT_PIN;
    if(state || lastDriveState==state || lastDriveState==255){
      //Init, just record
      //Or it's high value
      //Or we lost one edge? So we cannot do correct calculation but just record it
      lastDriveState = state;
      lastTime = timestamp;
    } else {
      //We got a valid pulse
      uint16 duration;
      if(timestamp>lastTime){
        duration = timestamp-lastTime;
      } else {
        //roll up
        duration = 65536 - lastTime + timestamp;
      }
      if(duration>0){
#ifdef CAPTURE_RAW_DATA
        if(rawdataWriteIndex<200){
          durRaw[rawdataWriteIndex++]=duration;
        }
#endif
        //Valid duration
        if(typeIndex>P0_DURATION){
          if(durations[writeIndex][P0_DURATION]*1.2<=duration){
            //P0 should be the biggest
            //Found a new bigger one which is bigger thnn origin P0's 120%, take it as P0
            typeIndex = P0_DURATION;
          }          
        }
        if(typeIndex==P1_DURATION){
          //The P2_OUT_DURATION could be too small to catch, verify
          if(duration>durations[writeIndex][P0_DURATION]*0.9){
            //Bigger than origin P0's 90%, so New is P0
            //The original P0 is also P0, so we missed the P2_OUT_DURATION
            durations[writeIndex][P1_DURATION]=durations[writeIndex][P2_OUT_DURATION];
            //Clear P2 out
            durations[writeIndex][P2_OUT_DURATION]=0;
            ProcessLineData();
          } else {
            //Just accept it
          }
        }
        //Save to proper location
        durations[writeIndex][typeIndex++]=duration;
        if(typeIndex>P1_DURATION){
          ProcessLineData();
          
        }
      } else {
        //Wrong, try again
        typeIndex = P0_DURATION;
      }
      lastDriveState = state;
      lastTime = timestamp;
    }
  }
}

static void SendTempReport(){ 
  
  // Set up the first attribute
  pReportCmd->attrList[0].attrID = ATTRID_MS_TEMPERATURE_MEASURED_VALUE;
  pReportCmd->attrList[0].dataType = ZCL_DATATYPE_UINT16;
  pReportCmd->attrList[0].attrData = (uint8 *)&zclWATERSWITCH_Temp;
  
  //Send the report
  zcl_SendReportCmd( WATERSWITCH_ENDPOINT, &WaterSwitch_DstAddr,
                    ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT, pReportCmd,
                    ZCL_FRAME_SERVER_CLIENT_DIR, 1, 0 ); 
}
static void SendOccupancyReport(){ 
  
  // Set up the first attribute
  pReportCmd->attrList[0].attrID = ATTRID_MS_OCCUPANCY_SENSING_CONFIG_OCCUPANCY;
  pReportCmd->attrList[0].dataType = ZCL_DATATYPE_UINT16;
  pReportCmd->attrList[0].attrData = (uint8 *)&zclWATERSWITCH_Occupancy;
  
  //Send the report
  zcl_SendReportCmd( WATERSWITCH_ENDPOINT, &WaterSwitch_DstAddr,
                    ZCL_CLUSTER_ID_MS_OCCUPANCY_SENSING, pReportCmd,
                    ZCL_FRAME_SERVER_CLIENT_DIR, 1, 0 ); 
}

#ifdef USE_ADC
void ProcessAdcBatchData(void){
  
  uchar strTemp[100];
#ifdef CAPTURE_RAW_DATA
#if (HAL_ADC == TRUE)
  if(readIndex<ADC_CAPTURE_COUNT){
    //Really raw data
    //    zclWATERSWITCH_Temp = adcBuf[readIndex*3];
    //    zclWATERSWITCH_Occupancy = adcBuf[readIndex*3+1];
    //    zclWATERSWITCH_Flow =adcBuf[readIndex*3+2];
    int actualIndex=readIndex*3;
    zclWATERSWITCH_Temp = adcBuf[actualIndex]>>6;
    zclWATERSWITCH_Occupancy = adcBuf[actualIndex+1]>>6;
    zclWATERSWITCH_Flow =adcBuf[actualIndex+2]>>6;
    //low 6 bits in temp
    zclWATERSWITCH_Temp|=((readIndex)&0x3f)<<10;
    //high 6 bits in occpuancy
    zclWATERSWITCH_Occupancy|=((readIndex)&0x0fc0)<<4;
    
    //Send
    SendTempReport();
    SendOccupancyReport();
    SendFlowReport();
    
    readIndex++;
    sprintf(strTemp, "Got data	%u	%u	%u	%d\n\r", zclWATERSWITCH_Temp, zclWATERSWITCH_Occupancy, zclWATERSWITCH_Flow, readIndex);
    INFO_OUTPUT( strTemp, strlen(strTemp)); //输出接收到的数据 
    osal_set_event (WaterSwitch_TaskID, WATERSWITCH_HAL_ADC_TRANSFER_DONE_EVT);
  } else {
    readIndex = 0;
    lastSentIndex = 0;
    //end
    zclWATERSWITCH_Temp = 999;
    zclWATERSWITCH_Occupancy = 0;
    zclWATERSWITCH_Flow = 888;
    //Send
    SendTempReport();
    SendOccupancyReport();
    SendFlowReport();
    //Let the regular task do it
    //Restart
    adcPending=0;
    //RestartAdcConvert();
  }
#endif
#endif
}
#endif

void RegularTask( void )
{
#if defined CAPTURE_RAW_DATA|| defined DEBUG
  uchar strOutput[100];
#endif
#ifdef USE_ADC
  //Start ADC
  if(adcPending==0){
    adcPending=1;
    RestartAdcConvert();
  }
  
  //uint16 adcData[3];
  //ReadAdcValues(adcData);
#endif
  
#ifdef DEBUG
  strOutput[0]=0;
#ifdef CAPTURE_RAW_DATA
  if(rawdataWriteIndex>=200){
    int i=0;
    for(;i<1;i++){
      sprintf(strTemp, "Raw:	%u	%u	%u	%u\n\r", durRaw[i*4+readIndex+P0_DURATION], durRaw[i*4+readIndex+P2_IN_DURATION], durRaw[i*4+readIndex+P2_OUT_DURATION],durRaw[i*4+readIndex+P1_DURATION]);
      strcat(strOutput, strTemp);
    }
    readIndex+=(i*4);
    if(readIndex>=200){
      rawdataWriteIndex = 0;
      readIndex = 0;
    }
  }
#endif
  
  sprintf(strTemp, "Valid:	%u	%u	%u	%u\n\r", validDuration[P0_DURATION], validDuration[P2_IN_DURATION], validDuration[P2_OUT_DURATION],validDuration[P1_DURATION]);
  strcat(strOutput, strTemp);
  AfSendData(0, strOutput, strlen(strOutput));
#endif
  
  //Fill into the attribute    
  if(hasValidDuration && osal_GetSystemClock() - lastValidTick <=1000){
    float p0=validDuration[P0_DURATION];
    float p1=validDuration[P1_DURATION];
    float p2=validDuration[P2_IN_DURATION];
    if(p0>0 && p1>0 && p2>0){
      p1=p0/p1;
      p2=p2/p0;
      //Temp
      //The formula need to adjust
      zclWATERSWITCH_Temp = 26.455*p1-31.974;
      //Water level
      if(p2>WATER_LEVEL0){
        zclWATERSWITCH_Occupancy = 0;
      } else if(p2>WATER_LEVEL1){
        zclWATERSWITCH_Occupancy = 1;
      } else if(p2>WATER_LEVEL2){
        zclWATERSWITCH_Occupancy = 2;
      } else if(p2>WATER_LEVEL3){
        zclWATERSWITCH_Occupancy = 3;
      } else{
        zclWATERSWITCH_Occupancy = 4;
      }
      //Send
      SendTempReport();
      SendOccupancyReport();
    }
  }
  //Flow
  zclWATERSWITCH_Flow = ACTIVE_LOW(WATER_ENTERING_DETECT);
  SendFlowReport();
  //Debug mode, send the raw data    
  LOG_OUTPUT(LOG_DEBUG, "Got data: %u, %u, %u\n\r", zclWATERSWITCH_Temp, zclWATERSWITCH_Occupancy, zclWATERSWITCH_Flow);
  
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
}
#endif