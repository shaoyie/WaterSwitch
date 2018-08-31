/**************************************************************************************************
  Filename:       hal_key.c
  Revised:        $Date: 2009-12-16 17:44:49 -0800 (Wed, 16 Dec 2009) $
  Revision:       $Revision: 21351 $

  Description:    This file contains the interface to the HAL KEY Service.


  Copyright 2006-2009 Texas Instruments Incorporated. All rights reserved.

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
 NOTE: If polling is used, the hal_driver task schedules the KeyRead()
       to occur every 100ms.  This should be long enough to naturally
       debounce the keys.  The KeyRead() function remembers the key
       state of the previous poll and will only return a non-zero
       value if the key state changes.

 NOTE: If interrupts are used, the KeyRead() function is scheduled
       25ms after the interrupt occurs by the ISR.  This delay is used
       for key debouncing.  The ISR disables any further Key interrupt
       until KeyRead() is executed.  KeyRead() will re-enable Key
       interrupts after executing.  Unlike polling, when interrupts
       are enabled, the previous key state is not remembered.  This
       means that KeyRead() will return the current state of the keys
       (not a change in state of the keys).

 NOTE: If interrupts are used, the KeyRead() fucntion is scheduled by
       the ISR.  Therefore, the joystick movements will only be detected
       during a pushbutton interrupt caused by S1 or the center joystick
       pushbutton.

 NOTE: When a switch like S1 is pushed, the S1 signal goes from a normally
       high state to a low state.  This transition is typically clean.  The
       duration of the low state is around 200ms.  When the signal returns
       to the high state, there is a high likelihood of signal bounce, which
       causes a unwanted interrupts.  Normally, we would set the interrupt
       edge to falling edge to generate an interrupt when S1 is pushed, but
       because of the signal bounce, it is better to set the edge to rising
       edge to generate an interrupt when S1 is released.  The debounce logic
       can then filter out the signal bounce.  The result is that we typically
       get only 1 interrupt per button push.  This mechanism is not totally
       foolproof because occasionally, signal bound occurs during the falling
       edge as well.  A similar mechanism is used to handle the joystick
       pushbutton on the DB.  For the EB, we do not have independent control
       of the interrupt edge for the S1 and center joystick pushbutton.  As
       a result, only one or the other pushbuttons work reasonably well with
       interrupts.  The default is the make the S1 switch on the EB work more
       reliably.

*********************************************************************/

/**************************************************************************************************
 *                                            INCLUDES
 **************************************************************************************************/
#include "hal_mcu.h"
#include "hal_defs.h"
#include "hal_types.h"
#include "hal_board.h"
#include "hal_drivers.h"
#include "hal_adc.h"
#include "hal_key.h"
#include "osal.h"

#if (defined HAL_KEY) && (HAL_KEY == TRUE)

/**************************************************************************************************
 *                                              MACROS
 **************************************************************************************************/

/**************************************************************************************************
 *                                            CONSTANTS
 **************************************************************************************************/
#define HAL_KEY_RISING_EDGE   0
#define HAL_KEY_FALLING_EDGE  1

#define HAL_KEY_DEBOUNCE_VALUE  25
#define HAL_KEY_POLLING_VALUE   100

/* CPU port interrupt */
#define HAL_KEY_CPU_PORT_0_IF P0IF
#define HAL_KEY_CPU_PORT_1_IF P1IF
#define HAL_KEY_CPU_PORT_2_IF P2IF

#define P0_IEN                IEN1   /* CPU interrupt mask register */
#define P0_IEN_BIT            BV(5) /* Mask bit for all of Port_0 */

#define P1_IEN                IEN2   /* CPU interrupt mask register */
#define P1_IEN_BIT            BV(4) /* Mask bit for all of Port_0 */

/* SW_6 is at P0.1 */
#define HAL_KEY_SW_6_PORT   P0
#define HAL_KEY_SW_6_BIT    BV(6)
#define HAL_KEY_SW_6_SEL    P0SEL
#define HAL_KEY_SW_6_DIR    P0DIR

/* edge interrupt */
#define HAL_KEY_SW_6_EDGEBIT  BV(0)
#define HAL_KEY_SW_6_EDGE     HAL_KEY_RISING_EDGE


/* SW_6 interrupts */
#define HAL_KEY_SW_6_IEN      P0IEN /* Port Interrupt Control register */
#define HAL_KEY_SW_6_IENBIT   BV(6) /* P0IEN - P0.6 enable/disable bit */
#define HAL_KEY_SW_6_ICTL     PICTL
#define HAL_KEY_SW_6_ICTLBIT  BV(0) /*Edge control for P0*/
#define HAL_KEY_SW_6_PXIFG    P0IFG /* Interrupt flag at source */

/* Joy stick move at P2.0 */
#define HAL_KEY_JOY_MOVE_PORT   P0
#define HAL_KEY_JOY_MOVE_BIT    BV(7)
#define HAL_KEY_JOY_MOVE_SEL    P0SEL
#define HAL_KEY_JOY_MOVE_DIR    P0DIR

/* edge interrupt */
#define HAL_KEY_JOY_MOVE_EDGEBIT  BV(0)
#define HAL_KEY_JOY_MOVE_EDGE     HAL_KEY_RISING_EDGE

/* Joy move interrupts */
#define HAL_KEY_JOY_MOVE_IEN      P0IEN  /* Port Interrupt Control register */
#define HAL_KEY_JOY_MOVE_IENBIT   BV(7) /* P0IEN - P0.7 enable/disable bit */
#define HAL_KEY_JOY_MOVE_ICTL     PICTL /* Port Interrupt Control register */
#define HAL_KEY_JOY_MOVE_ICTLBIT  BV(0) /*Edge control for P0*/
#define HAL_KEY_JOY_MOVE_PXIFG    P0IFG /* Interrupt flag at source */

#define HAL_KEY_JOY_CHN   HAL_ADC_CHANNEL_6

#if DEVICE_TYPE==WS_COORDINATOR
#define PUSH3_EDGEBIT  BV(1)
#define PUSH3_EDGE     HAL_KEY_RISING_EDGE
#define PUSH3_IEN      P1IEN  /* CPU interrupt mask register */
#define PUSH3_IENBIT   BV(2) /* P0IENL */
#define PUSH3_ICTL     PICTL /* Port Interrupt Control register */
#define PUSH3_ICTLBIT  BV(1) /*Edge control for P0*/
#define PUSH3_PXIFG    P1IFG /* Interrupt flag at source */

#define PUSH4_EDGEBIT  BV(1)
#define PUSH4_EDGE     HAL_KEY_RISING_EDGE
#define PUSH4_IEN      P1IEN  /* CPU interrupt mask register */
#define PUSH4_IENBIT   BV(3) /* P0IENL */
#define PUSH4_ICTL     PICTL /* Port Interrupt Control register */
#define PUSH4_ICTLBIT  BV(1) /*Edge control for P0*/
#define PUSH4_PXIFG    P1IFG /* Interrupt flag at source */
#endif

#if DEVICE_TYPE==WS_PUMP
#define PUSH3_EDGEBIT  BV(1)
#define PUSH3_EDGE     HAL_KEY_RISING_EDGE
#define PUSH3_IEN      P1IEN  /* CPU interrupt mask register */
#define PUSH3_IENBIT   BV(2) /* P0IENL */
#define PUSH3_ICTL     PICTL /* Port Interrupt Control register */
#define PUSH3_ICTLBIT  BV(1) /*Edge control for P0*/
#define PUSH3_PXIFG    P1IFG /* Interrupt flag at source */
#endif


/**************************************************************************************************
 *                                            TYPEDEFS
 **************************************************************************************************/


/**************************************************************************************************
 *                                        GLOBAL VARIABLES
 **************************************************************************************************/
static uint8 halKeySavedKeys;     /* used to store previous key state in polling mode */
static halKeyCBack_t pHalKeyProcessFunction;
static uint8 HalKeyConfigured;
bool Hal_KeyIntEnable;            /* interrupt enable/disable flag */
#if DEVICE_TYPE==WS_COORDINATOR
uint32 p0_0_time=0;
uint32 p1_3_time=0;
extern uint32 latestTimeStamp;
#endif

/**************************************************************************************************
 *                                        FUNCTIONS - Local
 **************************************************************************************************/
void halProcessKeyInterrupt(void);
uint8 halGetJoyKeyInput(void);



/**************************************************************************************************
 *                                        FUNCTIONS - API
 **************************************************************************************************/


/**************************************************************************************************
 * @fn      HalKeyInit
 *
 * @brief   Initilize Key Service
 *
 * @param   none
 *
 * @return  None
 **************************************************************************************************/
void HalKeyInit( void )
{
  /* Initialize previous key to 0 */
  halKeySavedKeys = 0;

  HAL_KEY_SW_6_SEL &= ~(HAL_KEY_SW_6_BIT);    /* Set pin function to GPIO */
  HAL_KEY_SW_6_DIR &= ~(HAL_KEY_SW_6_BIT);    /* Set pin direction to Input */

  HAL_KEY_JOY_MOVE_SEL &= ~(HAL_KEY_JOY_MOVE_BIT); /* Set pin function to GPIO */
  HAL_KEY_JOY_MOVE_DIR &= ~(HAL_KEY_JOY_MOVE_BIT); /* Set pin direction to Input */
#if DEVICE_TYPE==WS_COORDINATOR
  PUSH3_SEL &= ~(PUSH3_BV); /* Set pin function to GPIO */
  PUSH3_DIR &= ~(PUSH3_BV); /* Set pin direction to Input */
  PUSH4_SEL &= ~(PUSH4_BV); /* Set pin function to GPIO */
  PUSH4_DIR &= ~(PUSH4_BV); /* Set pin direction to Input */
  P1INP |= (PUSH3_BV|PUSH4_BV);
#endif
#if DEVICE_TYPE==WS_PUMP
  PUSH3_SEL &= ~(PUSH3_BV); /* Set pin function to GPIO */
  PUSH3_DIR &= ~(PUSH3_BV); /* Set pin direction to Input */
  P1INP |= (PUSH3_BV);
#endif

  /* Initialize callback function */
  pHalKeyProcessFunction  = NULL;

  /* Start with key is not configured */
  HalKeyConfigured = FALSE;
}


/**************************************************************************************************
 * @fn      HalKeyConfig
 *
 * @brief   Configure the Key serivce
 *
 * @param   interruptEnable - TRUE/FALSE, enable/disable interrupt
 *          cback - pointer to the CallBack function
 *
 * @return  None
 **************************************************************************************************/
void HalKeyConfig (bool interruptEnable, halKeyCBack_t cback)
{
  /* Enable/Disable Interrupt or */
  Hal_KeyIntEnable = interruptEnable;

  /* Register the callback fucntion */
  pHalKeyProcessFunction = cback;

  /* Determine if interrupt is enable or not */
  if (Hal_KeyIntEnable)
  {
    /* P0/P1 interrupt enable */
    P0_IEN|=P0_IEN_BIT;
    P1_IEN|=P1_IEN_BIT;

    PICTL &= ~(HAL_KEY_SW_6_EDGEBIT);    /* Clear the edge bit */
    /* For falling edge, the bit must be set. */
  #if (HAL_KEY_SW_6_EDGE == HAL_KEY_FALLING_EDGE)
    PICTL |= HAL_KEY_SW_6_EDGEBIT;
  #endif

    /* Interrupt configuration:
     * - Enable interrupt generation at the port
     * - Enable CPU interrupt
     * - Clear any pending interrupt
     */
    HAL_KEY_SW_6_IEN |= HAL_KEY_SW_6_IENBIT;
    HAL_KEY_SW_6_PXIFG = ~(HAL_KEY_SW_6_BIT);



    /* Rising/Falling edge configuratinn */

    HAL_KEY_JOY_MOVE_ICTL &= ~(HAL_KEY_JOY_MOVE_EDGEBIT);    /* Clear the edge bit */
    /* For falling edge, the bit must be set. */
  #if (HAL_KEY_JOY_MOVE_EDGE == HAL_KEY_FALLING_EDGE)
    HAL_KEY_JOY_MOVE_ICTL |= HAL_KEY_JOY_MOVE_EDGEBIT;
  #endif


    /* Interrupt configuration:
     * - Enable interrupt generation at the port
     * - Enable CPU interrupt
     * - Clear any pending interrupt
     */
    HAL_KEY_JOY_MOVE_IEN |= HAL_KEY_JOY_MOVE_IENBIT;
    HAL_KEY_JOY_MOVE_PXIFG = ~(HAL_KEY_JOY_MOVE_BIT);
    
#if DEVICE_TYPE==WS_COORDINATOR
        /* Rising/Falling edge configuratinn */

    PUSH3_ICTL &= ~(PUSH3_EDGEBIT);    /* Clear the edge bit */
    /* For falling edge, the bit must be set. */
  #if (PUSH3_EDGE == HAL_KEY_FALLING_EDGE)
    PUSH3_ICTL |= PUSH3_EDGEBIT;
  #endif


    /* Interrupt configuration:
     * - Enable interrupt generation at the port
     * - Enable CPU interrupt
     * - Clear any pending interrupt
     */
    PUSH3_IEN |= PUSH3_IENBIT;
    PUSH3_PXIFG = ~(PUSH3_BV);
    
    PUSH4_ICTL &= ~(PUSH4_EDGEBIT);    /* Clear the edge bit */
    /* For falling edge, the bit must be set. */
  #if (PUSH4_EDGE == HAL_KEY_FALLING_EDGE)
    PUSH4_ICTL |= PUSH4_EDGEBIT;
  #endif


    /* Interrupt configuration:
     * - Enable interrupt generation at the port
     * - Enable CPU interrupt
     * - Clear any pending interrupt
     */
    PUSH4_IEN |= PUSH4_IENBIT;
    PUSH4_PXIFG = ~(PUSH4_BV);
#endif
#if DEVICE_TYPE==WS_PUMP
        /* Rising/Falling edge configuratinn */

    PUSH3_ICTL &= ~(PUSH3_EDGEBIT);    /* Clear the edge bit */
    /* For falling edge, the bit must be set. */
  #if (PUSH3_EDGE == HAL_KEY_FALLING_EDGE)
    PUSH3_ICTL |= PUSH3_EDGEBIT;
  #endif


    /* Interrupt configuration:
     * - Enable interrupt generation at the port
     * - Enable CPU interrupt
     * - Clear any pending interrupt
     */
    PUSH3_IEN |= PUSH3_IENBIT;
    PUSH3_PXIFG = ~(PUSH3_BV);
#endif

    /* Do this only after the hal_key is configured - to work with sleep stuff */
    if (HalKeyConfigured == TRUE)
    {
      osal_stop_timerEx( Hal_TaskID, HAL_KEY_EVENT);  /* Cancel polling if active */
    }
  }
  else    /* Interrupts NOT enabled */
  {
    HAL_KEY_SW_6_ICTL &= ~(HAL_KEY_SW_6_ICTLBIT); /* don't generate interrupt */
    HAL_KEY_SW_6_IEN &= ~(HAL_KEY_SW_6_IENBIT);   /* Clear interrupt enable bit */

    osal_start_timerEx (Hal_TaskID, HAL_KEY_EVENT, HAL_KEY_POLLING_VALUE);    /* Kick off polling */
  }

  /* Key now is configured */
  HalKeyConfigured = TRUE;
}


/**************************************************************************************************
 * @fn      HalKeyRead
 *
 * @brief   Read the current value of a key
 *
 * @param   None
 *
 * @return  keys - current keys status
 **************************************************************************************************/
uint8 HalKeyRead ( void )
{
  uint8 keys = 0;

  if (HAL_PUSH_BUTTON1())
  {
    keys |= HAL_KEY_SW_6;
  }

/*  if ((HAL_KEY_JOY_MOVE_PORT & HAL_KEY_JOY_MOVE_BIT))  // Key is active low 
  {
    keys |= halGetJoyKeyInput();
  }
*/
  return keys;
}


/**************************************************************************************************
 * @fn      HalKeyPoll
 *
 * @brief   Called by hal_driver to poll the keys
 *
 * @param   None
 *
 * @return  None
 **************************************************************************************************/
void HalKeyPoll (void)
{
  uint8 keys = 0;

/* if ((HAL_KEY_JOY_MOVE_PORT & HAL_KEY_JOY_MOVE_BIT))  // Key is active HIGH 
  {
    keys = halGetJoyKeyInput();
  }
*/
  if (HAL_PUSH_BUTTON2())//S0
  {
    keys |= HAL_KEY_SW_2; 
  }
  if (HAL_PUSH_BUTTON1())//S1 
  {
    keys |= HAL_KEY_SW_1; 
  }
#if DEVICE_TYPE==WS_PUMP
  if (HAL_PUSH_BUTTON3())//S2
  {
    keys |= HAL_KEY_SW_3; 
  }
#endif
#if 0
  if (HAL_PUSH_BUTTON3())//S2
  {
    keys |= HAL_KEY_SW_3; 
  }
  if (HAL_PUSH_BUTTON4())//S3
  {
    keys |= HAL_KEY_SW_4; 
  }
#endif
  
  if (!Hal_KeyIntEnable)
  {
    if (keys == halKeySavedKeys)
    {
      /* Exit - since no keys have changed */
      return;
    }
    /* Store the current keys for comparation next time */
    halKeySavedKeys = keys;
  }
  else
  {
    /* Key interrupt handled here */
  }

  /* Invoke Callback if new keys were depressed */
  if (keys && (pHalKeyProcessFunction))
  {
    (pHalKeyProcessFunction) (keys, HAL_KEY_STATE_NORMAL);
  }
}

/**************************************************************************************************
 * @fn      halGetJoyKeyInput
 *
 * @brief   Map the ADC value to its corresponding key.
 *
 * @param   None
 *
 * @return  keys - current joy key status
 **************************************************************************************************/
uint8 halGetJoyKeyInput(void)
{
  /* The joystick control is encoded as an analog voltage.
   * Read the JOY_LEVEL analog value and map it to joy movement.
   */
  uint8 adc;
  uint8 ksave0 = 0;
  uint8 ksave1;

  /* Keep on reading the ADC until two consecutive key decisions are the same. */
  do
  {
    ksave1 = ksave0;    /* save previouse key reading */

    adc = HalAdcRead (HAL_KEY_JOY_CHN, HAL_ADC_RESOLUTION_8);

    if ((adc >= 2) && (adc <= 38))
    {
       ksave0 |= HAL_KEY_UP;
    }
    else if ((adc >= 74) && (adc <= 88))
    {
      ksave0 |= HAL_KEY_RIGHT;
    }
    else if ((adc >= 60) && (adc <= 73))
    {
      ksave0 |= HAL_KEY_LEFT;
    }
    else if ((adc >= 39) && (adc <= 59))
    {
      ksave0 |= HAL_KEY_DOWN;
    }
    else if ((adc >= 89) && (adc <= 100))
    {
      ksave0 |= HAL_KEY_CENTER;
    }
  } while (ksave0 != ksave1);

  return ksave0;
}





/**************************************************************************************************
 * @fn      halProcessKeyInterrupt
 *
 * @brief   Checks to see if it's a valid key interrupt, saves interrupt driven key states for
 *          processing by HalKeyRead(), and debounces keys by scheduling HalKeyRead() 25ms later.
 *
 * @param
 *
 * @return
 **************************************************************************************************/
void halProcessKeyInterrupt (void)
{
  bool valid=FALSE;

  if (HAL_KEY_SW_6_PXIFG & HAL_KEY_SW_6_BIT)  /* Interrupt Flag has been set */
  {
    HAL_KEY_SW_6_PXIFG &= ~(HAL_KEY_SW_6_BIT); /* Clear Interrupt Flag */
    valid = TRUE;
  }

  if (HAL_KEY_JOY_MOVE_PXIFG & HAL_KEY_JOY_MOVE_BIT)  /* Interrupt Flag has been set */
  {
    HAL_KEY_JOY_MOVE_PXIFG &= ~(HAL_KEY_JOY_MOVE_BIT); /* Clear Interrupt Flag */
    valid = TRUE;
  }
#if DEVICE_TYPE==WS_COORDINATOR
  if (PUSH3_PXIFG & PUSH3_BV)  /* Interrupt Flag has been set */
  {
    PUSH3_PXIFG &= ~(PUSH3_BV); /* Clear Interrupt Flag */
    //Record the trigger time
    p0_0_time=osal_GetSystemClock();
  }
  
  if (PUSH4_PXIFG & PUSH4_BV)  /* Interrupt Flag has been set */
  {
    PUSH4_PXIFG &= ~(PUSH4_BV); /* Clear Interrupt Flag */
    //Record the trigger time
    p1_3_time=osal_GetSystemClock();
  }
#endif
  
#if DEVICE_TYPE==WS_PUMP
  if (PUSH3_PXIFG & PUSH3_BV)  /* Interrupt Flag has been set */
  {
    PUSH3_PXIFG &= ~(PUSH3_BV); /* Clear Interrupt Flag */
    valid = TRUE;
  }
#endif

  if (valid)
  {
    osal_start_timerEx (Hal_TaskID, HAL_KEY_EVENT, HAL_KEY_DEBOUNCE_VALUE);
  }
}

/**************************************************************************************************
 * @fn      HalKeyEnterSleep
 *
 * @brief  - Get called to enter sleep mode
 *
 * @param
 *
 * @return
 **************************************************************************************************/
void HalKeyEnterSleep ( void )
{
}

/**************************************************************************************************
 * @fn      HalKeyExitSleep
 *
 * @brief   - Get called when sleep is over
 *
 * @param
 *
 * @return  - return saved keys
 **************************************************************************************************/
uint8 HalKeyExitSleep ( void )
{
  /* Wake up and read keys */
  return ( HalKeyRead () );
}

/***************************************************************************************************
 *                                    INTERRUPT SERVICE ROUTINE
 ***************************************************************************************************/

/**************************************************************************************************
 * @fn      halKeyPort0Isr
 *
 * @brief   Port0 ISR
 *
 * @param
 *
 * @return
 **************************************************************************************************/
HAL_ISR_FUNCTION( halKeyPort0Isr, P0INT_VECTOR )
{
  if (HAL_KEY_SW_6_PXIFG & (HAL_KEY_SW_6_BIT | HAL_KEY_JOY_MOVE_BIT))
  {
    halProcessKeyInterrupt();
  }

  /*
    Clear the CPU interrupt flag for Port_0
    PxIFG has to be cleared before PxIF
  */
  HAL_KEY_SW_6_PXIFG = 0;
  HAL_KEY_CPU_PORT_0_IF = 0;
}

/**************************************************************************************************
 * @fn      halKeyPort0Isr
 *
 * @brief   Port0 ISR
 *
 * @param
 *
 * @return
 **************************************************************************************************/
HAL_ISR_FUNCTION( halKeyPort1Isr, P1INT_VECTOR )
{
#if DEVICE_TYPE==WS_COORDINATOR
  if (PUSH4_PXIFG & (PUSH4_BV | PUSH3_BV))
  {
    halProcessKeyInterrupt();
  }

  /*
    Clear the CPU interrupt flag for Port_0
    PxIFG has to be cleared before PxIF
  */
  PUSH4_PXIFG = 0;
#endif
#if DEVICE_TYPE==WS_PUMP
  if (PUSH3_PXIFG & PUSH3_BV)
  {
    halProcessKeyInterrupt();
  }

  /*
    Clear the CPU interrupt flag for Port_0
    PxIFG has to be cleared before PxIF
  */
  PUSH3_PXIFG = 0;
#endif
  HAL_KEY_CPU_PORT_1_IF = 0;
}



/**************************************************************************************************
 * @fn      halKeyPort2Isr
 *
 * @brief   Port2 ISR
 *
 * @param
 *
 * @return
 **************************************************************************************************/
HAL_ISR_FUNCTION( halKeyPort2Isr, P2INT_VECTOR )
{
  if (HAL_KEY_JOY_MOVE_PXIFG & HAL_KEY_JOY_MOVE_BIT)
  {
    halProcessKeyInterrupt();
  }

  /*
    Clear the CPU interrupt flag for Port_2
    PxIFG has to be cleared before PxIF
    Notes: P2_1 and P2_2 are debug lines.
  */
  HAL_KEY_JOY_MOVE_PXIFG = 0;
  HAL_KEY_CPU_PORT_2_IF = 0;
}

#else


void HalKeyInit(void){}
void HalKeyConfig(bool interruptEnable, halKeyCBack_t cback){}
uint8 HalKeyRead(void){ return 0;}
void HalKeyPoll(void){}

#endif /* HAL_KEY */





/**************************************************************************************************
**************************************************************************************************/



