MsiInstallProduct will install an MSI package on the local computer. Encapsulates the [MsiInstallProduct](http://msdn.microsoft.com/en-us/library/aa370315%28VS.85%29.aspx) function from the MSI C API.

## Parameters ##
| **Name** | **Type** | **Description** |
|:---------|:---------|:----------------|
| MsiPath  | String   | The path to the MSI package to install |
| Properties | String or Object | Properties to pass to the installer |

## Return Value ##
The function will return true or false for success and failure. Call [GetLastError](GetLastError.md) to get information about any errors.

## Remarks ##
If you pass a string for the properties parameter, it will pass it as is to MSI. If you pass an object, it will enumerate the properties of the object and parse them into a single string of name/value pairs.