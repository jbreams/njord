GetFileAttributes will get the properties of a specified file and return them as a JavaScript object. It takes one parameter, the path to the file, and returns an object on success and false on failure. The error  code for the failure can be retrieved with [GetLastError](GetLastError.md) The JavaScript object returned will have the following properties:


| **Name** | **Type** | **Description** |
|:---------|:---------|:----------------|
| attributes | UInt32   | The attributes of the file, see the [GetFileAttributes](http://msdn.microsoft.com/en-us/library/aa364944(VS.85).aspx) documentation on MSDN for more information. |
| created  | DateTime | The date/time the file was created |
| accessed | DateTime | The date/time the file was last accessed |
| written  | DateTime | The date/time the file was last written |
| size     | UInt64   | The size of the file. |