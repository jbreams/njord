SetCurrentDirectory will set the current directory for the nJord process; after calling this function, relative paths will be processed based on the path provided to this function.

h2. Parameters
| **Name** | **Type** | **Description** |
|:---------|:---------|:----------------|
| Path     | String   | Specifies the path to set as the current directory. |

h2. Return value
The function will return true or false for success and failure respectively, call [GetLastError](GetLastError.md) for more information.