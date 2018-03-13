#ifndef WATERSWITCH_IO_H
#define WATERSWITCH_IO_H

#include "hal_mcu.h"
#include "hal_defs.h"
#include "hal_types.h"

#include "WaterSwitch.h"

#define PORT_VALUE ACTIVE_HIGH

#if DEVICE_TYPE==WS_COORDINATOR
//Input
#define FIRE_ON_DETECT  P0_0
#define FIRE_ON_DETECT_BV  BV(0)

#define FIRE_USING_DETECT  P1_3
#define FIRE_USING_DETECT_BV  BV(3)
//Output
#define FIRE_TEMP_UP  P0_3
#define FIRE_TEMP_UP_BV  BV(3)
#define PUMP_POWER  P0_1
#define PUMP_POWER_BV BV(1)
#define PUMP_DIRECTION  P0_2
#define PUMP_DIRECTION_BV BV(2)

#define FIRE_TEMP_DOWN  P1_2
#define FIRE_TEMP_DOWN_BV  BV(2)

#define FIRE_SWITCH  P2_0
#define FIRE_SWITCH_BV  BV(0)

#elif DEVICE_TYPE==WS_PUMP
//Input
#define WATER_USING_DETECT  P1_3
#define WATER_USING_DETECT_BV  BV(3)
//Output
#define PUMP_POWER  P0_1
#define PUMP_POWER_BV BV(1)
#define PUMP_DIRECTION  P0_2
#define PUMP_DIRECTION_BV BV(2)


#elif DEVICE_TYPE==WS_TEMP

//Input
#define WATER_ENTERING_DETECT  P1_3
#define WATER_ENTERING_DETECT_BV  BV(3)

#elif DEVICE_TYPE==WS_GATEWAY
#else
#endif

void WaterSwitch_InitIO(void);

#endif