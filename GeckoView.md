GeckoView encapsulates a single browser window.

## Methods ##
| **Name** | **Description** |
|:---------|:----------------|
| [LoadData](LoadData.md) | Loads a string representing and HTML document into the browser window and optionally waits for a DOM event. |
| [LoadURI](LoadURI.md) | Loads a URI into the browser window and optionally waits for a DOM event. |
| [Destroy](Destroy.md) | Destroys the window. |
| [RegisterEvent ](RegisterEvent.md) | Registers a DOM event for either a callback or a wait with the browser window. |
| [UnregisterEvent ](UnregisterEvent.md) | Unregisters a single or all registered events. |
| [WaitForStuff](WaitForStuff.md) | Waits for a single specified DOM event to become signaled. |
| [WaitForThings](WaitForThings.md) | Waits for all registered events to become signaled. |
| [GetDOM](GetDOM.md) | Returns the DOMDocument for the loaded page. |
| [GetInput](GetInput.md) | Gets the current value of a form input. |
| [GetElementByID](GetElementByID.md) | Gets a DOM node by its ID. |
| [ImportDOM](ImportDOM.md) | Imports a DOM node from a string and adds, but does not attach, it to the loaded document. |