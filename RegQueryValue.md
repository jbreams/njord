QueryValue will query a value from the registry and return it as an appropriate JavaScript type. It takes one argument, the  name of the value to be queried. If the function succeeds, it will return true; otherwise, it will return false - more information about the failure can be retrieved from [GetLastError](GetLastError.md).

## Type Mappings ##
The function will try to match up the value from the registry to the appropriate JavaScript type. Below is a table that maps what registry types map to which JavaScript types.

| **Registry Type** | **JavaScript Type** |
|:------------------|:--------------------|
| REG\_DWORD        | jsdouble (unsigned 32-bit integer) |
| REG\_QWORD        | jsdouble (unsigned 64-bit integer) |
| REG\_MULTI\_SZ    | array object of strings |
| REG\_SZ           | string              |
| REG\_EXPAND\_SZ   | string              |
| REG\_LINK         | string              |

Other values are not supported and will not be processed and the function will return null.