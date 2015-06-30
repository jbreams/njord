LoadURI loads a URI into the browser and optionally waits for a specified DOM event. On success it returns true, on failure it returns false.

## Parameters ##
| **Name** | **Type** | **Description**|
|:---------|:---------|:---------------|
| URI      |  String  | The URI to load into the browser |
| EventTarget | String   | The ID attribute of the element you want to wait on an event. |
| EventName | String   | The name of the event you want to wait for. |

The event parameters are optional.