WaitForThings will wait for an unspecified event to fire, or for all the events to fire. If the function is not waiting for all events to fire, it will return the DOM node whose event fired. Otherwise, it will return true to indicate success. If none of the events fired within the timeout period, it will return false.

## Parameters ##
All parameters are optional.
| **Name** | **Type** | **Description** |
|:---------|:---------|:----------------|
| WaitForAll | Boolean  | Determines whether the function should wait for a single event or all events to fire before returning |
| Timeout  | UInt32   | The number of milliseconds to wait before timing out. Defaults to infinity. |