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
#include "zcl.h"
#include "zcl_ms.h"
#include "ZComDef.h"
  
#define DEBUG

/*********************************************************************
 * CONSTANTS
 */
#define DEVICE_TYPE   WS_PUMP
//Only choose one of the four
#define WS_COORDINATOR  1
#define WS_PUMP   2
#define WS_TEMP   3
#define WS_GATEWAY  4
  
#define WATERSWITCH_CLUSTERID 0xFC01  //For internal data transfer
  
// These constants are only for example and should be changed to the
// device's needs
#define WATERSWITCH_ENDPOINT           20

#define WATERSWITCH_PROFID             ZCL_HA_PROFILE_ID //0x0104
  
#if DEVICE_TYPE==WS_COORDINATOR
  
#define WATERSWITCH_DEVICEID           ZCL_HA_DEVICEID_ON_OFF_SWITCH   
#define ZCLWATERSWITCH_MAX_INCLUSTERS        5
#define ZCLWATERSWITCH_MAX_OUTCLUSTERS       4
#define WATERSWITCH_MAX_ATTRIBUTES        15
  
#elif DEVICE_TYPE==WS_PUMP
#define WATERSWITCH_DEVICEID           ZCL_HA_DEVICEID_PUMP
#define ZCLWATERSWITCH_MAX_INCLUSTERS        1
#define ZCLWATERSWITCH_MAX_OUTCLUSTERS       1
#define WATERSWITCH_MAX_ATTRIBUTES        12
  
#elif DEVICE_TYPE==WS_TEMP
#define WATERSWITCH_DEVICEID           ZCL_HA_DEVICEID_TEMPERATURE_SENSOR
#define ZCLWATERSWITCH_MAX_INCLUSTERS        0
#define ZCLWATERSWITCH_MAX_OUTCLUSTERS       3
#define WATERSWITCH_MAX_ATTRIBUTES        13
  
#else
#define WATERSWITCH_DEVICEID           ZCL_HA_DEVICEID_REMOTE_CONTROL
#endif


// Send Message Timeout
#define WATERSWITCH_REGULAR_TASK_TIMEOUT   5000     // Every 5 seconds
#define WATERSWITCH_VALVE_TIMEOUT   15000
  
#define WATERSWITCH_DELAY_TIMEOUT   1000

// Application Events (OSAL) - These are bit weighted definitions.
#define WATERSWITCH_REGULAR_TASK_EVT       0x0001
#define WATERSWITCH_MATCH_SERVICE_EVT       0x0004
#define WATERSWITCH_VALVE_SERVICE_EVT       0x0008

#if defined( IAR_ARMCM3_LM )
#define WATERSWITCH_RTOS_MSG_EVT       0x0002
#endif  
  
#define PUMP_OFF                       0x00
#define PUMP_ON                        0x01
  
#define SALOR_OFF                       0x00
#define SALOR_ON                        0x01
  
#define AUTO_CONTROL                    0x00
#define MANUAL_CONTROL                  0x01
  
#define TEMP_WORKING                    0x01
#define PUMP_WORKING                    0x02
#define TEMP_ERROR                      (1<<4)
#define PUMP_ERROR                      (2<<4)
  
#define ERROR_MASK                      0xf0
#define WORKING_STATUS_MASK             0x0f

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Task Initialization for the Generic Application
 */
extern void WaterSwitch_Init( byte task_id );
extern SimpleDescriptionFormat_t WaterSwitch_epDesc;

extern CONST zclAttrRec_t zclWATERSWITCH_Attrs[];
/*
 * Task Event Processor for the Generic Application
 */
extern UINT16 WaterSwitch_ProcessEvent( byte task_id, UINT16 events );

// The status value we care
extern uint8  zclWATERSWITCH_OnOff;
extern uint8 zclWATERSWITCH_OnOffSwitch;
extern uint8  zclWATERSWITCH_OnOff;
extern uint16  zclWATERSWITCH_Temp;
extern uint16  zclWATERSWITCH_Occupancy;
#if DEVICE_TYPE==WS_TEMP
extern cId_t* zclWATERSWITCH_InClusterList;
#else
extern cId_t zclWATERSWITCH_InClusterList[];
#endif
extern cId_t zclWATERSWITCH_OutClusterList[];

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* WATERSWITCH_H */
