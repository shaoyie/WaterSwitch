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
  
  T1CTL =1 ;    //prescaler 1, free running
  T1CCTL0 &= ~(1<<2);   //capture mode, capture on all edges
  T1CCTL0 |= (1<<6)|3;  //capture on all edges, interrupt enabled  
  T1CNTH = 1;   //Write anything to init the channel 0
  
  T1IE = 1;     //Enable interrupt

}

static uint16 durations[3][4];
static uint16 validDuration[4];
static uint8 writeIndex=0;
static uint8 typeIndex=0;
static uint8 dataCount=0;
static uint8 hasValidDuration=0;

static uint8 lastDriveState=999;
static uint16 lastTime=0;
#pragma vector = T1_VECTOR
__interrupt void Timer1_ISR(void)
{
  T1IF = 0;
  if(T1STAT&1){
    //The channel 0 captured something
    T1STAT &= ~1;
    
    uint16 timestamp = T1CC0H<<8|T1CC0L;
    uint8 state=TEMP_DECT_PIN;
    if(state || lastDriveState==state || lastDriveState==999){
      //Init, just record
      //Or it's high value
      //Or we lost one edge? So we cannot do correct calculation but just record it
      lastDriveState = TEMP_DECT_PIN;
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
        //Valid duration
        if(typeIndex>P0_DURATION){
          if(durations[writeIndex][P0_DURATION]<=duration){
            //P0 should be the biggest
            //Found a new bigger one, take it as P0
            typeIndex = P0_DURATION;
          }          
        }
        //Save to proper location
        durations[writeIndex][typeIndex++]=duration;
        if(typeIndex>P1_DURATION){
          //A line full, process it
          if(dataCount<3) {dataCount++;}
          if(dataCount>=3){
            //Has enough data to check whether we have valid data or not
            uint8 hasInvalidData=0;
            for(int i=P0_DURATION;i<=P1_DURATION;i++){
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
      } else {
        //Wrong, try again
        typeIndex = P0_DURATION;
      }
      lastDriveState = TEMP_DECT_PIN;
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
    HalUARTWrite(1, strTemp, strlen(strTemp)); //输出接收到的数据 
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

void RegularTask( void )
{
#if 0//(HAL_ADC == TRUE)
  //Start ADC
  if(adcPending==0){
    adcPending=1;
    RestartAdcConvert();
  }
#else
  //uint16 adcData[3];
  //ReadAdcValues(adcData);
#ifdef CAPTURE_RAW_DATA
  //Debug mode, send the raw data  
  
  uchar strTemp[40];
  
  
  if(hasValidDuration){
    //Fill into the attribute
    zclWATERSWITCH_Temp = validDuration[P0_DURATION];
    zclWATERSWITCH_Occupancy = validDuration[P2_IN_DURATION];
    zclWATERSWITCH_Flow =validDuration[P1_DURATION];
  }
  sprintf(strTemp, "Got data: %u, %u, %u, %u\n\r", zclWATERSWITCH_Temp, zclWATERSWITCH_Occupancy, zclWATERSWITCH_Flow, validDuration[P2_OUT_DURATION]);
  HalUARTWrite(1, strTemp, strlen(strTemp)); //输出接收到的数据 
#else
  
  //Fill into the attribute
  //zclWATERSWITCH_Temp = adcData[0];
  //zclWATERSWITCH_Occupancy = adcData[1];
  zclWATERSWITCH_Flow = ACTIVE_HIGH(WATER_ENTERING_DETECT);
#endif
  //Send
  SendTempReport();
  SendOccupancyReport();
  SendFlowReport();
#endif
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