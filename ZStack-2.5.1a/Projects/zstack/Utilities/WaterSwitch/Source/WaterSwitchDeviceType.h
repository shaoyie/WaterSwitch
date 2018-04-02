#ifndef WATERSWITCHDEVICETYPE_H
#define WATERSWITCHDEVICETYPE_H

#define DEVICE_TYPE   WS_COORDINATOR
//Only choose one of the four
#define WS_COORDINATOR  1
#define WS_PUMP   2
#define WS_TEMP   3
#define WS_GATEWAY  4

#define DEBUG
//#define CAPTURE_RAW_DATA
//#define USE_ADC

/*********************************************************************
 * CONSTANTS
 */

  
#define WATERSWITCH_CLUSTERID 0xFC01  //For internal data transfer
  
// These constants are only for example and should be changed to the
// device's needs
#define WATERSWITCH_ENDPOINT           20
#define WATERSWITCH_CUSTOMIZED_ENDPOINT           100

#define WATERSWITCH_PROFID             ZCL_HA_PROFILE_ID //0x0104
#define WATERSWITCH_CUSTOMIZED_PROFID             0xbf11
#define WATERSWITCH_CUSTOMIZED_DEVICEID           5
  
#if DEVICE_TYPE==WS_COORDINATOR
  
#define WATERSWITCH_DEVICEID           ZCL_HA_DEVICEID_ON_OFF_SWITCH   
#define ZCLWATERSWITCH_MAX_INCLUSTERS        5
#define ZCLWATERSWITCH_MAX_OUTCLUSTERS      2
#define WATERSWITCH_MAX_ATTRIBUTES        16

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
  
#elif DEVICE_TYPE==WS_GATEWAY
#define WATERSWITCH_DEVICEID           ZCL_HA_DEVICEID_REMOTE_CONTROL
#define ZCLWATERSWITCH_MAX_INCLUSTERS        0
#define ZCLWATERSWITCH_MAX_OUTCLUSTERS       2
#define WATERSWITCH_MAX_ATTRIBUTES        10
#else
#endif


// Send Message Timeout
#define WATERSWITCH_REGULAR_TASK_TIMEOUT   5000     // Every 5 seconds
#define WATERSWITCH_FIRE_POWER_DELAY_TIMEOUT   5000
#define WATERSWITCH_VALVE_TIMEOUT   15000
#define WATERSWITCH_SET_TEMP_TIMEOUT   15000
  
#define WATERSWITCH_DELAY_TIMEOUT   1000
#define WATERSWITCH_PRESS_KEY_TIMEOUT   300

// Application Events (OSAL) - These are bit weighted definitions.
#define WATERSWITCH_REGULAR_TASK_EVT       1
#define WATERSWITCH_MATCH_SERVICE_EVT       (1<<2)
#define WATERSWITCH_VALVE_SERVICE_EVT       (1<<3)
#define WATERSWITCH_HAL_ADC_TRANSFER_DONE_EVT       (1<<4)
#define WATERSWITCH_FIRE_OPERATION_EVT       (1<<5)
#define WATERSWITCH_CHECK_PENDING_TASK_EVT       (1<<6)

//The pending task that we need to check result
#define TURN_ON_OFF_VALVE             1
#define SET_WORKMODE                  1<<1

#if defined( IAR_ARMCM3_LM )
#define WATERSWITCH_RTOS_MSG_EVT       0x0002
#endif  
  
#define PUMP_OFF                       0x00
#define PUMP_ON                        0x01

#define RELEASE_KEY                       0x00
#define PRESS_KEY                        0x01
  
#define SALOR_OFF                       0x00
#define SALOR_ON                        0x01
#define PENDING                         0xff
  
#define AUTO_CONTROL                    0x00
#define MANUAL_CONTROL                  0x01
  
#define TEMP_WORKING                    0x01
#define PUMP_WORKING                    0x02
#define TEMP_ERROR                      (1<<8)
#define PUMP_ERROR                      (2<<8)
  
#define ERROR_MASK                      0xff00
#define WORKING_STATUS_MASK             0x00ff
  
#define KEY_FIRE_SWITCH                 1
#define KEY_FIRE_TEMP_UP                 1<<1

#define ENV_TEMP_CALIBRATION               293   //K

#define ADC_CAPTURE_COUNT               200
#define ADC_CHANNEL_COUNT               3

#define OUTPUT_SEIRAL                       0
#define OUTPUT_AF_MESSAGE                   1     //Redirect the debug output

#endif