#ifndef WATERSWITCH_IO_H
#define WATERSWITCH_IO_H

#include "hal_mcu.h"
#include "hal_defs.h"
#include "hal_types.h"

#include "hal_board_cfg.h"

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

//Input
#define TEMP_DECT_PIN   P1_2
#define TEMP_DECT_PIN_BV   BV(2)

#define CAPTURE_VALUE_TOLERANCE   60
#define WATER_LEVEL0   0.7
#define WATER_LEVEL1   0.62
#define WATER_LEVEL2   0.55
#define WATER_LEVEL3   0.443

#define P0_DURATION 0
#define P2_IN_DURATION 1
#define P2_OUT_DURATION 2
#define P1_DURATION 3

#elif DEVICE_TYPE==WS_GATEWAY

#define CMD0_READ 0
#define CMD0_READ_RSP 1
#define CMD0_WRITE 2
#define CMD0_WRITE_RSP 3

#define CMD1_TEMP  0
#define CMD1_OCCUPANCY 1
#define CMD1_DEVICE_STATUS 2
#define CMD1_WORK_MODE 3
#define CMD1_WATER_SUPPLIER 4
#define CMD1_SWITCH_OUTPUT 5
#else
#endif

void WaterSwitch_InitIO(void);

#if DEVICE_TYPE==WS_COORDINATOR || DEVICE_TYPE==WS_PUMP
void TurnOnOffValve(uint8 onoff);
void StopValueOutput(void);
#endif

#endif