FileSetContents will write a the contents of a file. The function will open a file, truncate it, and write a string to it. If the file specified does not exist, the function will create it. The function will return true on success and false on failure - more information about the failure can be retrieved from [GetLastError](GetLastError.md).

## Parameters ##
| **Name** | **Type** | **Description** |
|:---------|:---------|:----------------|
| Filename | String   | The path to the file to be written. |
| Contents | String   | The data to be written to the file. |
| Append   | Boolean or UInt32 | Whether the data should be appended to written at an offset |
| Unicode  | Boolean  | Whether the data should be written as UTF16. |

## Remarks ##
The function will accept either a boolean value or a number as its third argument. If it is true, then the function will advance the file pointer to the end of the file before writing. If it's a number, then it will advance the file pointer by that value before writing. If the value is false or 0, then the third parameter has no effect. If the value is a number and is greater than the file size, the function will behave as though the value was true.