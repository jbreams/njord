MessageBox will display a modal win32 message box to the user and wait for it to return. The function encapsulates the [MessageBox](http://msdn.microsoft.com/en-us/library/ms645505(VS.85).aspx) function, and more information about flags and return values can be found on the [MSDN](http://msdn.microsoft.com/en-us/library/ms645505(VS.85).aspx) documentation.

## Parameters ##
| **Name** | **Type** | **Description** |
|:---------|:---------|:----------------|
| text     | String   | The message to be displayed in the message box |
| caption  | String   | The text to be displayed in the title bar of the message box (Defaults to "nJord")|
| flags    | UInt32   | A combination of flags that specify how to display the box (Defaults to MB\_OK) |

## Remarks ##
By default, passing one argument will display a dialog box with a single "Ok" button and a title bar that says nJord. Internally, the function sets the flags to MB\_TOPMOST | MB\_SETFOREGROUND to force the message box to the front of other windows that may have been created by nJord.