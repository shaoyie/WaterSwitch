/**************************************************************************************************
  Filename:       zcl_WATERSWITCH_data.c
  Revised:        $Date: 2008-03-11 11:01:35 -0700 (Tue, 11 Mar 2008) $
  Revision:       $Revision: 16570 $


  Description:    Zigbee Cluster Library - sample device application.


  Copyright 2006-2007 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED �AS IS?WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, 
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

/*********************************************************************
 * INCLUDES
 */
#include "ZComDef.h"
#include "OSAL.h"
#include "AF.h"
#include "ZDConfig.h"

#include "zcl.h"
#include "zcl_general.h"
#include "zcl_ha.h"

#include "WaterSwitch.h"

/*********************************************************************
 * CONSTANTS
 */

#define WATERSWITCH_DEVICE_VERSION     0
#define WATERSWITCH_FLAGS              0

#define WATERSWITCH_HWVERSION          0
#define WATERSWITCH_ZCLVERSION         0

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

// Basic Cluster
const uint8 zclWATERSWITCH_HWRevision = WATERSWITCH_HWVERSION;
const uint8 zclWATERSWITCH_ZCLVersion = WATERSWITCH_ZCLVERSION;
const uint8 zclWATERSWITCH_ManufacturerName[] = { 16, 'S','a','w','y','e','r',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ' };
#if DEVICE_TYPE==WS_COORDINATOR
const uint8 zclWATERSWITCH_ModelId[] = { 16, 'W','A','T','E','R','S','W','I','T','C','H',' ',' ',' ',' ',' ' };
#elif DEVICE_TYPE==WS_PUMP
const uint8 zclWATERSWITCH_ModelId[] = { 16, 'W','A','T','E','R','S','W','I','T','C','H','_','P','U','M','P' };
#elif DEVICE_TYPE==WS_TEMP
const uint8 zclWATERSWITCH_ModelId[] = { 16, 'W','A','T','E','R','S','W','I','T','C','H','_','T','E','M','P' };
#elif DEVICE_TYPE==WS_GATEWAY
const uint8 zclWATERSWITCH_ModelId[] = { 16, 'W','A','T','E','R','S','W','I','T','C','H','_','G','W',' ',' ' };
#else
const uint8 zclWATERSWITCH_ModelId[] = { 16, 'W','A','T','E','R','S','W','I','T','C','H','_','T','B','D',' ' };
#endif
const uint8 zclWATERSWITCH_DateCode[] = { 16, '2','0','1','8','0','3','0','6',' ',' ',' ',' ',' ',' ',' ',' ' };
const uint8 zclWATERSWITCH_PowerSource = POWER_SOURCE_MAINS_1_PHASE;

uint8 zclWATERSWITCH_LocationDescription[17] = { 16, ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ' };
uint8 zclWATERSWITCH_PhysicalEnvironment = OUTPUT_LEVEL; //Control the output channcel
uint8 zclWATERSWITCH_DeviceEnable = DEVICE_ENABLED;
uint16 device_Status = 0;

//The nv configration
waterSwichConfig_t zclWATERSWITCH_NvConfig;

// The status value we care
uint8 zclWATERSWITCH_OnOff = PENDING;
uint8 zclWATERSWITCH_OnOffSwitch = AUTO_CONTROL;

uint16  zclWATERSWITCH_Temp = 0;
uint16  zclWATERSWITCH_Occupancy = 0;
uint16  zclWATERSWITCH_Flow = 0;

/*********************************************************************
 * ATTRIBUTE DEFINITIONS - Uses REAL cluster IDs
 */
CONST zclAttrRec_t zclWATERSWITCH_Attrs[WATERSWITCH_MAX_ATTRIBUTES] =
{
  // *** General Basic Cluster Attributes ***
  {
    ZCL_CLUSTER_ID_GEN_BASIC,             // Cluster IDs - defined in the foundation (ie. zcl.h)
    {  // Attribute record
      ATTRID_BASIC_HW_VERSION,            // Attribute ID - Found in Cluster Library header (ie. zcl_general.h)
      ZCL_DATATYPE_UINT8,                 // Data Type - found in zcl.h
      ACCESS_CONTROL_READ,                // Variable access control - found in zcl.h
      (void *)&zclWATERSWITCH_HWRevision  // Pointer to attribute variable
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_ZCL_VERSION,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ,
      (void *)&zclWATERSWITCH_ZCLVersion
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_MANUFACTURER_NAME,
      ZCL_DATATYPE_CHAR_STR,
      ACCESS_CONTROL_READ,
      (void *)zclWATERSWITCH_ManufacturerName
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_MODEL_ID,
      ZCL_DATATYPE_CHAR_STR,
      ACCESS_CONTROL_READ,
      (void *)zclWATERSWITCH_ModelId
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_DATE_CODE,
      ZCL_DATATYPE_CHAR_STR,
      ACCESS_CONTROL_READ,
      (void *)zclWATERSWITCH_DateCode
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_POWER_SOURCE,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ,
      (void *)&zclWATERSWITCH_PowerSource
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_LOCATION_DESC,
      ZCL_DATATYPE_CHAR_STR,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)zclWATERSWITCH_LocationDescription
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC, 
    { // Attribute record
      ATTRID_BASIC_PHYSICAL_ENV,
      ZCL_DATATYPE_UINT8,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWATERSWITCH_PhysicalEnvironment
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_DEVICE_ENABLED,
      ZCL_DATATYPE_UINT8,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWATERSWITCH_DeviceEnable
    }
  },
  
  // *** Global nv config Attribute ***
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_WATER_SWITCH_NV_CONFIG,
      ZCL_DATATYPE_OCTET_STR,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      NULL
    }
  },
#if DEVICE_TYPE==WS_COORDINATOR || DEVICE_TYPE==WS_PUMP
  // *** On / Off Cluster Attributes ***
  {
    ZCL_CLUSTER_ID_GEN_ON_OFF,
    { // Attribute record
      ATTRID_ON_OFF,
      ZCL_DATATYPE_BOOLEAN,
      ACCESS_CONTROL_READ,
      (void *)&zclWATERSWITCH_OnOff
    }
  },
#endif
#if DEVICE_TYPE==WS_COORDINATOR
  // *** On / Off Cluster Attributes ***
  {
    ZCL_CLUSTER_ID_GEN_ON_OFF_SWITCH_CONFIG,
    { // Attribute record
      ATTRID_ON_OFF_SWITCH_ACTIONS,
      ZCL_DATATYPE_UINT8,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclWATERSWITCH_OnOffSwitch
    }
  },
  // *** General Basic Cluster Attributes ***
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_ALARM_MASK,
      ZCL_DATATYPE_UINT16,
      (ACCESS_CONTROL_READ),
      (void *)&device_Status
    }
  },
#endif
#if DEVICE_TYPE==WS_COORDINATOR || DEVICE_TYPE==WS_TEMP
  // *** Temprature Cluster Attributes ***
  {
    ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT,
    { // Attribute record
      ATTRID_MS_TEMPERATURE_MEASURED_VALUE,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      (void *)&zclWATERSWITCH_Temp
    }
  },
  
  // *** Occupancy Cluster Attributes ***
  {
    ZCL_CLUSTER_ID_MS_OCCUPANCY_SENSING,
    { // Attribute record
      ATTRID_MS_OCCUPANCY_SENSING_CONFIG_OCCUPANCY,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      (void *)&zclWATERSWITCH_Occupancy
    }
  },
#endif
#if DEVICE_TYPE==WS_COORDINATOR || DEVICE_TYPE==WS_TEMP || DEVICE_TYPE==WS_PUMP
  // *** Flow Cluster Attributes ***
  {
    ZCL_CLUSTER_ID_MS_FLOW_MEASUREMENT,
    { // Attribute record
      ATTRID_MS_FLOW_MEASUREMENT_MEASURED_VALUE,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ,
      (void *)&zclWATERSWITCH_Flow
    }
  },
#endif
};

/*********************************************************************
 * SIMPLE DESCRIPTOR
 */
// This is the Cluster ID List and should be filled with Application
// specific cluster IDs.
#if DEVICE_TYPE==WS_COORDINATOR
cId_t zclWATERSWITCH_InClusterList[ZCLWATERSWITCH_MAX_INCLUSTERS] =
{
  ZCL_CLUSTER_ID_GEN_ON_OFF,
  ZCL_CLUSTER_ID_GEN_ON_OFF_SWITCH_CONFIG,
  ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT,
  ZCL_CLUSTER_ID_MS_OCCUPANCY_SENSING,
  ZCL_CLUSTER_ID_MS_FLOW_MEASUREMENT
};

cId_t zclWATERSWITCH_OutClusterList[ZCLWATERSWITCH_MAX_OUTCLUSTERS] =
{
  ZCL_CLUSTER_ID_GEN_ON_OFF,
  ZCL_CLUSTER_ID_GEN_ON_OFF_SWITCH_CONFIG
};
#elif DEVICE_TYPE==WS_PUMP
cId_t zclWATERSWITCH_InClusterList[ZCLWATERSWITCH_MAX_INCLUSTERS] =
{
  ZCL_CLUSTER_ID_GEN_ON_OFF
};

cId_t zclWATERSWITCH_OutClusterList[ZCLWATERSWITCH_MAX_OUTCLUSTERS] =
{
  ZCL_CLUSTER_ID_MS_FLOW_MEASUREMENT
};
#elif DEVICE_TYPE==WS_TEMP
cId_t* zclWATERSWITCH_InClusterList = NULL;

cId_t zclWATERSWITCH_OutClusterList[ZCLWATERSWITCH_MAX_OUTCLUSTERS] =
{
  ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT,
  ZCL_CLUSTER_ID_MS_OCCUPANCY_SENSING,
  ZCL_CLUSTER_ID_MS_FLOW_MEASUREMENT
};
#elif DEVICE_TYPE==WS_GATEWAY
cId_t* zclWATERSWITCH_InClusterList = NULL;

cId_t zclWATERSWITCH_OutClusterList[ZCLWATERSWITCH_MAX_OUTCLUSTERS] =
{
  ZCL_CLUSTER_ID_GEN_ON_OFF,
  ZCL_CLUSTER_ID_GEN_ON_OFF_SWITCH_CONFIG
};
#else

#endif

SimpleDescriptionFormat_t WaterSwitch_epDesc =
{
  WATERSWITCH_ENDPOINT,                  //  int Endpoint;
  WATERSWITCH_PROFID,                  //  uint16 AppProfId[2];
  WATERSWITCH_DEVICEID,      //  uint16 AppDeviceId[2];
  WATERSWITCH_DEVICE_VERSION,            //  int   AppDevVer:4;
  WATERSWITCH_FLAGS,                     //  int   AppFlags:4;
  ZCLWATERSWITCH_MAX_INCLUSTERS,         //  byte  AppNumInClusters;
#if DEVICE_TYPE==WS_TEMP || DEVICE_TYPE==WS_GATEWAY
  NULL, //  byte *pAppInClusterList;
#else
  (cId_t *)zclWATERSWITCH_InClusterList, //  byte *pAppInClusterList;
#endif
  ZCLWATERSWITCH_MAX_OUTCLUSTERS,        //  byte  AppNumInClusters;
  (cId_t *)zclWATERSWITCH_OutClusterList //  byte *pAppInClusterList;
};


SimpleDescriptionFormat_t WaterSwitch_custEpDesc =
{
  WATERSWITCH_CUSTOMIZED_ENDPOINT,                  //  int Endpoint;
  WATERSWITCH_CUSTOMIZED_PROFID,                  //  uint16 AppProfId[2];
  WATERSWITCH_CUSTOMIZED_DEVICEID,      //  uint16 AppDeviceId[2];
  WATERSWITCH_DEVICE_VERSION,            //  int   AppDevVer:4;
  WATERSWITCH_FLAGS,                     //  int   AppFlags:4;
  0,         //  byte  AppNumInClusters;
  NULL,
  0,        //  byte  AppNumInClusters;
  NULL //  byte *pAppInClusterList;
};

/*********************************************************************
 * GLOBAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/****************************************************************************
****************************************************************************/


