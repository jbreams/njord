UnregisterEvent will remove a single or all event registrations from the loaded document, depending on the parameters passed.

## Parameters ##
| **Name** | **Type** | **Description** |
|:---------|:---------|:----------------|
| Target   | [DOMNode](DOMNode.md) | The target node to remove the registration from. |
| Event    | String   | The event to be unregistered. |

The function will remove all events if no parameters are specified. If you specifiy one argument, you must specify the other - each call to RegisterEvent must have a corresponding UnregisterEvent call if you unregister the events by node.