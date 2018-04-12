A Zigbee based gas water heater and solar water heater controller.
ZStack-2.5.1a is the Zigbee project under IAR.
QtProj is the QT UI project.

There are 4 types of zigbee devices in this system.
Coordinator, which is the central controller.
A temperature sensor, which is combined with the salor water heater controller.
A pump controller.
A remote controller, which accept serial commands from the Raspberry Pi.

The first one should be Zigbee Coordinator, the other three should be Zigbee EndDevice.
Modify the DEVICE_TYPE in the WaterSwitchDeviceType.h to select the device type, and then select the corresponding build option in IAR (CoordinatorEB/EndDeviceEB)