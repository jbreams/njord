Gets an environment variable from the process's environment block. Does not test whether it is stored locally or as a system variable.

## Arguments ##

| **Name** | **Type** | **Description** |
|:---------|:---------|:----------------|
| Name     | String   | The name of the environment variable to get. |

## Return Value ##

The function returns the value of the variable as a string on success and false on failure. Call GetLastError for more information on failure.

## Remarks ##

This function encapsulates the [GetEnvironmentVariable](http://msdn.microsoft.com/en-us/library/ms683188(VS.85).aspx) function.