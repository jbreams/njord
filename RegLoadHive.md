RegLoadHive will load a specified registry hive into either HKEY\_USERS or HKEY\_LOCAL\_MACHINE. On success, it will return true; on failure it will return false - more information about the failure can be retrieved with [GetLastError](GetLastError.md).

## Parameters ##
| **Name** | **Type** | **Description** |
|:---------|:---------|:----------------|
| parent   | UInt32   | Either HKEY\_USERS or HKEY\_LOCAL\_MACHINE |
| fileName | String   | The path to the registry hive to load |
| keyName  | String   | The name of the key this hive will be loaded into in the registry (optional) |

## Remarks ##
This function requires administrative privileges to work properly, it will attempt to adjust the process token to have the SE\_RESTORE\_NAME and SE\_BACKUP\_NAME privileges enabled. The key name is optional, and the name of the key will be the name of the file if not specified.