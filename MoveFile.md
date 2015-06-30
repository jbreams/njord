MoveFile encapsulates the native Win32 [MoveFileEx](http://msdn.microsoft.com/en-us/library/aa365240(VS.85).aspx) function. All the arguments are the same.

## Arguments ##
| **Name** | **Type** | **Description** |
|:---------|:---------|:----------------|
| lpExistingFileName | String   | The current name of the file or directory on the local computer. |
| lpNewFileName | String   | The new name of the file or directory on the local computer.|
| dwFlags  | Unsigned Long | This parameter can be one or more flags. |

## Flags ##
| **Value** | **Meaning** |
|:----------|:------------|
| MOVEFILE\_COPY\_ALLOWED | If the file is to be moved to a different volume, the function simulates the move by using the CopyFile and DeleteFile functions. This value cannot be used with MOVEFILE\_DELAY\_UNTIL\_REBOOT. |
| MOVEFILE\_DELAY\_UNTIL\_REBOOT || The system does not move the file until the operating system is restarted. The system moves the file immediately after AUTOCHK is executed, but before creating any paging files. Consequently, this parameter enables the function to delete paging files from previous startups. This value can be used only if the process is in the context of a user who belongs to the administrators group or the LocalSystem account.|
This value cannot be used with MOVEFILE\_COPY\_ALLOWED. 
| MOVEFILE\_FAIL\_IF\_NOT\_TRACKABLE | The function fails if the source file is a link source, but the file cannot be tracked after the move. This situation can occur if the destination is a volume formatted with the FAT file system. |
| MOVEFILE\_REPLACE\_EXISTING || If a file named lpNewFileName exists, the function replaces its contents with the contents of the lpExistingFileName file, provided that security requirements regarding access control lists (ACLs) are met. For more information, see the Remarks section of this topic.|
This value cannot be used if lpNewFileName or lpExistingFileName names a directory. 
| MOVEFILE\_WRITE\_THROUGH | The function does not return until the file is actually moved on the disk. Setting this value guarantees that a move performed as a copy and delete operation is flushed to disk before the function returns. The flush occurs at the end of the copy operation. This value has no effect if MOVEFILE\_DELAY\_UNTIL\_REBOOT is set. |

## Return Value ##
The function returns true on a successful and false if there is an error. Call [GetLastError](GetLastError.md) for more information.

## Remarks ##
If the dwFlags parameter specifies MOVEFILE\_DELAY\_UNTIL\_REBOOT, MoveFileEx fails if it cannot access the registry. The function stores the locations of the files to be renamed at restart in the following registry value:

HKEY\_LOCAL\_MACHINE\SYSTEM\CurrentControlSet\Control\Session  Manager\PendingFileRenameOperations

This registry value is of type REG\_MULTI\_SZ. Each rename operation stores one of the following NULL-terminated strings, depending on whether the rename is a delete or not:

szDstFile\0\0

szSrcFile\0szDstFile\0

The string szDstFile\0\0 indicates that the file szDstFile is to be deleted on reboot. The string szSrcFile\0szDstFile\0 indicates that szSrcFile is to be renamed szDstFile on reboot.

Note  Although \0\0 is technically not allowed in a REG\_MULTI\_SZ node, it can because the file is considered to be renamed to a null name.

The system uses these registry entries to complete the operations at restart in the same order that they were issued. For example, the following code fragment creates registry entries that delete szDstFile and rename szSrcFile to be szDstFile at restart:
```
MoveFileEx(szDstFile, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
MoveFileEx(szSrcFile, szDstFile, MOVEFILE_DELAY_UNTIL_REBOOT);
```
Because the actual move and deletion operations specified with the MOVEFILE\_DELAY\_UNTIL\_REBOOT flag take place after the calling application has ceased running, the return value cannot reflect success or failure in moving or deleting the file. Rather, it reflects success or failure in placing the appropriate entries into the registry.

The system deletes a directory that is tagged for deletion with the MOVEFILE\_DELAY\_UNTIL\_REBOOT flag only if it is empty. To ensure deletion of directories, move or delete all files from the directory before attempting to delete it. Files may be in the directory at boot time, but they must be deleted or moved before the system can delete the directory.

The move and deletion operations are carried out at boot time in the same order that they are specified in the calling application. To delete a directory that has files in it at boot time, first delete the files.

If a file is moved across volumes, MoveFileEx does not move the security descriptor with the file. The file is assigned the default security descriptor in the destination directory.

The MoveFileEx function coordinates its operation with the link tracking service, so link sources can be tracked as they are moved.

To delete or rename a file, you must have either delete permission on the file or delete child permission in the parent directory. If you set up a directory with all access except delete and delete child and the ACLs of new files are inherited, then you should be able to create a file without being able to delete it. However, you can then create a file, and get all the access you request on the handle that is returned to you at the time that you create the file. If you request delete permission at the time you create the file, you can delete or rename the file with that handle but not with any other handle.

_Taken from the MSDN Library_