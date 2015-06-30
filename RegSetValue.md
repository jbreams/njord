SetValue will set a value on a registry key. If the value did not exist, it is created; otherwise, it is overwritten.

## Parameters ##
| **Name** | **Type** | **Description** |
|:---------|:---------|:----------------|
| Name     | String   | The name of the value to set |
| Value    | Varies   | The value to set, depends on the type of value |
| Type     | UInt32   | A constant that tells what type of value to set |

## Return value ##

The function returns true on success and false on failure, more information can be retrieved using [GetLastError](GetLastError.md).

## Remarks ##

SetValue tries to determine what kind of value you want to set by the type of JavaScript value is passed. For example, if you want to set a REG\_SZ (single null-terminated string), you need only send a string to the function, or if you want REG\_DWORD (unsigned long), you need only pass an integer. The function will accept the following types to registry values.

| **Name** | **Description** |
|:---------|:----------------|
| REG\_DWORD | An unsigned 32-bit integer |
| REG\_SZ  | A null-terminated string |
| REG\_EXPAND\_SZ | A null-terminated string with environment variables |
| REG\_MULTI\_SZ | An array of null-terminated strings |

If no suitable conversion is found, the function will convert it to a REG\_SZ. For REG\_EXPAND\_SZ, you must specify type parameter. For REG\_MULTI\_SZ, you must pass an array of values - they will all be converted into strings.