CopyFile encapsulates the native Win32 [CopyFile](http://msdn.microsoft.com/en-us/library/aa363851(VS.85).aspx) function and takes the same parameters in the same order.

## Arguments ##

| **Name** | **Type** | **Description** |
|:---------|:---------|:----------------|
| lpExistingFileName | String   | The name of an existing file. |
| lpNewFileName | String   | The name of a new file |
| bFailIfExists | Boolean  | If this parameter is TRUE and the new file specified by lpNewFileName already exists, the function fails. If this parameter is FALSE and the new file already exists, the function overwrites the existing file and succeeds. |

## Return Value ##

The function returns true on a success and false if an error occurs. To get extended error information, call [GetLastError](GetLastError.md).

## Remarks ##
Security attributes for the existing file are not copied to the new file.

File attributes for the existing file are copied to the new file. For example, if an existing file has the FILE\_ATTRIBUTE\_READONLY file attribute, a copy created through a call to CopyFile will also have the FILE\_ATTRIBUTE\_READONLY file attribute.

This function fails with ERROR\_ACCESS\_DENIED if the destination file already exists and has the FILE\_ATTRIBUTE\_HIDDEN or FILE\_ATTRIBUTE\_READONLY attribute set.

When CopyFile is used to copy an encrypted file, it attempts to encrypt the destination file with the keys used in the encryption of the source file. If this cannot be done, this function attempts to encrypt the destination file with default keys, as in Windows 2000. If neither of these methods can be done, CopyFile fails with an ERROR\_ENCRYPTION\_FAILED error code.

Windows 2000:  When CopyFile is used to copy an encrypted file, the function attempts to encrypt the destination file with the default keys. No attempt is made to encrypt the destination file with the keys used in the encryption of the source file. If it cannot be encrypted, CopyFile completes the copy operation without encrypting the destination file.
Symbolic link behavior—If the source file is a symbolic link, the actual file copied is the target of the symbolic link.

If the destination file already exists and is a symbolic link, the symbolic link is overwritten by the source file.