GeckoCreateView will create a new [GeckoView](GeckoView.md) and return the object, or false on failure. This function assumes that GeckoInit has been called and has succeeded, calls to this function where the GRE has not been initialized will result in a deadlocked critical section - don't do that.

## Parameters ##
| Name | Type | Description |
|:-----|:-----|:------------|
| cX   | UInt16 | The width of the new browser window |
| cY   | UInt16 | The height of the new browser window |
| allowClose | Bool | Specifies whether the window message handler should allow the window to be closed by the user |
| x    | UInt16 | The x coordinate for the new window |
| y    | UInt16 | The y coordinate of the new window |

## Remarks ##
The function will automatically center the new window on the primary display of the system. The allowClose parameter should be used with extreme care, because if the window is closed and the browser is destroyed, and further calls - including any registered events - will cause an access violation. IF the x, y coordinates are not specified the window will be centered.