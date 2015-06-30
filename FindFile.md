The find file function encapsulates the [FindFirstFile](http://msdn.microsoft.com/en-us/library/aa364418(VS.85).aspx) function.

## Arguments ##
| **Name** | **Type** | **Description** |
|:---------|:---------|:----------------|
| SearchString | String   | The search pattern to execute. |

## Return Value ##
On success, the function returns a JavaScript object that has the results of the find operation and a method for advancing the find handle to the next result. On failure, it returns false.

## Remarks ##

For more information about defining search criteria, see the MSDN documentation on [FindFirstFile](http://msdn.microsoft.com/en-us/library/aa364418(VS.85).aspx).

The JavaScript object that's returned from FindFirstFile encapsulates all the fields of the [WIN32\_FIND\_DATA](http://msdn.microsoft.com/en-us/library/aa365740(VS.85).aspx) structure. They are summarized below.


| **Property** | **Type** | **Description** |
|:-------------|:---------|:----------------|
| fileName     | String   | The name of the file |
| altFileName  | String   | The 8.3 format of the filename |
| attributes   | UInt32   | The attributes of the file |
| fileSize     | UInt64   | The size of the file in bytes |
| createTime   | Date object | The date/time the file was created |
| lastWriteTime | Date object | The date/time the file was last modified |


The object has one method, named "Next", which will advance the find handle to the next result and update the contents of the object. If there are no results or the Next method failed, it will return false. The next method is actually the same code as the FindFile function, it just acts on its parent object instead of creating a new one.

The find handle will be cleaned up and closed when the object is finalized.