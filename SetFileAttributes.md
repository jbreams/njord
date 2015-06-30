SetFileAttributes will set the attributes flag on a specified file. For more information on attributes that can be set, see the MSDN documentation on [GetFileAttributes](http://msdn.microsoft.com/en-us/library/aa364944(VS.85).aspx). The function will return true on success and false on failure - additional information about the failure can be retrieved from [GetLastError](GetLastError.md).

## Arguments ##
| **Name** | **Type** | **Description** |
|:---------|:---------|:----------------|
| Path     | String   | The path to the file. |
| Attributes | UInt32   | The attributes to set on the file. |