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

#if DEVICE_TYPE==WS_GATEWAY
uint8 targetWorkMode=0;
uint8 targetWaterSupplier=0;

void ReadAttributeForCmd(uint8 cmd1);

void WaterSwitch_InitIO(void){
}

static void SendSwitchCmd(uint supplier){
  
  if(supplier==SALOR_ON){
    zclGeneral_SendOnOff_CmdOn( WATERSWITCH_ENDPOINT, &WaterSwitch_DstAddr, false, 0 );
  } else {
    zclGeneral_SendOnOff_CmdOff( WATERSWITCH_ENDPOINT, &WaterSwitch_DstAddr, false, 0 );
  }
}

void CheckPendingTaskCB(){
  //Has pending task
  //Set work mode
  if(pendingTask & SET_WORKMODE){
    //Send again
    WriteAttrbuite(ZCL_CLUSTER_ID_GEN_ON_OFF_SWITCH_CONFIG, ATTRID_ON_OFF_SWITCH_ACTIONS, ZCL_DATATYPE_UINT8, &targetWorkMode);
    CheckPendingTask(SET_WORKMODE);
  }
  
  //Turn on/off valve
  if(pendingTask & TURN_ON_OFF_VALVE){
    //Send again
    SendSwitchCmd(targetWaterSupplier);
    CheckPendingTask(TURN_ON_OFF_VALVE);
  }
}

/*********************************************************************
* @fn      zclWATERSWITCH_ProcessInWriteRspCmd
*
* @brief   Process the "Profile" Write Response Command
*
* @param   pInMsg - incoming message to process
*
* @return  none
*/
uint8 zclWATERSWITCH_ProcessInWriteRspCmd( zclIncomingMsg_t *pInMsg )
{
  
  //We only care the feedback when we have pending task
  if(pendingTask & SET_WORKMODE){
    zclWriteRspCmd_t *writeRspCmd;
    uint8 i;
    LOG_OUTPUT(LOG_DEBUG, "Got write attribute feedback\n\r");
    writeRspCmd = (zclWriteRspCmd_t *)pInMsg->attrCmd;
    for (i = 0; i < writeRspCmd->numAttr; i++)
    {
      // Notify the device of the results of the its original write attributes
      // command.
      if(pInMsg->clusterId==ZCL_CLUSTER_ID_GEN_ON_OFF_SWITCH_CONFIG &&writeRspCmd->attrList[i].status == ZCL_STATUS_SUCCESS){
        //Write success, clear the pending task sign
        ClearPendingTask(SET_WORKMODE);
        //Notify up level
        //auto/manual work mode
        SendSerialData(CMD0_WRITE_RSP, CMD1_WORK_MODE, &targetWorkMode, sizeof(targetWorkMode));
        LOG_OUTPUT(LOG_DEBUG, "Clear the pending task\n\r");
      }
    }
  }
  
  return TRUE;
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
  reportCmd = (zclReportCmd_t *)pInMsg->attrCmd;
  for (i = 0; i < reportCmd->numAttr; i++)
  {
    // Device is notified of the latest values of the attribute of another device.
    reportRec = &(reportCmd->attrList[i]);
    
    if(pInMsg->clusterId==ZCL_CLUSTER_ID_GEN_ON_OFF && 
       reportRec->attrID == ATTRID_ON_OFF)
    {
      
      //salor or fire
      uint8 supplier=*((uint8 *)reportRec->attrData);
      //Report to the upper machine by UART
      SendSerialData(CMD0_READ_RSP, CMD1_WATER_SUPPLIER, &supplier, sizeof(supplier));
      
    } else if(pInMsg->clusterId==ZCL_CLUSTER_ID_GEN_ON_OFF_SWITCH_CONFIG && 
              reportRec->attrID == ATTRID_ON_OFF_SWITCH_ACTIONS)
    {
      //auto/manual work mode
      uint8 workmode=*((uint8 *)reportRec->attrData);
      //Report to the upper machine by UART
      SendSerialData(CMD0_READ_RSP, CMD1_WORK_MODE, &workmode, sizeof(workmode));
    }
  }
  return TRUE;
}

void HandelSerialData(mtOSALSerialData_t *pkt ){
  uint8 cmd0=pkt->msg[MT_RPC_POS_CMD0];
  uint8 cmd1=pkt->msg[MT_RPC_POS_CMD1];
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
            LOG_OUTPUT(LOG_ERROR, "Wrong cmd1 length\n\r");
          } else {
            WriteAttrbuite(ZCL_CLUSTER_ID_GEN_ON_OFF_SWITCH_CONFIG, ATTRID_ON_OFF_SWITCH_ACTIONS, ZCL_DATATYPE_UINT8, &(pkt->msg[MT_RPC_POS_DAT0]));
            //Make sure the set is done
            targetWorkMode=pkt->msg[MT_RPC_POS_DAT0];
            CheckPendingTask(SET_WORKMODE);
          }
          break;
        case CMD1_SWITCH_OUTPUT:
          if(length!=1){
            //error
            LOG_OUTPUT(LOG_ERROR,  "Wrong cmd1 length\n\r");
          } else {
            WriteAttrbuite(ZCL_CLUSTER_ID_GEN_BASIC, ATTRID_BASIC_PHYSICAL_ENV, ZCL_DATATYPE_UINT8, &(pkt->msg[MT_RPC_POS_DAT0]));
          }
          break;
        case CMD1_WATER_SUPPLIER:
          
          if(length!=1){
            //error
            LOG_OUTPUT(LOG_ERROR,  "Wrong cmd1 length\n\r");
          } else {
            targetWaterSupplier=pkt->msg[MT_RPC_POS_DAT0];
            SendSwitchCmd(targetWaterSupplier);
            //Make sure the set is done
            CheckPendingTask(TURN_ON_OFF_VALVE);
          }
          break;
        default:
          LOG_OUTPUT(LOG_ERROR,  "Wrong cmd1 %x\n\r", cmd1);
          break;
        }
      }
      break;
    default:    
      LOG_OUTPUT(LOG_ERROR,  "Wrong cmd0 %x\n\r", cmd0);
      break;
    }
  } else {
    
    LOG_OUTPUT(LOG_ERROR,  "Not bound with server yet\n\r");
  }
  
  
  INFO_OUTPUT(LOG_DEBUG, "Uart got:", 9); 
  INFO_OUTPUT(LOG_DEBUG, &(pkt->msg[MT_RPC_FRAME_HDR_SZ]), length); 
}



//Read the specific attribute from the coordinator
void ReadAttributeForCmd(uint8 cmd1){
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
  case CMD1_SWITCH_OUTPUT:
    ReadAttribute(ZCL_CLUSTER_ID_GEN_BASIC, ATTRID_BASIC_PHYSICAL_ENV);
    break;
  default:    
    LOG_OUTPUT(LOG_ERROR, "Wrong cmd1 %x\n\r", cmd1);
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
    INFO_OUTPUT(DATA_OUTPUT, pbuf, len+5); 
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
        //Report to the upper machine by UART
        SendSerialData(CMD0_READ_RSP, CMD1_TEMP, (uint8*)&data, sizeof(data));
      } else if(pInMsg->clusterId == ZCL_CLUSTER_ID_MS_OCCUPANCY_SENSING && prsp->attrID==ATTRID_MS_OCCUPANCY_SENSING_CONFIG_OCCUPANCY){
        //water level
        uint16 data=*((uint16 *)prsp->data);
        uint8 occ=(uint8)data;
        //Report to the upper machine by UART
        SendSerialData(CMD0_READ_RSP, CMD1_OCCUPANCY, &occ, sizeof(occ));
      } else if(pInMsg->clusterId == ZCL_CLUSTER_ID_GEN_ON_OFF_SWITCH_CONFIG && prsp->attrID==ATTRID_ON_OFF_SWITCH_ACTIONS){
        //auto/manual work mode
        uint8 workmode=*(prsp->data);
        //Report to the upper machine by UART
        SendSerialData(CMD0_READ_RSP, CMD1_WORK_MODE, &workmode, sizeof(workmode));
      } else if(pInMsg->clusterId == ZCL_CLUSTER_ID_GEN_ON_OFF && prsp->attrID==ATTRID_ON_OFF){
        //salor or fire
        uint8 supplier=*(prsp->data);
        //Report to the upper machine by UART
        SendSerialData(CMD0_READ_RSP, CMD1_WATER_SUPPLIER, &supplier, sizeof(supplier));
      } else if(pInMsg->clusterId == ZCL_CLUSTER_ID_GEN_BASIC && prsp->attrID==ATTRID_BASIC_PHYSICAL_ENV){
        //out put channel
        uint8 tempSwitch=*(prsp->data);
        //Report to the upper machine by UART
        SendSerialData(CMD0_READ_RSP, CMD1_SWITCH_OUTPUT, &tempSwitch, sizeof(tempSwitch));
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