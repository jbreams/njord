Sets an environment variable either globally or in the local process.

## Arguments ##
| **Name** | **Type** | **Description** |
|:---------|:---------|:----------------|
| Name     | String   | The name of the environment variable to set. |
| Value    | String   | The value of the environment variable to set. |
| Addition | Boolean  | Specifies whether the variable should be added to a pre-existing variable. |
| Local    | Boolean  | Specifies whether the variable should affect only the current and child processes or the whole system. |

## Return Value ##

The function returns true on a successful load and returns false on an error. Call GetLastError for more information.

## Remarks ##

This function encapsulates both the [SetEnvironmentVariable](http://msdn.microsoft.com/en-us/library/ms686206(VS.85).aspx) function and the [method for changing system environment variables](http://msdn.microsoft.com/en-us/library/ms682653(VS.85).aspx) described on the MSDN.

The function will attempt to determine whether a variable is a [REG\_SZ or REG\_EXPAND\_SZ](http://msdn.microsoft.com/en-us/library/ms724884.aspx) by checking for the presence of a percent character.

If the addition argument is set to true, the function will retrieve the current value and append the new value separated by a semi-colon.