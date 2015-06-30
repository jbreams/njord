MsiSetInternalUI will set the level of user interface to use during calls to [MsiInstallProduct](MsiInstallProduct.md). The function encapsulates, to an extent, [MsiSetInternalUI](http://msdn.microsoft.com/en-us/library/aa370389%28VS.85%29.aspx) from the Microsoft MSI C API.

## Parameters ##
| **Name** | **Type** | **Description** |
|:---------|:---------|:----------------|
| UILevel  | UInt32   | The level the UI should display, see remarks |
| ParentWindow | Object   | The progress dialog that should be set as the parent window for MSI installs (optional) |

## Return Value ##
The function returns nothing.

## Remarks ##
The function will accept the following value for the UILevel
| **Name** | **Description** |
|:---------|:----------------|
| INSTALLUILEVEL\_NONE | There should be no user interface, totally silent |
| INSTALLUILEVEL\_BASIC | Display a progess window and error messages |
| INSTALLUILEVEL\_REDUCED | Don't display wizard dialog boxes |
| INSTALLUILEVEL\_FULL | Display the full user interface for the package |

The ParentWindow parameter accepts a [xDriverUI](xDriverUI.md) object and takes the HWND value for the parent of all MSI operations.

It is the responsibility of the MSI package to honor the UI level. Some poorly written packages may still display a UI regardless of what was set in this call.