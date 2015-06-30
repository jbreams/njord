ApplyImage will apply a loaded [WIMImage](WIMImage.md) to a local filesystem. It takes two parameters, the first is the path where the image should be applied (eg. "C:\\"), and the second is a flags parameter that will control how the files are restored. When the image has been applied, the function will flush the file buffers on the target filesystem to prevent data corruption if the computer is suddenly turned off.

## Flags ##
| **Value** | **Description** |
|:----------|:----------------|
| WIM\_FLAG\_VERIFY | Verifies that the files match original data |
| WIM\_FLAG\_INDEX | Specifies that the image is to be sequentially read for performance reasons |
| WIM\_FLAG\_NO\_APPLY | Goes through the application process without actually creating any files or directories. Useful for testing. |
| WIM\_FLAG\_FILEINFO | Displays information about which files are being applied in the dialog box created by [StartUI](WIMStartUI.md) |
| WIM\_FLAG\_NO\_RP\_FIX | Disables automatich path fixups for junctions and symbolic links |
| WIM\_FLAG\_NO\_DIRACL | Disables restoring security information for directories |
| WIM\_FLAG\_NO\_FILEACL | Disables restoring security information for files |