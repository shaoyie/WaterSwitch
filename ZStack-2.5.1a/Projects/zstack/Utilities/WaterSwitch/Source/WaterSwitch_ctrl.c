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

#define CMD0_READ 0
#define CMD0_READ_RSP 1
#define CMD0_WRITE 2
#define CMD0_WRITE_RSP 3

#define CMD1_TEMP  0
#define CMD1_OCCUPANCY 1
#define CMD1_DEVICE_STATUS 2
#define CMD1_WORK_MODE 3
#define CMD1_WATER_SUPPLIER 4

#if DEVICE_TYPE==WS_GATEWAY
int pendingCmd = -1;

void ReadAttributeForCmd(uint8 cmd1);

void WaterSwitch_InitIO(void){
}

void HandelSerialData(mtOSALSerialData_t *pkt ){
  uint8 cmd0=pkt->msg[MT_RPC_POS_CMD0];
  uint8 cmd1=pkt->msg[MT_RPC_POS_CMD1];
  uchar strTemp[40]; 
  uint8 length=pkt->msg[MT_RPC_POS_LEN]; 
  
  if(bound){
    //Only when bound the command has meaning
    switch(cmd0){
    case CMD0_READ:
      ReadAttributeForCmd(cmd1);
      break;
    case CMD0_WRITE:
      {
        switch(cmd1){
        case CMD1_WORK_MODE:
          if(length!=1){
            //error
            sprintf(strTemp, "Wrong cmd1 length\n\r");
          } else {
            WriteAttrbuite(ZCL_CLUSTER_ID_GEN_ON_OFF_SWITCH_CONFIG, ATTRID_ON_OFF_SWITCH_ACTIONS, ZCL_DATATYPE_UINT8, &(pkt->msg[MT_RPC_POS_DAT0]));
            //Read attribute to trigger the UI update
            ReadAttributeForCmd(CMD1_WORK_MODE);
          }
          break;
        case CMD1_WATER_SUPPLIER:
          break;
        default:    
          sprintf(strTemp, "Wrong cmd1 %x\n\r", cmd1);
          HalUARTWrite(1, strTemp, strlen(strTemp));
          break;
        }
      }
      break;
    default:    
      sprintf(strTemp, "Wrong cmd0 %x\n\r", cmd0);
      HalUARTWrite(1, strTemp, strlen(strTemp));
      break;
    }
  } else {
    
    sprintf(strTemp, "Not bound with server yet\n\r");
    HalUARTWrite(1, strTemp, strlen(strTemp));
  }
  
  
  HalUARTWrite(1, "Uart got:", 9); 
  HalUARTWrite(1, &(pkt->msg[MT_RPC_FRAME_HDR_SZ]), length); 
}



//Read the specific attribute from the coordinator
void ReadAttributeForCmd(uint8 cmd1){
  uchar strTemp[40]; 
  uint8 attrNum=1;
  switch(cmd1){
  case CMD1_TEMP:
    ReadAttribute(ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT, ATTRID_MS_TEMPERATURE_MEASURED_VALUE);
    break;
  case CMD1_OCCUPANCY:
    ReadAttribute(ZCL_CLUSTER_ID_MS_OCCUPANCY_SENSING, ATTRID_MS_OCCUPANCY_SENSING_CONFIG_OCCUPANCY);
    break;
  case CMD1_DEVICE_STATUS:
    ReadAttribute(ZCL_CLUSTER_ID_GEN_BASIC, ATTRID_BASIC_ALARM_MASK);
    break;
  case CMD1_WORK_MODE:
    ReadAttribute(ZCL_CLUSTER_ID_GEN_ON_OFF_SWITCH_CONFIG, ATTRID_ON_OFF_SWITCH_ACTIONS);
    break;
  case CMD1_WATER_SUPPLIER:
    ReadAttribute(ZCL_CLUSTER_ID_GEN_ON_OFF, ATTRID_ON_OFF);
    break;
  default:    
    sprintf(strTemp, "Wrong cmd1 %x\n\r", cmd1);
    HalUARTWrite(1, strTemp, strlen(strTemp));
    break;
  }
}

//Send the given data through the serial port
void SendSerialData(uint cmd0, uint cmd1, uint8* data, uint8 len){
  uint8* pbuf;
  int i=0;
  int readIndex=0;
  pbuf=(uint8*)osal_msg_allocate(len + 5);
  
  if(pbuf){
    //Fill the header
    pbuf[i++]=MT_UART_SOF;
    pbuf[i++]=len;
    pbuf[i++]=cmd0;
    pbuf[i++]=cmd1;
    //Copy the data
    while(readIndex<len){
      pbuf[i++]=data[readIndex++];
    }
    pbuf[i]=MT_UartCalcFCS(&(pbuf[1]), len+3);
    HalUARTWrite(1, pbuf, len+5); 
    osal_mem_free( pbuf );
  }
}

#ifdef ZCL_READ
/*********************************************************************
* @fn      zclWATERSWITCH_ProcessInReadRspCmd
*
* @brief   Process the "Profile" Read Response Command
*
* @param   pInMsg - incoming message to process
*
* @return  none
*/
uint8 zclWATERSWITCH_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclReadRspCmd_t *readRspCmd;
  uint8 i;
  zclReadRspStatus_t* prsp;
  
  readRspCmd = (zclReadRspCmd_t *)pInMsg->attrCmd;
  for (i = 0; i < readRspCmd->numAttr; i++)
  {
    prsp= &(readRspCmd->attrList[i]);
    if(prsp->status == ZCL_STATUS_SUCCESS){
      // Notify the originator of the results of the original read attributes
      // attempt and, for each successfull request, the value of the requested
      // attribute
      if(pInMsg->clusterId == ZCL_CLUSTER_ID_GEN_BASIC && prsp->attrID==ATTRID_BASIC_ALARM_MASK){
        //Device Status
        uint16 data=*((uint16 *)prsp->data);
        //Report to the upper machine by UART
        SendSerialData(CMD0_READ_RSP, CMD1_DEVICE_STATUS, (uint8*)&data, sizeof(data));
      } else if(pInMsg->clusterId == ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT && prsp->attrID==ATTRID_MS_TEMPERATURE_MEASURED_VALUE){
        //temperature
        uint16 data=*((uint16 *)prsp->data);
        uint8 temp=(uint8)data;
        //Report to the upper machine by UART
        SendSerialData(CMD0_READ_RSP, CMD1_TEMP, &temp, sizeof(temp));
      } else if(pInMsg->clusterId == ZCL_CLUSTER_ID_MS_OCCUPANCY_SENSING && prsp->attrID==ATTRID_MS_OCCUPANCY_SENSING_CONFIG_OCCUPANCY){
        //temperature
        uint16 data=*((uint16 *)prsp->data);
        uint8 occ=(uint8)data;
        //Report to the upper machine by UART
        SendSerialData(CMD0_READ_RSP, CMD1_OCCUPANCY, &occ, sizeof(occ));
      } else if(pInMsg->clusterId == ZCL_CLUSTER_ID_GEN_ON_OFF_SWITCH_CONFIG && prsp->attrID==ATTRID_ON_OFF_SWITCH_ACTIONS){
        //temperature
        uint16 data=*((uint16 *)prsp->data);
        uint8 workmode=(uint8)data;
        //Report to the upper machine by UART
        SendSerialData(CMD0_READ_RSP, CMD1_WORK_MODE, &workmode, sizeof(workmode));
      } else if(pInMsg->clusterId == ZCL_CLUSTER_ID_GEN_ON_OFF && prsp->attrID==ATTRID_ON_OFF){
        //temperature
        uint16 data=*((uint16 *)prsp->data);
        uint8 supplier=(uint8)data;
        //Report to the upper machine by UART
        SendSerialData(CMD0_READ_RSP, CMD1_WATER_SUPPLIER, &supplier, sizeof(supplier));
      }
    }  else {
      //error
    }
  }
  
  return TRUE;
}
#endif // ZCL_READ

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

void RegularTask( void )
{
}
#endif