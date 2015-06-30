The RegKey object encapsulates a single open registry key and allows basic operations on it.

## Methods ##
| **Name** | **Description** |
|:---------|:----------------|
| [Create](RegCreateKey.md) | Uses the existing object to create or open a new registry key |
| [Open](RegOpenKey.md) | Uses the existing object to open a new registry key |
| [SetValue](RegSetValue.md) | Sets a value on the registry key |
| [DeleteValue](RegDeleteValue.md) | Deletes a value from the registry key |
| [QueryValue](RegQueryValue.md) | Queries and returns a value from the registry key |
| [EnumValues](RegEnumValues.md) | Returns an array with the name of the values of the key |
| [EnumKeys](RegEnumKeys.md) | Returns an array with the names of the sub-keys of the key |
| [DeleteSubKey](RegDeleteSubKey.md) | Deletes a subkey from the key |
| [Flush](RegFlushKey.md) | Flushes changes made to a registry key to disk synchronously |
| [Close](RegCloseKey.md) | Closes they key |