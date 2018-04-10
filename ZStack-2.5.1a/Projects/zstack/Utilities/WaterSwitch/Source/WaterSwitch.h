/**************************************************************************************************
  Filename:       WaterSwitch.h
  Revised:        $Date: 2012-02-12 15:58:41 -0800 (Sun, 12 Feb 2012) $
  Revision:       $Revision: 29216 $

  Description:    This file contains the Generic Application definitions.


  Copyright 2004-2012 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License"). You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product. Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED “AS IS?WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, 
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE, 
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com. 
**************************************************************************************************/

#ifndef WATERSWITCH_H
#define WATERSWITCH_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */
#include "WaterSwitchDeviceType.h"
#include "zcl.h"
#include "zcl_ms.h"
#include "ZComDef.h"
#include "MT_UART.h"
  


/*********************************************************************
 * MACROS
 */

#define uint unsigned int
#define uchar unsigned char
/*********************************************************************
 * FUNCTIONS
 */

/*
 * Task Initialization for the Generic Application
 */
void WaterSwitch_Init( byte task_id );

typedef struct
{
  uint16          tempCalibration;      //Tempratrue calibration value
  uint8           winterThreshold;      //The env temp decide winter
  uint8           winterSwtichTemp;     //The center temp to switch between fire and gas in winter
  uint8           summerSwitchTemp;     //The switch temp in summer
} waterSwichConfig_t;

extern SimpleDescriptionFormat_t WaterSwitch_epDesc;
extern SimpleDescriptionFormat_t WaterSwitch_custEpDesc;

extern waterSwichConfig_t zclWATERSWITCH_NvConfig;

extern CONST zclAttrRec_t zclWATERSWITCH_Attrs[];
/*
 * Task Event Processor for the Generic Application
 */
extern UINT16 WaterSwitch_ProcessEvent( byte task_id, UINT16 events );

// The status value we care
extern byte WaterSwitch_TaskID;
extern byte WaterSwitch_TransID;
extern afAddrType_t WaterSwitch_DstAddr;
extern uint16 device_Status;
extern uint8 zclWATERSWITCH_PhysicalEnvironment;
extern uint8  zclWATERSWITCH_OnOff;
extern uint8 zclWATERSWITCH_OnOffSwitch;
extern uint16  zclWATERSWITCH_Temp;
extern uint16  zclWATERSWITCH_Occupancy;
extern uint16 zclWATERSWITCH_Flow;

#if DEVICE_TYPE==WS_TEMP || DEVICE_TYPE==WS_GATEWAY
extern cId_t* zclWATERSWITCH_InClusterList;
#else
extern cId_t zclWATERSWITCH_InClusterList[];
#endif
extern cId_t zclWATERSWITCH_OutClusterList[];

extern byte bound;
extern uint16 pendingTask;
extern zclReportCmd_t *pReportCmd;
extern char strTemp[];

#if DEVICE_TYPE==WS_COORDINATOR
extern afAddrType_t WaterSwitch_TempDstAddr;
extern afAddrType_t WaterSwitch_PumpAddr;
extern afAddrType_t WaterSwitch_RemoteControlAddr;
extern uint16 device_Status;
extern uint32 tick;
extern uint32 lastTempTick;
extern uint32 lastPumpTick;
extern uint32 lastFireOnTick;
extern uint16 waterEntering;
extern uint16 salorWaterUsing;
extern uint8 fireTurnedOn;
extern uint8 fireUsing;
extern uint8 fireOperation;
extern uint32 p0_0_time;
extern uint32 p1_3_time;

ZStatus_t wsGloalConfigCB( uint16 clusterId, uint16 attrId, uint8 oper,
                          uint8 *pValue, uint16 *pLen );
void HandelFireOperationEvents(void);
void SelectWaterSupplier(uint8 supplier);
void ActiveEPReq(uint16 bindAddr);
void ToggleWaterSupplier();
void ToggleWorkMode();
void UpdateLeds();
#endif
#if DEVICE_TYPE==WS_GATEWAY   
void HandelSerialData(mtOSALSerialData_t *pkt );
void SendSerialData(uint cmd0, uint cmd1, uint8* data, uint8 len);
#endif

void SendFlowReport();

void WriteAttrbuite(uint16 clusterID, uint16 attrID, uint8  dataType, uint8* data);
void ReadAttribute(uint16 clusterID, uint16 attrID);

void AfSendData(uint16 shortAddr, uint8* data, uint16 length);
void CheckPendingTask(uint16 task);
void ClearPendingTask(uint16 task);
void CheckPendingTaskCB();

#if DEVICE_TYPE==WS_COORDINATOR
#define LOG_OUTPUT_CHANNEL      WaterSwitch_RemoteControlAddr.addr.shortAddr
#else
#define LOG_OUTPUT_CHANNEL      0
#endif

#define LOG_LEVEL_ENABLED(l) (zclWATERSWITCH_PhysicalEnvironment&l)

//Data should always be enabled
#define INFO_OUTPUT(level, x, len)  if(((zclWATERSWITCH_PhysicalEnvironment|DATA_OUTPUT)&level)!=0){   \
if(zclWATERSWITCH_PhysicalEnvironment&OUTPUT_AF_MESSAGE){                           \
  AfSendData(LOG_OUTPUT_CHANNEL, x, len);                                 \
} else {                                                                  \
  HalUARTWrite(1, x, len);                                                \
}                                                                         \
}

#define LOG_OUTPUT(level, format, args...)    if(((zclWATERSWITCH_PhysicalEnvironment|DATA_OUTPUT)&level)!=0){ \
  sprintf(strTemp, format, ##args);                                                \
  INFO_OUTPUT(level, strTemp, strlen(strTemp));                                   \
} 

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* WATERSWITCH_H */
