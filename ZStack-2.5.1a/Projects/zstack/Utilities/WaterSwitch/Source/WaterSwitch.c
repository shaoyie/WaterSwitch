/******************************************************************************
Filename:       WaterSwitch.c
Revised:        $Date: 2012-03-07 01:04:58 -0800 (Wed, 07 Mar 2012) $
Revision:       $Revision: 29656 $

Description:    Generic Application (no Profile).


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
PROVIDED 揂S IS?WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
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
******************************************************************************/

/*********************************************************************
This application isn't intended to do anything useful, it is
intended to be a simple example of an application's structure.

This application sends "Hello World" to another "Generic"
application every 5 seconds.  The application will also
receives "Hello World" packets.

The "Hello World" messages are sent/received as MSG type message.

This applications doesn't have a profile, so it handles everything
directly - itself.

Key control:
SW1:
SW2:  initiates end device binding
SW3:
SW4:  initiates a match description request
*********************************************************************/

/*********************************************************************
* INCLUDES
*/
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

/* RTOS */
#if defined( IAR_ARMCM3_LM )
#include "RTOS_App.h"
#endif  

/*********************************************************************
* MACROS
*/

#define zcl_MandatoryReportableAttribute( a ) ( TRUE /*a.attr.attrId == ATTRID_ON_OFF */)
/*********************************************************************
* CONSTANTS
*/

/*********************************************************************
* TYPEDEFS
*/

/*********************************************************************
* GLOBAL VARIABLES
*/

/*********************************************************************
* EXTERNAL VARIABLES
*/

/*********************************************************************
* EXTERNAL FUNCTIONS
*/

/*********************************************************************
* LOCAL VARIABLES
*/
byte WaterSwitch_TaskID;   // Task ID for internal task/event processing
// This variable will be received when
// WaterSwitch_Init() is called.
devStates_t WaterSwitch_NwkState;


byte WaterSwitch_TransID;  // This is the unique message ID (counter)

//The server point's addr
afAddrType_t WaterSwitch_DstAddr;
#if DEVICE_TYPE==WS_COORDINATOR
afAddrType_t WaterSwitch_TempDstAddr;
afAddrType_t WaterSwitch_PumpAddr;
afAddrType_t WaterSwitch_RemoteControlAddr;
#endif
//Bound with the server
byte bound=0;
uint16 pendingTask=0;
char strTemp[40];


// Event Endpoint to allow SYS_APP_MSGs
static endPointDesc_t WATERSWITCH_CustomizedEp =
{
  WATERSWITCH_CUSTOMIZED_ENDPOINT,                                 // Event endpoint
  &WaterSwitch_TaskID,
  (SimpleDescriptionFormat_t *)&WaterSwitch_custEpDesc,  // No Simple description for this test endpoint
  (afNetworkLatencyReq_t)0            // No Network Latency req
};

/*********************************************************************
* LOCAL FUNCTIONS
*/
static void WaterSwitch_ProcessZDOMsgs( zdoIncomingMsg_t *inMsg );
static void WaterSwitch_HandleKeys( byte shift, byte keys );
static void WaterSwitch_MessageMSGCB( afIncomingMSGPacket_t *pckt );
void RegularTask( void );
static void zclWATERSWITCH_BasicResetCB( void );
static void zclWATERSWITCH_IdentifyCB( zclIdentify_t *pCmd );
static void zclWATERSWITCH_IdentifyQueryRspCB(  zclIdentifyQueryRsp_t *pRsp );
static void zclWATERSWITCH_ProcessIdentifyTimeChange( void );

// Functions to process ZCL Foundation incoming Command/Response messages
static void zclWATERSWITCH_ProcessIncomingMsg( zclIncomingMsg_t *msg );
#ifdef ZCL_READ
uint8 zclWATERSWITCH_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg );
#endif
#ifdef ZCL_WRITE
uint8 zclWATERSWITCH_ProcessInWriteRspCmd( zclIncomingMsg_t *pInMsg );
#endif
#ifdef ZCL_REPORT
static uint8 zclWATERSWITCH_ProcessInConfigReportCmd( zclIncomingMsg_t *pInMsg );
#endif
static uint8 zclWATERSWITCH_ProcessInDefaultRspCmd( zclIncomingMsg_t *pInMsg );
#ifdef ZCL_DISCOVER
static uint8 zclWATERSWITCH_ProcessInDiscRspCmd( zclIncomingMsg_t *pInMsg );
#endif
void zclWATERSWITCH_OnOffCB( uint8 cmd );

// Local functions for setting reporting
uint8 zclWATERSWITCH_ProcessInReportCmd( zclIncomingMsg_t *pInMsg );

static void InitReportCmd(void);
static void InitDevice(uint8 task_id);
#if DEVICE_TYPE==WS_TEMP
void ProcessAdcBatchData(void);
#endif


zclReportCmd_t *pReportCmd;      // report command structure
//static zclReportCmd_t *pTempOccupReportCmd;      // report command structure
static uint8 binding=0;

/*********************************************************************
* ZCL General Profile Callback table
*/
static zclGeneral_AppCallbacks_t zclWATERSWITCH_CmdCallbacks =
{
  zclWATERSWITCH_BasicResetCB,     // Basic Cluster Reset command
  zclWATERSWITCH_IdentifyCB,       // Identify command
  zclWATERSWITCH_IdentifyQueryRspCB, // Identify Query Response command
  zclWATERSWITCH_OnOffCB,           // On / Off cluster command
  NULL,                         // Level Control Move to Level command
  NULL,                         // Level Control Move command
  NULL,                         // Level Control Step command
  NULL,                         // Group Response commands
  NULL,                         // Scene Store Request command
  NULL,                         // Scene Recall Request command
  NULL,                         // Scene Response commands
  NULL,                         // Alarm (Response) commands
  NULL,                         // RSSI Location commands
  NULL,                         // RSSI Location Response commands
};

#if defined( IAR_ARMCM3_LM )
static void WaterSwitch_ProcessRtosMessage( void );
#endif

/*********************************************************************
* NETWORK LAYER CALLBACKS
*/

/*********************************************************************
* PUBLIC FUNCTIONS
*/

/*********************************************************************
* @fn      WaterSwitch_Init
*
* @brief   Initialization function for the Generic App Task.
*          This is called during initialization and should contain
*          any application specific initialization (ie. hardware
*          initialization/setup, table initialization, power up
*          notificaiton ... ).
*
* @param   task_id - the ID assigned by OSAL.  This ID should be
*                    used to send messages and set timers.
*
* @return  none
*/
void WaterSwitch_Init( uint8 task_id )
{
  WaterSwitch_TaskID = task_id;
  WaterSwitch_NwkState = DEV_INIT;
  WaterSwitch_TransID = 0;
  
  //Register UART
  InitDevice(task_id);
  
  // Device hardware initialization can be added here or in main() (Zmain.c).
  // If the hardware is application specific - add it here.
  // If the hardware is other parts of the device add it in main().
  
  WaterSwitch_DstAddr.addrMode = (afAddrMode_t)AddrNotPresent;
  WaterSwitch_DstAddr.endPoint = 0;
  WaterSwitch_DstAddr.addr.shortAddr = 0;
  
  // This app is part of the Home Automation Profile
  zclHA_Init( &WaterSwitch_epDesc );
  
  // Register the ZCL General Cluster Library callback functions
  zclGeneral_RegisterCmdCallbacks( WATERSWITCH_ENDPOINT, &zclWATERSWITCH_CmdCallbacks );
  
  // Register the application's attribute list
  zcl_registerAttrList( WATERSWITCH_ENDPOINT, WATERSWITCH_MAX_ATTRIBUTES, zclWATERSWITCH_Attrs );
  
#if DEVICE_TYPE==WS_COORDINATOR
  zcl_registerReadWriteCB(WATERSWITCH_ENDPOINT, wsGloalConfigCB, NULL);
#endif
  
  // Register the Application to receive the unprocessed Foundation command/response messages
  zcl_registerForMsg( WaterSwitch_TaskID );
  
  // Register the endpoint description with the AF
  afRegister( &WATERSWITCH_CustomizedEp );
  
  // Register for all key events - This app will handle all key events
  RegisterForKeys( WaterSwitch_TaskID );
  
  // Update the display
#if defined ( LCD_SUPPORTED )
  HalLcdWriteString( "WaterSwitch", HAL_LCD_LINE_1 );
#endif
  
  //Register the ZDO callbacks that we care
  ZDO_RegisterForZDOMsg( WaterSwitch_TaskID, End_Device_Bind_rsp );
  ZDO_RegisterForZDOMsg( WaterSwitch_TaskID, Match_Desc_rsp );
  ZDO_RegisterForZDOMsg( WaterSwitch_TaskID, Active_EP_rsp );
  ZDO_RegisterForZDOMsg( WaterSwitch_TaskID, Simple_Desc_rsp );
  
#if defined( IAR_ARMCM3_LM )
  // Register this task with RTOS task initiator
  RTOS_RegisterApp( task_id, WATERSWITCH_RTOS_MSG_EVT );
#endif
  InitReportCmd();
}

void InitReportCmd(){
  int numOfAttr=1;
  //Create the OnOff report command
  pReportCmd = (zclReportCmd_t *)osal_mem_alloc( sizeof( zclReportCmd_t ) + ( numOfAttr * sizeof( zclReport_t ) ) );
  if ( pReportCmd != NULL )
  {
    pReportCmd->numAttr = numOfAttr;
  }
}

/*********************************************************************
* @fn      WaterSwitch_ProcessEvent
*
* @brief   Generic Application Task event processor.  This function
*          is called to process all events for the task.  Events
*          include timers, messages and any other user defined events.
*
* @param   task_id  - The OSAL assigned task ID.
* @param   events - events to process.  This is a bit map and can
*                   contain more than one event.
*
* @return  none
*/
uint16 WaterSwitch_ProcessEvent( uint8 task_id, uint16 events )
{
  afIncomingMSGPacket_t *MSGpkt;
  afDataConfirm_t *afDataConfirm;
  
  // Data Confirmation message fields
  byte sentEP;
  ZStatus_t sentStatus;
  byte sentTransID;       // This should match the value sent
  zAddrType_t dstAddr;
  (void)task_id;  // Intentionally unreferenced parameter
  
  if ( events & SYS_EVENT_MSG )
  {
    MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( WaterSwitch_TaskID );
    while ( MSGpkt )
    {
      switch ( MSGpkt->hdr.event )
      {
        
      case ZCL_INCOMING_MSG:
        // Incoming ZCL Foundation command/response messages
        zclWATERSWITCH_ProcessIncomingMsg( (zclIncomingMsg_t *)MSGpkt );
        break;
        
      case ZDO_MATCH_DESC_RSP_SENT:
#if DEVICE_TYPE==WS_COORDINATOR
        //Sent when the server responds to client's bind request
        LOG_OUTPUT(LOG_INFO, "MATCH_RSP for %x\n\r", ((ZDO_MatchDescRspSent_t *)MSGpkt)->nwkAddr);
        //Request the client's active endpoints
        ActiveEPReq(((ZDO_MatchDescRspSent_t *)MSGpkt)->nwkAddr);
#endif
        break;
        
      case ZDO_CB_MSG:
        WaterSwitch_ProcessZDOMsgs( (zdoIncomingMsg_t *)MSGpkt );
        break;
        
      case KEY_CHANGE:
        WaterSwitch_HandleKeys( ((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys );
        break;
        
      case AF_DATA_CONFIRM_CMD:
        // This message is received as a confirmation of a data packet sent.
        // The status is of ZStatus_t type [defined in ZComDef.h]
        // The message fields are defined in AF.h
        afDataConfirm = (afDataConfirm_t *)MSGpkt;
        sentEP = afDataConfirm->endpoint;
        sentStatus = afDataConfirm->hdr.status;
        sentTransID = afDataConfirm->transID;
        (void)sentEP;
        (void)sentTransID;
        
        // Action taken when confirmation is received.
        if ( sentStatus != ZSuccess )
        {
          // The data wasn't delivered -- Do something
        }
        break;
        
      case AF_INCOMING_MSG_CMD:
        WaterSwitch_MessageMSGCB( MSGpkt );
        break;
        
      case ZDO_STATE_CHANGE:
        WaterSwitch_NwkState = (devStates_t)(MSGpkt->hdr.status);
        if ( (WaterSwitch_NwkState == DEV_ZB_COORD)
            || (WaterSwitch_NwkState == DEV_ROUTER)
              || (WaterSwitch_NwkState == DEV_END_DEVICE) )
        { 
          LOG_OUTPUT(LOG_INFO, "Status changed\n\r");
#if DEVICE_TYPE==WS_COORDINATOR
          //Allow the devices to bind
          //zb_AllowBind(0xFF);
          afSetMatch(WaterSwitch_epDesc.EndPoint, TRUE);
          // Start regular task in a regular interval.
          osal_stop_timerEx( WaterSwitch_TaskID, WATERSWITCH_REGULAR_TASK_EVT );
          osal_start_timerEx( WaterSwitch_TaskID,
                             WATERSWITCH_REGULAR_TASK_EVT,
                             WATERSWITCH_REGULAR_TASK_TIMEOUT * 2 );  //Wait for device connect
#else
          // Initiate a Match Description Request (Service Discovery)
          if(binding==0){
            dstAddr.addrMode = AddrBroadcast;
            dstAddr.addr.shortAddr = NWK_BROADCAST_SHORTADDR;
            ZDP_MatchDescReq( &dstAddr, NWK_BROADCAST_SHORTADDR,
                             WATERSWITCH_PROFID,
                             ZCLWATERSWITCH_MAX_OUTCLUSTERS, zclWATERSWITCH_OutClusterList,   // Server's input is my output
                             ZCLWATERSWITCH_MAX_INCLUSTERS, zclWATERSWITCH_InClusterList,
                             TRUE );
            //Avoid duplicate bind request
            binding = 1;
            //Timeout for bind request
            osal_start_timerEx( WaterSwitch_TaskID,
                               WATERSWITCH_MATCH_SERVICE_EVT,
                               WATERSWITCH_REGULAR_TASK_TIMEOUT );
          }
#endif
        }
        break;
        
#if DEVICE_TYPE==WS_GATEWAY
      case CMD_SERIAL_MSG:
        HandelSerialData((mtOSALSerialData_t *)MSGpkt);
        break;
#endif
      default:
        break;
      }
      
      // Release the memory
      osal_msg_deallocate( (uint8 *)MSGpkt );
      
      // Next
      MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( WaterSwitch_TaskID );
    }
    
    // return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }
  
  // Send a message out - This event is generated by a timer
  //  (setup in WaterSwitch_Init()).
  if ( events & WATERSWITCH_REGULAR_TASK_EVT )
  {
    // Send "the" message
    RegularTask();
    
    // Setup to send message again
    osal_start_timerEx( WaterSwitch_TaskID,
                       WATERSWITCH_REGULAR_TASK_EVT,
                       WATERSWITCH_REGULAR_TASK_TIMEOUT );
    
    // return unprocessed events
    return (events ^ WATERSWITCH_REGULAR_TASK_EVT);
  }
  
  if ( events & WATERSWITCH_MATCH_SERVICE_EVT )
  {
    //Match service request timeout, allow to match again
    binding = 0;
    // return unprocessed events
    return (events ^ WATERSWITCH_MATCH_SERVICE_EVT);
  }
#if DEVICE_TYPE==WS_TEMP && defined USE_ADC
  if ( events & WATERSWITCH_HAL_ADC_TRANSFER_DONE_EVT )
  {
    //Match service request timeout, allow to match again
    
    ProcessAdcBatchData();
    // return unprocessed events
    return (events ^ WATERSWITCH_HAL_ADC_TRANSFER_DONE_EVT);
  }
#endif
#if DEVICE_TYPE==WS_COORDINATOR || DEVICE_TYPE==WS_PUMP
  if ( events & WATERSWITCH_VALVE_SERVICE_EVT )
  {
    //Delay for value control, stop the output driven
    StopValueOutput();
    // return unprocessed events
    return (events ^ WATERSWITCH_VALVE_SERVICE_EVT);
  }
#endif
#if DEVICE_TYPE==WS_COORDINATOR
  if ( events & WATERSWITCH_FIRE_OPERATION_EVT )
  {
    //Handel the fire operation remain things...release the keys
    HandelFireOperationEvents();
    // return unprocessed events
    return (events ^ WATERSWITCH_FIRE_OPERATION_EVT);
  }
#endif
#if defined( IAR_ARMCM3_LM )
  // Receive a message from the RTOS queue
  if ( events & WATERSWITCH_RTOS_MSG_EVT )
  {
    // Process message from RTOS queue
    WaterSwitch_ProcessRtosMessage();
    
    // return unprocessed events
    return (events ^ WATERSWITCH_RTOS_MSG_EVT);
  }
#endif
  if ( events & WATERSWITCH_CHECK_PENDING_TASK_EVT){
    //If has pending task
    if(pendingTask){
      CheckPendingTaskCB();
    }
    // return unprocessed events
    return (events ^ WATERSWITCH_CHECK_PENDING_TASK_EVT);
  }
  
  // Discard unknown events
  return 0;
}

/*********************************************************************
* Event Generation Functions
*/

/*********************************************************************
* @fn      WaterSwitch_ProcessZDOMsgs()
*
* @brief   Process response messages
*
* @param   none
*
* @return  none
*/
static void WaterSwitch_ProcessZDOMsgs( zdoIncomingMsg_t *inMsg )
{
  ZDO_ActiveEndpointRsp_t *pRsp;
  switch ( inMsg->clusterID )
  {
  case End_Device_Bind_rsp:
    LOG_OUTPUT(LOG_INFO,  "End_Device_Bind_rsp\n");
#if 0
    if ( ZDO_ParseBindRsp( inMsg ) == ZSuccess )
    {
      // Light LED
      HalLedSet( HAL_LED_4, HAL_LED_MODE_ON );
    }
#if defined( BLINK_LEDS )
    else
    {
      // Flash LED to show failure
      HalLedSet ( HAL_LED_4, HAL_LED_MODE_FLASH );
    }
#endif
#endif
    break;
  case Match_Desc_rsp:
#if DEVICE_TYPE!=WS_COORDINATOR
    //Get server's match response
    LOG_OUTPUT(LOG_INFO, "Match_Desc_rsp\n");
    binding = 0;
    
    pRsp = ZDO_ParseEPListRsp( inMsg );
    if ( pRsp )
    {
      if ( pRsp->status == ZSuccess && pRsp->cnt )
      {
        bound = 1;
        //Record server's addr info
        WaterSwitch_DstAddr.addrMode = (afAddrMode_t)Addr16Bit;
        WaterSwitch_DstAddr.addr.shortAddr = pRsp->nwkAddr;
        // Take the first endpoint, Can be changed to search through endpoints
        WaterSwitch_DstAddr.endPoint = pRsp->epList[0];
        LOG_OUTPUT(LOG_INFO,  "add:%x ep:%d\n\r", WaterSwitch_DstAddr.addr.shortAddr, WaterSwitch_DstAddr.endPoint);
        // Light LED
        //HalLedSet( HAL_LED_4, HAL_LED_MODE_ON );
      }
      osal_mem_free( pRsp );
    }
    
    // Start regular task in a regular interval.
    osal_stop_timerEx( WaterSwitch_TaskID, WATERSWITCH_REGULAR_TASK_EVT );
    osal_start_timerEx( WaterSwitch_TaskID,
                       WATERSWITCH_REGULAR_TASK_EVT,
                       WATERSWITCH_REGULAR_TASK_TIMEOUT );
#endif
    break;    
#if DEVICE_TYPE==WS_COORDINATOR
  case Active_EP_rsp:
    LOG_OUTPUT(LOG_INFO,  "Active_EP_rsp\n");
    {
      zAddrType_t dstAddr;
      ZDO_ActiveEndpointRsp_t *pRsp = ZDO_ParseEPListRsp( inMsg );
      dstAddr.addrMode = Addr16Bit;
      dstAddr.addr.shortAddr = pRsp->nwkAddr;
      if ( pRsp )
      {
        if ( pRsp->status == ZSuccess && pRsp->cnt )
        {
          for(int i=0;i<pRsp->cnt;i++){
            //Search through endpoints
            LOG_OUTPUT(LOG_INFO, "add:%x ep:%d\n\r", dstAddr.addr.shortAddr, pRsp->epList[i]);
            if(pRsp->epList[i]>0){
              //Request simple desc
              ZDP_SimpleDescReq(&dstAddr, pRsp->nwkAddr, pRsp->epList[i], TRUE);
            }
          }
        }
        osal_mem_free( pRsp );
      }
    } 
    break;
    
  case Simple_Desc_rsp:
    LOG_OUTPUT(LOG_INFO, "Simple_Desc_rsp\n");
    //Get the specified endpoint's description
    {
      ZDO_SimpleDescRsp_t *pSimpleDescRsp;   // pointer to received simple desc response
      afAddrType_t* pAddr = NULL;
      pSimpleDescRsp = (ZDO_SimpleDescRsp_t *)osal_mem_alloc( sizeof( ZDO_SimpleDescRsp_t ) );
      
      if(pSimpleDescRsp)
      {
        pSimpleDescRsp->simpleDesc.pAppInClusterList = NULL;
        pSimpleDescRsp->simpleDesc.pAppOutClusterList = NULL;
        
        ZDO_ParseSimpleDescRsp( inMsg, pSimpleDescRsp );
        
        if(pSimpleDescRsp->simpleDesc.AppDeviceId == ZCL_HA_DEVICEID_TEMPERATURE_SENSOR){
          pAddr=&WaterSwitch_TempDstAddr;
          LOG_OUTPUT(LOG_INFO,  "add:%x ep:%d is temp node\n\r", pSimpleDescRsp->nwkAddr, pSimpleDescRsp->simpleDesc.EndPoint);
        } else if(pSimpleDescRsp->simpleDesc.AppDeviceId == ZCL_HA_DEVICEID_PUMP){
          pAddr=&WaterSwitch_PumpAddr;
          LOG_OUTPUT(LOG_INFO, "add:%x ep:%d is pump node\n\r", pSimpleDescRsp->nwkAddr, pSimpleDescRsp->simpleDesc.EndPoint);
        } else if(pSimpleDescRsp->simpleDesc.AppDeviceId == ZCL_HA_DEVICEID_REMOTE_CONTROL){
          pAddr=&WaterSwitch_RemoteControlAddr;
          LOG_OUTPUT(LOG_INFO,  "add:%x ep:%d is remote control\n\r", pSimpleDescRsp->nwkAddr, pSimpleDescRsp->simpleDesc.EndPoint);
        }
        if(pAddr){
          //Record the device's addr info
          pAddr->addr.shortAddr = pSimpleDescRsp->nwkAddr;//0xFFFE; //bindAddr;
          pAddr->addrMode = afAddr16Bit;
          
          pAddr->panId = 0;                                    // Not an inter-pan message.
          pAddr->endPoint = pSimpleDescRsp->simpleDesc.EndPoint;  // Set the endpoint.
        }
        
      }
      
      // free memory for InClusterList
      if (pSimpleDescRsp->simpleDesc.pAppInClusterList)
      {
        osal_mem_free(pSimpleDescRsp->simpleDesc.pAppInClusterList);
      }
      
      // free memory for OutClusterList
      if (pSimpleDescRsp->simpleDesc.pAppOutClusterList)
      {
        osal_mem_free(pSimpleDescRsp->simpleDesc.pAppOutClusterList);
      }
      
      osal_mem_free( pSimpleDescRsp );
    }
    break;
#endif
  default:
    break;
  }
}

#if DEVICE_TYPE==WS_COORDINATOR
static uint32 lastClickStamp=0;
#endif
/*********************************************************************
* @fn      WaterSwitch_HandleKeys
*
* @brief   Handles all key events for this device.
*
* @param   shift - true if in shift/alt.
* @param   keys - bit field for key events. Valid entries:
*                 HAL_KEY_SW_4
*                 HAL_KEY_SW_3
*                 HAL_KEY_SW_2
*                 HAL_KEY_SW_1
*
* @return  none
*/
static void WaterSwitch_HandleKeys( uint8 shift, uint8 keys )
{
  zAddrType_t dstAddr;
  
  // Shift is used to make each button/switch dual purpose.
  if ( shift )
  {
  }
  else
  {
    if ( keys & HAL_KEY_SW_1 )
    {    
      
      LOG_OUTPUT(LOG_DEBUG, "Btn 1 pressed\n\r");
#if DEVICE_TYPE==WS_COORDINATOR
      uint32 now=osal_GetSystemClock();
      //Debounce
      if(now-lastClickStamp>1000){
        lastClickStamp=now;
        ToggleWorkMode();
      }
#endif
    }
    
    if ( keys & HAL_KEY_SW_2 )
    {
      LOG_OUTPUT(LOG_DEBUG, "Btn 2 pressed\n\r");
#if DEVICE_TYPE==WS_COORDINATOR
      uint32 now=osal_GetSystemClock();
      //Debounce
      if(now-lastClickStamp>1000){
        lastClickStamp=now;
        ToggleWaterSupplier();
      }
#endif
    }
    if ( keys & HAL_KEY_SW_3 )
    {
      
      LOG_OUTPUT(LOG_DEBUG,"Btn 3 pressed\n\r");
    }
    
    if ( keys & HAL_KEY_SW_4 )
    {
      
      LOG_OUTPUT(LOG_DEBUG, "Btn 4 pressed\n\r");
    }
  }
}

/*********************************************************************
* LOCAL FUNCTIONS
*/

/*********************************************************************
* @fn      WaterSwitch_MessageMSGCB
*
* @brief   Data message processor callback.  This function processes
*          any incoming data - probably from other devices.  So, based
*          on cluster ID, perform the intended action.
*
* @param   none
*
* @return  none
*/
static void WaterSwitch_MessageMSGCB( afIncomingMSGPacket_t *pkt )
{
  switch ( pkt->clusterId )
  {
  case WATERSWITCH_CLUSTERID:
    if(LOG_LEVEL_ENABLED(LOG_DEBUG)){   
#if DEVICE_TYPE==WS_GATEWAY
      //Check RSSI
      LOG_OUTPUT(LOG_DEBUG, "Rssi: %ddB\n\r", pkt->rssi);
#endif
      INFO_OUTPUT(LOG_DEBUG, pkt->cmd.Data, pkt->cmd.DataLength); //输出接收到的数据 
    }
    break;
  }
}

/*********************************************************************
* @fn      zclWATERSWITCH_BasicResetCB
*
* @brief   Callback from the ZCL General Cluster Library
*          to set all the Basic Cluster attributes to  default values.
*
* @param   none
*
* @return  none
*/
static void zclWATERSWITCH_BasicResetCB( void )
{
}

/*********************************************************************
* @fn      zclWATERSWITCH_IdentifyCB
*
* @brief   Callback from the ZCL General Cluster Library when
*          it received an Identity Command for this application.
*
* @param   srcAddr - source address and endpoint of the response message
* @param   identifyTime - the number of seconds to identify yourself
*
* @return  none
*/
static void zclWATERSWITCH_IdentifyCB( zclIdentify_t *pCmd )
{
  //  zclWATERSWITCH_IdentifyTime = pCmd->identifyTime;
  //  zclWATERSWITCH_ProcessIdentifyTimeChange();
}

/*********************************************************************
* @fn      zclWATERSWITCH_IdentifyQueryRspCB
*
* @brief   Callback from the ZCL General Cluster Library when
*          it received an Identity Query Response Command for this application.
*
* @param   srcAddr - source address
* @param   timeout - number of seconds to identify yourself (valid for query response)
*
* @return  none
*/
static void zclWATERSWITCH_IdentifyQueryRspCB(  zclIdentifyQueryRsp_t *pRsp )
{
  // Query Response (with timeout value)
  (void)pRsp;
  
}

/******************************************************************************
*
*  Functions for processing ZCL Foundation incoming Command/Response messages
*
*****************************************************************************/

/*********************************************************************
* @fn      zclWATERSWITCH_ProcessIncomingMsg
*
* @brief   Process ZCL Foundation incoming message
*
* @param   pInMsg - pointer to the received message
*
* @return  none
*/
static void zclWATERSWITCH_ProcessIncomingMsg( zclIncomingMsg_t *pInMsg )
{
  switch ( pInMsg->zclHdr.commandID )
  {
#ifdef ZCL_READ
  case ZCL_CMD_READ_RSP:
    zclWATERSWITCH_ProcessInReadRspCmd( pInMsg );
    break;
#endif
#ifdef ZCL_WRITE
  case ZCL_CMD_WRITE_RSP:
    zclWATERSWITCH_ProcessInWriteRspCmd( pInMsg );
    break;
#endif
#ifdef ZCL_REPORT
    // See ZCL Test Applicaiton (zcl_testapp.c) for sample code on Attribute Reporting
  case ZCL_CMD_CONFIG_REPORT:
    zclWATERSWITCH_ProcessInConfigReportCmd( pInMsg );
    break;
    
  case ZCL_CMD_CONFIG_REPORT_RSP:
    //zclWATERSWITCH_ProcessInConfigReportRspCmd( pInMsg );
    break;
    
  case ZCL_CMD_READ_REPORT_CFG:
    //zclWATERSWITCH_ProcessInReadReportCfgCmd( pInMsg );
    break;
    
  case ZCL_CMD_READ_REPORT_CFG_RSP:
    //zclWATERSWITCH_ProcessInReadReportCfgRspCmd( pInMsg );
    break;
    
  case ZCL_CMD_REPORT:
    zclWATERSWITCH_ProcessInReportCmd( pInMsg );
    break;
#endif
  case ZCL_CMD_DEFAULT_RSP:
    zclWATERSWITCH_ProcessInDefaultRspCmd( pInMsg );
    break;
#ifdef ZCL_DISCOVER
  case ZCL_CMD_DISCOVER_RSP:
    zclWATERSWITCH_ProcessInDiscRspCmd( pInMsg );
    break;
#endif
  default:
    break;
  }
  
  if ( pInMsg->attrCmd )
    osal_mem_free( pInMsg->attrCmd );
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
#if DEVICE_TYPE!=WS_GATEWAY
uint8 zclWATERSWITCH_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg )
{  
  return TRUE;
}
#endif
#endif // ZCL_READ

#ifdef ZCL_WRITE
/*********************************************************************
* @fn      zclWATERSWITCH_ProcessInWriteRspCmd
*
* @brief   Process the "Profile" Write Response Command
*
* @param   pInMsg - incoming message to process
*
* @return  none
*/
#if DEVICE_TYPE!=WS_GATEWAY
uint8 zclWATERSWITCH_ProcessInWriteRspCmd( zclIncomingMsg_t *pInMsg )
{
#if 0
  zclWriteRspCmd_t *writeRspCmd;
  uint8 i;
  
  writeRspCmd = (zclWriteRspCmd_t *)pInMsg->attrCmd;
  for (i = 0; i < writeRspCmd->numAttr; i++)
  {
    // Notify the device of the results of the its original write attributes
    // command.
  }
  
#endif
  return TRUE;
}
#endif
#endif // ZCL_WRITE


/*********************************************************************
* @fn      zclWATERSWITCH_ProcessInDefaultRspCmd
*
* @brief   Process the "Profile" Default Response Command
*
* @param   pInMsg - incoming message to process
*
* @return  none
*/
static uint8 zclWATERSWITCH_ProcessInDefaultRspCmd( zclIncomingMsg_t *pInMsg )
{
#if DEVICE_TYPE==WS_COORDINATOR || DEVICE_TYPE==WS_GATEWAY
  // Device is notified of the Default Response command.
  //Has pending task?
  if(pendingTask & TURN_ON_OFF_VALVE){
    zclDefaultRspCmd_t *defaultRspCmd = (zclDefaultRspCmd_t *)pInMsg->attrCmd;
    
    if(pInMsg->clusterId==ZCL_CLUSTER_ID_GEN_ON_OFF) {
      
      LOG_OUTPUT(LOG_INFO,  "Clear TURN_ON_OFF_VALVE pending task\n\r");
      ClearPendingTask(TURN_ON_OFF_VALVE);
#if DEVICE_TYPE==WS_GATEWAY
      //Report to the upper machine by UART
      SendSerialData(CMD0_WRITE_RSP, CMD1_WATER_SUPPLIER, NULL, 0);
#endif
    }
  }
#endif
  return TRUE;
}

#ifdef ZCL_REPORT
/*********************************************************************
* @fn      zclWATERSWITCH_ProcessInConfigReportCmd
*
* @brief   Process the "Profile" Configure Reporting Command
*
* @param   pInMsg - incoming message to process
* @param   logicalClusterID - logical cluster ID
*
* @return  TRUE if attribute was found in the Attribute list,
*          FALSE if not
*/
static uint8 zclWATERSWITCH_ProcessInConfigReportCmd( zclIncomingMsg_t *pInMsg )
{
#if 0
  zclCfgReportCmd_t *cfgReportCmd;
  zclCfgReportRec_t *reportRec;
  zclCfgReportRspCmd_t *cfgReportRspCmd;
  zclAttrRec_t attrRec;
  uint8 status;
  uint8 i, j = 0;
  
  cfgReportCmd = (zclCfgReportCmd_t *)pInMsg->attrCmd;
  
  // Allocate space for the response command
  cfgReportRspCmd = (zclCfgReportRspCmd_t *)osal_mem_alloc( sizeof ( zclCfgReportRspCmd_t ) + \
    ( cfgReportCmd->numAttr * sizeof ( zclCfgReportStatus_t) ) );
  if ( cfgReportRspCmd == NULL )
    return FALSE; // EMBEDDED RETURN
  
  // Process each Attribute Reporting Configuration record
  for ( i = 0; i < cfgReportCmd->numAttr; i++ )
  {
    reportRec = &(cfgReportCmd->attrList[i]);
    
    status = ZCL_STATUS_SUCCESS;
    
    if ( zclFindAttrRec( WATERSWITCH_ENDPOINT, pInMsg->clusterId, reportRec->attrID, &attrRec ) )
    {
      if ( reportRec->direction == ZCL_SEND_ATTR_REPORTS )
      {
        if ( reportRec->dataType == attrRec.attr.dataType )
        {
          // This the attribute that is to be reported
          if ( zcl_MandatoryReportableAttribute( attrRec ) == TRUE )
          {
#if 0
            if ( reportRec->minReportInt < ZCL_MIN_REPORTING_INTERVAL ||
                ( reportRec->maxReportInt != 0 && 
                 reportRec->maxReportInt < reportRec->minReportInt ) )
            {
              // Invalid fields
              status = ZCL_STATUS_INVALID_VALUE;
            }
            else
            {
              // Set the Min and Max Reporting Intervals and Reportable Change
              status = zclWATERSWITCH_SetAttrReportInterval( &attrRec.attr, reportRec );
            }
#endif
          }
          else
          {
            // Attribute cannot be reported
            status = ZCL_STATUS_UNREPORTABLE_ATTRIBUTE;
          }
        }
        else
        {
          // Attribute data type is incorrect
          status = ZCL_STATUS_INVALID_DATA_TYPE;
        }
      }
      else
      {
        // We shall expect reports of values of this attribute
        if ( zcl_MandatoryReportableAttribute( attrRec ) == TRUE )
        {    
          // Set the Timeout Period
          //          status = zclWATERSWITCH_SetAttrTimeoutPeriod( &attrRec.attr, reportRec );
        }
        else
        {
          // Reports of attribute cannot be received
          status = ZCL_STATUS_UNSUPPORTED_ATTRIBUTE;
        }
      }   
    }
    else
    {
      // Attribute is not supported
      status = ZCL_STATUS_UNSUPPORTED_ATTRIBUTE;
    }
    
    // If not successful then record the status
    if ( status != ZCL_STATUS_SUCCESS )
    {
      cfgReportRspCmd->attrList[j].status = status;
      cfgReportRspCmd->attrList[j].direction = reportRec->direction;
      cfgReportRspCmd->attrList[j++].attrID = reportRec->attrID;
    }
  } // for loop
  
  cfgReportRspCmd->numAttr = j;
  if ( cfgReportRspCmd->numAttr == 0 )
  {
    // Since all attributes were configured successfully, include a single 
    // attribute status record in the response command with the status field
    // set to SUCCESS and the attribute ID field omitted.
    cfgReportRspCmd->attrList[0].status = ZCL_STATUS_SUCCESS;
    cfgReportRspCmd->numAttr = 1;
  }
  
  // Send the response back
  zcl_SendConfigReportRspCmd( WATERSWITCH_ENDPOINT, &(pInMsg->srcAddr), 
                             pInMsg->clusterId, cfgReportRspCmd, ZCL_FRAME_SERVER_CLIENT_DIR, 
                             true, pInMsg->zclHdr.transSeqNum );
  osal_mem_free( cfgReportRspCmd );
#endif
  return TRUE ;
}
#endif // ZCL_REPORT

#ifdef ZCL_DISCOVER
/*********************************************************************
* @fn      zclWATERSWITCH_ProcessInDiscRspCmd
*
* @brief   Process the "Profile" Discover Response Command
*
* @param   pInMsg - incoming message to process
*
* @return  none
*/
static uint8 zclWATERSWITCH_ProcessInDiscRspCmd( zclIncomingMsg_t *pInMsg )
{
#if 0
  zclDiscoverRspCmd_t *discoverRspCmd;
  uint8 i;
  
  discoverRspCmd = (zclDiscoverRspCmd_t *)pInMsg->attrCmd;
  for ( i = 0; i < discoverRspCmd->numAttr; i++ )
  {
    // Device is notified of the result of its attribute discovery command.
  }
#endif
  return TRUE;
}
#endif // ZCL_DISCOVER

/*********************************************************************
* @fn      zclWATERSWITCH_ProcessInReportCmd
*
* @brief   Process the "Profile" Report Command
*
* @param   pInMsg - incoming message to process
*
* @return  none
*/
#if DEVICE_TYPE==WS_PUMP || DEVICE_TYPE==WS_TEMP
uint8 zclWATERSWITCH_ProcessInReportCmd( zclIncomingMsg_t *pInMsg )
{
  return TRUE;
}
#endif


#if defined( IAR_ARMCM3_LM )
/*********************************************************************
* @fn      WaterSwitch_ProcessRtosMessage
*
* @brief   Receive message from RTOS queue, send response back.
*
* @param   none
*
* @return  none
*/
static void WaterSwitch_ProcessRtosMessage( void )
{
  osalQueue_t inMsg;
  
  if ( osal_queue_receive( OsalQueue, &inMsg, 0 ) == pdPASS )
  {
    uint8 cmndId = inMsg.cmnd;
    uint32 counter = osal_build_uint32( inMsg.cbuf, 4 );
    
    switch ( cmndId )
    {
    case CMD_INCR:
      counter += 1;  /* Increment the incoming counter */
      /* Intentionally fall through next case */
      
    case CMD_ECHO:
      {
        userQueue_t outMsg;
        
        outMsg.resp = RSP_CODE | cmndId;  /* Response ID */
        osal_buffer_uint32( outMsg.rbuf, counter );    /* Increment counter */
        osal_queue_send( UserQueue1, &outMsg, 0 );  /* Send back to UserTask */
        break;
      }
      
    default:
      break;  /* Ignore unknown command */    
    }
  }
}
#endif

static void InitDevice(uint8 task_id){
  
  //uchar strTemp[20];
  WaterSwitch_InitIO();
  MT_UartInit ();
  MT_UartRegisterTaskID(task_id); 
  //sprintf(strTemp, "UartInit OK\n\r");
  //INFO_OUTPUT(strTemp, strlen(strTemp)); 
  
#if DEVICE_TYPE==WS_COORDINATOR
  //Init the global config value
  zclWATERSWITCH_NvConfig.tempCalibration = ENV_TEMP_CALIBRATION;
  zclWATERSWITCH_NvConfig.winterThreshold = 20;
  zclWATERSWITCH_NvConfig.winterSwtichTemp = 60;
  zclWATERSWITCH_NvConfig.summerSwitchTemp = 55;  
  
  //Init
  if(osal_nv_item_init(ZCD_NV_WATER_SWITCH_CONFIG, sizeof(zclWATERSWITCH_NvConfig), &zclWATERSWITCH_NvConfig)!=NV_OPER_FAILED){
    //Reload
    if(osal_nv_read(ZCD_NV_WATER_SWITCH_CONFIG, 0, sizeof(zclWATERSWITCH_NvConfig), &zclWATERSWITCH_NvConfig)!=SUCCESS){
      LOG_OUTPUT(LOG_ERROR,  "Reload global config error\r\n");
    }
  } else {
      LOG_OUTPUT(LOG_ERROR,  "Init nv_config error\r\n");
  }
#endif
}

#if DEVICE_TYPE==WS_TEMP||DEVICE_TYPE==WS_PUMP
void SendFlowReport(){ 
  
  // Set up the first attribute
  pReportCmd->attrList[0].attrID = ATTRID_MS_FLOW_MEASUREMENT_MEASURED_VALUE;
  pReportCmd->attrList[0].dataType = ZCL_DATATYPE_UINT16;
  pReportCmd->attrList[0].attrData = (uint8 *)&zclWATERSWITCH_Flow;
  
  //Send the report
  zcl_SendReportCmd( WATERSWITCH_ENDPOINT, &WaterSwitch_DstAddr,
                    ZCL_CLUSTER_ID_MS_FLOW_MEASUREMENT, pReportCmd,
                    ZCL_FRAME_SERVER_CLIENT_DIR, 1, 0 ); 
}
#endif

void CheckPendingTask(uint16 task){
  pendingTask |= task;
  osal_stop_timerEx( WaterSwitch_TaskID, WATERSWITCH_CHECK_PENDING_TASK_EVT );
  osal_start_timerEx( WaterSwitch_TaskID,
                     WATERSWITCH_CHECK_PENDING_TASK_EVT,
                     WATERSWITCH_DELAY_TIMEOUT );
}

void ClearPendingTask(uint16 task){
  pendingTask &= ~task;
}


/*********************************************************************
* @fn      RegularTask
*
* @brief   The regular task for a device.
*
* @param   none
*
* @return  none
*/
extern char adcDataSel;


void WriteAttrbuite(uint16 clusterID, uint16 attrID, uint8  dataType, uint8* data){
  
  uint8 attrNum=1;
  uint8 * pBuf=osal_mem_alloc( 1 + 5 * attrNum );
  if(pBuf!=NULL){
    zclWriteCmd_t *writeCmd=(zclWriteCmd_t *)pBuf;
    writeCmd->numAttr=attrNum;
    zclWriteRec_t* writeRec=&(writeCmd->attrList[0]);
    writeRec->attrID=attrID;
    writeRec->dataType=dataType;
    writeRec->attrData=data;             // Attribute ID (see ZigBee Cluster Library spec)
    zcl_SendWriteRequest( WATERSWITCH_ENDPOINT, &WaterSwitch_DstAddr, clusterID,
                         writeCmd, ZCL_CMD_WRITE, ZCL_FRAME_CLIENT_SERVER_DIR,
                         FALSE, 0 );
    osal_mem_free( pBuf );
  }
}

void ReadAttribute(uint16 clusterID, uint16 attrID){
  
  uint8 *pBuf;
  uint8 attrNum=1;
  
  zclReadCmd_t *readCmd;
  //Get attr
  pBuf = osal_mem_alloc( 1 + 2 * attrNum );
  if(pBuf!=NULL){
    readCmd = (zclReadCmd_t *)pBuf;
    readCmd->numAttr = 1; 
    readCmd->attrID[0] = attrID;                // Attribute ID (see ZigBee Cluster Library spec)
    zcl_SendRead(WATERSWITCH_ENDPOINT, &WaterSwitch_DstAddr, clusterID, readCmd, ZCL_FRAME_CLIENT_SERVER_DIR, TRUE, 0);  
    osal_mem_free( pBuf );
  }
}

//The node send raw data to coordinator for debug purpose
void AfSendData(uint16 shortAddr, uint8* data, uint16 length){
  afAddrType_t dstAddr; 
  dstAddr.addr.shortAddr = shortAddr;//0xFFFE; //bindAddr;
  dstAddr.addrMode = afAddr16Bit;
  dstAddr.endPoint = WATERSWITCH_CUSTOMIZED_ENDPOINT;
  if ( AF_DataRequest( &dstAddr, &WATERSWITCH_CustomizedEp, WATERSWITCH_CLUSTERID, length, data, &WaterSwitch_TransID, AF_DISCV_ROUTE, AF_DEFAULT_RADIUS ) == afStatus_SUCCESS ) 
  {
  }
  else
  {
    // Error occurred in request to send.
  }
}

/*********************************************************************
*/
