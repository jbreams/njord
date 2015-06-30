RegDeleteKey will recursively delete a key from the registry - for example `RegDeleteKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft");` would delete all the values and all the subkeys in the Microsoft key of the SOFWARE hive. The function returns true on success and false on failure.

## Parameters ##
| **Name** | **Type** | **Description** |
|:---------|:---------|:----------------|
| hive     | UInt32 or [RegKey](RegKey.md) object | The parent key for the key to be deleted. |
| subKey   | String   | The name of the key to be recursively deleted. |