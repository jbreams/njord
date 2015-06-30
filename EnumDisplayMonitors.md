EnumDisplayMonitors will return an array of objects with information about the monitors attached to the system. It is provided to give multimonitor support for creating Gecko windows. The function takes no arguments. The object will have the following properties.

| **Name** | **Type** |**Description** |
|:---------|:---------|:---------------|
| isPrimary | Boolean  | Whether the monitor represented is the primary monitor. |
| rcWork   | RECT object | The rectangle in which windows can be created in virtual coordinates. |
| rcMonitor | RECT object | The physical coordinates of the monitor. |

The RECT objects have four properties: top, left, bottom, and right which specifies a rectangle on a monitor. They are all 16-bit unsigned values.