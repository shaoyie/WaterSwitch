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
#define uint unsigned int
#define uchar unsigned char

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
afAddrType_t WaterSwitch_TempDstAddr;
afAddrType_t WaterSwitch_PumpAddr;
afAddrType_t WaterSwitch_RemoteControlAddr;


// Event Endpoint to allow SYS_APP_MSGs
static endPointDesc_t WATERSWITCH_TestEp =
{
  100,                                 // Event endpoint
  &WaterSwitch_TaskID,
  (SimpleDescriptionFormat_t *)NULL,  // No Simple description for this test endpoint
  (afNetworkLatencyReq_t)0            // No Network Latency req
};

/*********************************************************************
* LOCAL FUNCTIONS
*/
static void WaterSwitch_ProcessZDOMsgs( zdoIncomingMsg_t *inMsg );
static void WaterSwitch_HandleKeys( byte shift, byte keys );
static void WaterSwitch_MessageMSGCB( afIncomingMSGPacket_t *pckt );
static void RegularTask( void );
static void zclWATERSWITCH_BasicResetCB( void );
static void zclWATERSWITCH_IdentifyCB( zclIdentify_t *pCmd );
static void zclWATERSWITCH_IdentifyQueryRspCB(  zclIdentifyQueryRsp_t *pRsp );
static void zclWATERSWITCH_ProcessIdentifyTimeChange( void );

// Functions to process ZCL Foundation incoming Command/Response messages
static void zclWATERSWITCH_ProcessIncomingMsg( zclIncomingMsg_t *msg );
#ifdef ZCL_READ
static uint8 zclWATERSWITCH_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg );
#endif
#ifdef ZCL_WRITE
static uint8 zclWATERSWITCH_ProcessInWriteRspCmd( zclIncomingMsg_t *pInMsg );
#endif
#ifdef ZCL_REPORT
static uint8 zclWATERSWITCH_ProcessInConfigReportCmd( zclIncomingMsg_t *pInMsg );
#endif
static uint8 zclWATERSWITCH_ProcessInDefaultRspCmd( zclIncomingMsg_t *pInMsg );
#ifdef ZCL_DISCOVER
static uint8 zclWATERSWITCH_ProcessInDiscRspCmd( zclIncomingMsg_t *pInMsg );
#endif

// Local functions for setting reporting
static uint8 zclWATERSWITCH_ProcessInReportCmd( zclIncomingMsg_t *pInMsg );

static void InitReportCmd(void);
static void ActiveEPReq(uint16 bindAddr);
static void InitDevice(uint8 task_id);

static zclReportCmd_t *pOnOffReportCmd;      // report command structure
//static zclReportCmd_t *pTempOccupReportCmd;      // report command structure
static uint8 binding=0;

#if DEVICE_TYPE==WS_COORDINATOR
static uint8 device_Status = 0;
static uint32 tick=0;
static uint32 lastTempTick=0;
static uint32 lastPumpTick=0;
#endif
/*********************************************************************
* ZCL General Profile Callback table
*/
static zclGeneral_AppCallbacks_t zclWATERSWITCH_CmdCallbacks =
{
  zclWATERSWITCH_BasicResetCB,     // Basic Cluster Reset command
  zclWATERSWITCH_IdentifyCB,       // Identify command
  zclWATERSWITCH_IdentifyQueryRspCB, // Identify Query Response command
  NULL,                         // On / Off cluster command - not needed.
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
  
  // Register the Application to receive the unprocessed Foundation command/response messages
  zcl_registerForMsg( WaterSwitch_TaskID );
  
  // Register the endpoint description with the AF
  afRegister( &WATERSWITCH_TestEp );
  
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
  int numOnOffAttr=1;
  //Create the OnOff report command
  pOnOffReportCmd = (zclReportCmd_t *)osal_mem_alloc( sizeof( zclReportCmd_t ) + ( numOnOffAttr * sizeof( zclReport_t ) ) );
  if ( pOnOffReportCmd != NULL )
  {
    pOnOffReportCmd->numAttr = numOnOffAttr;
    
    // Set up the first attribute
    pOnOffReportCmd->attrList[0].attrID = ATTRID_ON_OFF;
    pOnOffReportCmd->attrList[0].dataType = ZCL_DATATYPE_UINT8;
    pOnOffReportCmd->attrList[0].attrData = &zclWATERSWITCH_OnOff; 
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
  uchar strTemp[30];
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
        //Sent when the server responds to client's bind request
        sprintf(strTemp, "MATCH_RSP for %x\n\r", ((ZDO_MatchDescRspSent_t *)MSGpkt)->nwkAddr);
        HalUARTWrite(1, strTemp,strlen(strTemp));
        //Request the client's active endpoints
        ActiveEPReq(((ZDO_MatchDescRspSent_t *)MSGpkt)->nwkAddr);
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
          
          HalUARTWrite(1,"Status changed\n\r", sizeof("Status changed\n\r")); 
#if DEVICE_TYPE==WS_COORDINATOR
          //Allow the devices to bind
          //zb_AllowBind(0xFF);
          afSetMatch(WaterSwitch_epDesc.EndPoint, TRUE);
          // Start regular task in a regular interval.
          osal_stop_timerEx( WaterSwitch_TaskID, WATERSWITCH_REGULAR_TASK_EVT );
          osal_start_timerEx( WaterSwitch_TaskID,
                             WATERSWITCH_REGULAR_TASK_EVT,
                             WATERSWITCH_REGULAR_TASK_TIMEOUT );
#else
          // Initiate a Match Description Request (Service Discovery)
          if(binding==0){
            dstAddr.addrMode = AddrBroadcast;
            dstAddr.addr.shortAddr = NWK_BROADCAST_SHORTADDR;
            ZDP_MatchDescReq( &dstAddr, NWK_BROADCAST_SHORTADDR,
                             WATERSWITCH_PROFID,
                             ZCLWATERSWITCH_MAX_INCLUSTERS, zclWATERSWITCH_InClusterList,
                             ZCLWATERSWITCH_MAX_OUTCLUSTERS, zclWATERSWITCH_OutClusterList,   // No incoming clusters to bind
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
  uchar strTemp[30];
  switch ( inMsg->clusterID )
  {
  case End_Device_Bind_rsp:
    HalUARTWrite(1,"End_Device_Bind_rsp\n", sizeof("End_Device_Bind_rsp\n")); 
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
    break;
#if DEVICE_TYPE==WS_COORDINATOR
  case Active_EP_rsp:
    HalUARTWrite(1,"Active_EP_rsp\n\r", sizeof("Active_EP_rsp\n\r")); 
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
            // Take the first endpoint, Can be changed to search through endpoints
            sprintf(strTemp, "add:%x ep:%d\n\r", dstAddr.addr.shortAddr, pRsp->epList[i]);
            HalUARTWrite(1, strTemp,strlen(strTemp));
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
#endif
  case Match_Desc_rsp:
#if DEVICE_TYPE!=WS_COORDINATOR
    //Get server's match response
    HalUARTWrite(1,"Match_Desc_rsp\n\r", sizeof("Match_Desc_rsp\n\r")); 
    binding = 0;
    zAddrType_t dstAddr;
    
#if 0
    //The cluster id need to be check in detail
    if ( APSME_BindRequest( WATERSWITCH_ENDPOINT,
                           zclWATERSWITCH_OutClusterList[0], &dstAddr, pRsp->epList[0] ) == ZSuccess )
    {
      osal_start_timerEx( ZDAppTaskID, ZDO_NWK_UPDATE_NV, 250 );
      
      // Find IEEE addr
      ZDP_IEEEAddrReq( pRsp->nwkAddr, ZDP_ADDR_REQTYPE_SINGLE, 0, 0 );
      
    }
#endif
    ZDO_ActiveEndpointRsp_t *pRsp = ZDO_ParseEPListRsp( inMsg );
    if ( pRsp )
    {
      if ( pRsp->status == ZSuccess && pRsp->cnt )
      {
        WaterSwitch_DstAddr.addrMode = (afAddrMode_t)Addr16Bit;
        WaterSwitch_DstAddr.addr.shortAddr = pRsp->nwkAddr;
        // Take the first endpoint, Can be changed to search through endpoints
        WaterSwitch_DstAddr.endPoint = pRsp->epList[0];
        sprintf(strTemp, "add:%x ep:%d\n\r", WaterSwitch_DstAddr.addr.shortAddr, WaterSwitch_DstAddr.endPoint);
        HalUARTWrite(1, strTemp,strlen(strTemp));
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
  case Simple_Desc_rsp:
    HalUARTWrite(1,"Simple_Desc_rsp\n\r", sizeof("Simple_Desc_rsp\n\r")); 
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
        } else if(pSimpleDescRsp->simpleDesc.AppDeviceId == ZCL_HA_DEVICEID_PUMP){
          pAddr=&WaterSwitch_PumpAddr;
        } else if(pSimpleDescRsp->simpleDesc.AppDeviceId == ZCL_HA_DEVICEID_REMOTE_CONTROL){
          pAddr=&WaterSwitch_RemoteControlAddr;
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
  }
  break;
#endif
default:
  break;
}
}

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
  HalUARTWrite(1,"Key pressed\n", sizeof("Key pressed\n")); 
  
  // Shift is used to make each button/switch dual purpose.
  if ( shift )
  {
    if ( keys & HAL_KEY_SW_1 )
    {
    }
    if ( keys & HAL_KEY_SW_2 )
    {
    }
    if ( keys & HAL_KEY_SW_3 )
    {
    }
    if ( keys & HAL_KEY_SW_4 )
    {
    }
  }
  else
  {
    if ( keys & HAL_KEY_SW_1 )
    {
      // Since SW1 isn't used for anything else in this application...
#if defined( SWITCH1_BIND )
      // we can use SW1 to simulate SW2 for devices that only have one switch,
      keys |= HAL_KEY_SW_2;
#elif defined( SWITCH1_MATCH )
      // or use SW1 to simulate SW4 for devices that only have one switch
      keys |= HAL_KEY_SW_4;
#endif
    }
    
    if ( keys & HAL_KEY_SW_2 )
    {
      HalLedSet ( HAL_LED_4, HAL_LED_MODE_OFF );
      
      // Initiate an End Device Bind Request for the mandatory endpoint
      dstAddr.addrMode = Addr16Bit;
      dstAddr.addr.shortAddr = 0x0000; // Coordinator
      ZDP_EndDeviceBindReq( &dstAddr, NLME_GetShortAddr(),
                           WaterSwitch_epDesc.EndPoint,
                           WATERSWITCH_PROFID,
                           0, NULL,   // No incoming clusters to bind
                           ZCLWATERSWITCH_MAX_OUTCLUSTERS, zclWATERSWITCH_OutClusterList,
                           TRUE );
    }
    
    if ( keys & HAL_KEY_SW_3 )
    {
    }
    
    if ( keys & HAL_KEY_SW_4 )
    {
      HalLedSet ( HAL_LED_4, HAL_LED_MODE_OFF );
      // Initiate a Match Description Request (Service Discovery)
      dstAddr.addrMode = AddrBroadcast;
      dstAddr.addr.shortAddr = NWK_BROADCAST_SHORTADDR;
      ZDP_MatchDescReq( &dstAddr, NWK_BROADCAST_SHORTADDR,
                       WATERSWITCH_PROFID,
                       ZCLWATERSWITCH_MAX_OUTCLUSTERS, zclWATERSWITCH_OutClusterList,
                       0, NULL,   // No incoming clusters to bind
                       TRUE );
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
#if 0
  case WATERSWITCH_CLUSTERID:
    // "the" message
#if defined( LCD_SUPPORTED )
    HalLcdWriteScreen( (char*)pkt->cmd.Data, "rcvd" );
#elif defined( WIN32 )
    WPRINTSTR( pkt->cmd.Data );
#endif
#endif
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
  HalUARTWrite(1,"IdentifyQueryRsp\n", sizeof("IdentifyQueryRsp\n")); 
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
static uint8 zclWATERSWITCH_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclReadRspCmd_t *readRspCmd;
  uint8 i;
  zclReadRspStatus_t* prsp;
  uint8  *data;
  
  readRspCmd = (zclReadRspCmd_t *)pInMsg->attrCmd;
  for (i = 0; i < readRspCmd->numAttr; i++)
  {
    prsp= &(readRspCmd->attrList[i]);
    if(prsp->status == ZCL_STATUS_SUCCESS){
      data = prsp->data;
      // Notify the originator of the results of the original read attributes
      // attempt and, for each successfull request, the value of the requested
      // attribute
      switch(pInMsg->clusterId)  
      {  
      case ZCL_CLUSTER_ID_GEN_BASIC:  
        {  
          switch(prsp->attrID){
          case ATTRID_BASIC_MANUFACTURER_NAME:
            HalUARTWrite(1,data, strlen(data));
            break;
          case ATTRID_BASIC_MODEL_ID:
            HalUARTWrite(1,data, strlen(data));
            break;
          default:
            break;
          }
        }
        // And that's OK !  
      }  
      break;  
    }  else {
      //error
    }
  }
  
  return TRUE;
}
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
static uint8 zclWATERSWITCH_ProcessInWriteRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclWriteRspCmd_t *writeRspCmd;
  uint8 i;
  
  writeRspCmd = (zclWriteRspCmd_t *)pInMsg->attrCmd;
  for (i = 0; i < writeRspCmd->numAttr; i++)
  {
    // Notify the device of the results of the its original write attributes
    // command.
  }
  
  return TRUE;
}
#endif // ZCL_WRITE

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
  
  return TRUE ;
}
#endif // ZCL_REPORT

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
  // zclDefaultRspCmd_t *defaultRspCmd = (zclDefaultRspCmd_t *)pInMsg->attrCmd;
  // Device is notified of the Default Response command.
  (void)pInMsg;
  return TRUE;
}

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
  zclDiscoverRspCmd_t *discoverRspCmd;
  uint8 i;
  
  discoverRspCmd = (zclDiscoverRspCmd_t *)pInMsg->attrCmd;
  for ( i = 0; i < discoverRspCmd->numAttr; i++ )
  {
    // Device is notified of the result of its attribute discovery command.
  }
  
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
static uint8 zclWATERSWITCH_ProcessInReportCmd( zclIncomingMsg_t *pInMsg )
{
  zclReportCmd_t *reportCmd;
  zclReport_t *reportRec;
  uint8 i;
  uint8 *OnOffState;
  
  reportCmd = (zclReportCmd_t *)pInMsg->attrCmd;
  for (i = 0; i < reportCmd->numAttr; i++)
  {
    // Device is notified of the latest values of the attribute of another device.
    reportRec = &(reportCmd->attrList[i]);
    
    if ( reportRec->attrID == ATTRID_ON_OFF )
    {
      static uint8 DlvdCnt=0;
      char lcd_buf[32] = "On/Off Attr ";
      OnOffState = reportRec->attrData;
      
#ifdef LCD_SUPPORTED       
      // print out value of On/Off Attr
      HalLcdWriteString("Zigbee Coord Sw", HAL_LCD_LINE_1);  
      lcd_buf[12] = (DlvdCnt++ & 0x7) + 48; //count 0-7 and then rap around
      HalLcdWriteString(lcd_buf, HAL_LCD_LINE_2);      
      
      if ( *OnOffState == LIGHT_ON )
        HalLcdWriteString("ON", HAL_LCD_LINE_3);
      else
        HalLcdWriteString("OFF", HAL_LCD_LINE_3);
#endif //LCD_SUPPORTED 
    }
    
#if 0    
    if(OnOffReportTimeoutPeriod)
    {
      //Reset the timer. Time out in ms so *1000 to get seconds     
      osal_stop_timerEx( zclWATERSWITCH_TaskID, WATERSWITCH_ON_OFF_REPORT_ATTRIBUTE_TIMEOUT_EVT );
      osal_start_timerEx( zclWATERSWITCH_TaskID, WATERSWITCH_ON_OFF_REPORT_ATTRIBUTE_TIMEOUT_EVT, OnOffReportTimeoutPeriod*1000 );    
    }
    
#endif
  }
  return TRUE;
}

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

//Get the specified device's active endpoint list
void ActiveEPReq(uint16 bindAddr){
  zAddrType_t dstAddr;
  uint attrNum = 1;
  
  //    uint8 txOptions=0;
  
  dstAddr.addr.shortAddr = bindAddr;//0xFFFE; //bindAddr;
  dstAddr.addrMode = afAddr16Bit;
  
  //Get Request a device's endpoint list
  HalUARTWrite(1,"ZDP_ActiveEPReq\n", sizeof("ZDP_ActiveEPReq\n")); 
  ZDP_ActiveEPReq(&dstAddr, bindAddr, TRUE);
  
  // Send the message
  //zclGeneral_SendOnOff_CmdToggle( WATERSWITCH_ENDPOINT, &dstAddr, false, 0 );
  //zclGeneral_SendIdentifyQuery(WATERSWITCH_ENDPOINT, &dstAddr, false, 0);
  
}

static void InitDevice(void){
  
  MT_UartInit ();
  MT_UartRegisterTaskID(task_id); 
  HalUARTWrite(1,"UartInit OK\n", sizeof("UartInit OK\n")); 
#if DEVICE_TYPE==WS_COORDINATOR
#elif DEVICE_TYPE==WS_PUMP

#elif DEVICE_TYPE==WS_TEMP
#elif DEVICE_TYPE==WS_GATEWAY
#else

#endif
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
static void RegularTask( void )
{
#if DEVICE_TYPE==WS_COORDINATOR
  tick++;
  
#elif DEVICE_TYPE==WS_PUMP
#elif DEVICE_TYPE==WS_TEMP
  
#elif DEVICE_TYPE==WS_GATEWAY
#else
#endif
  
#if 0
  char theMessageData[] = "Hello World";
  if ( AF_DataRequest( &WaterSwitch_DstAddr, &WaterSwitch_epDesc,
                      WATERSWITCH_CLUSTERID,
                      (byte)osal_strlen( theMessageData ) + 1,
                      (byte *)&theMessageData,
                      &WaterSwitch_TransID,
                      AF_DISCV_ROUTE, AF_DEFAULT_RADIUS ) == afStatus_SUCCESS )
  {
    // Successfully requested to be sent.
  }
  else
  {
    // Error occurred in request to send.
  }
#endif
}

#if 0
//Just for reference
void ReadAttr(){
  
  uint8 *pBuf;
  uint8 attrNum=1;
  afAddrType_t dstAddr;          
  zclReadCmd_t *readCmd;
  
  sprintf(strTemp, "add:%x ep:%d is TempDev\n\r", pSimpleDescRsp->nwkAddr, pSimpleDescRsp->simpleDesc.EndPoint);
  HalUARTWrite(1, strTemp,strlen(strTemp));
  
  //Get attr
  pBuf = osal_mem_alloc( attrNum + 16 * attrNum );
  if(pBuf!=NULL){
    dstAddr.addr.shortAddr = pSimpleDescRsp->nwkAddr;//0xFFFE; //bindAddr;
    dstAddr.addrMode = afAddr16Bit;
    
    dstAddr.panId = 0;                                    // Not an inter-pan message.
    dstAddr.endPoint = pSimpleDescRsp->simpleDesc.EndPoint;  // Set the endpoint.
    readCmd = (zclReadCmd_t *)pBuf;
    readCmd->numAttr = 1;  
    readCmd->attrID[0] = ATTRID_BASIC_MODEL_ID;                // Attribute ID (see ZigBee Cluster Library spec)
    zcl_SendRead(WATERSWITCH_ENDPOINT, &dstAddr, ZCL_CLUSTER_ID_GEN_BASIC, readCmd, ZCL_FRAME_CLIENT_SERVER_DIR, TRUE, 0);  
    osal_mem_free( pBuf );
  }
}
#endif
/*********************************************************************
*/
