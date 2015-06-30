Registers an event for a callback or wait notification from the current DOM. Registrations last the lifetime of the DOM document to which they apply, all registrations made with RegisterEvent are removed when [LoadData](LoadData.md) or [LoadURI](LoadURI.md) is called. The function returns true on success and false on failure.

## Parameters ##
| **Name** | **Type** | **Description** |
|:---------|:---------|:----------------|
| Target   | [DOMNode](DOMNode.md) | The DOM node that is the target of the event |
| Event    | String   | The event to be registered |
| Callback | String   | The name of the function to be called if the event fires |