RegUnloadHive will unload a hive that was previously loaded with [RegLoadHive](RegLoadHive.md). On success it will return true, and will return false on failure - more information about the failure can be retrieved with [GetLastError](GetLastError.md).

## Parameters ##
| **Name** | **Type** | **Description** |
|:---------|:---------|:----------------|
| parent   | UInt32   | Either HKEY\_USERS or HKEY\_LOCAL\_MACHINE |
| keyName  | String   | The name of the subkey to be unloaded. |

## Remarks ##

This function requires administrative privileges to work properly, it will attempt to adjust the process token to have the SE\_RESTORE\_NAME and SE\_BACKUP\_NAME privileges enabled. The keyName should be the name used to load the hive in [RegLoadHive](RegLoadHive.md).