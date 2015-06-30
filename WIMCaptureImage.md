CaptureImage will capture an image from the local filesystem. It takes two parameters, the first is the path for the root of the image and the second is an optional flags value. The path for the root of the image does not necessarily have to be the root of the filesystem. You could, for example call wimFile.CaptureImage("c:\\pathtoimage") if c:\pathtoimage pointed to the contents of your image. On success the function will return a [WIMImage](WIMImage.md) object for the image just created, otherwise it will return false; more information about any errors can be retrieved with [GetLastError](GetLastError.md).

## Flags ##
| **Value** | **Description** |
|:----------|:----------------|
| WIM\_FLAG\_NO\_DIRACL | Do not capture directory security information |
| WIM\_FLAG\_NO\_FILEACL | Do not capture file security information |
| WIM\_FLAG\_NO\_RP\_FIX | Disables automatic path fixups for junctions and symbolic links |
| WIM\_FLAG\_VERIFY | Capture verifies single-instance files byte by byte |