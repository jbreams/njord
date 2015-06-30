WIMCreateFile will create or open a WIM file and return it as a [WIMFile](WIMFile.md) object.

## Parameters ##
| **Name** | **Type** | **Description** |
|:---------|:---------|:----------------|
| FileName | String   | The path to the WIM file to be opened or created |
| DesiredAccess | UInt32   | A bitmask to determine the security rights to the file being opened |
| CreationDisposition | UInt32   | Determines whether the file should be opened or created or something inbetween |
| Flags    | UInt32   | Flags that change the behavior of the imaging engine |
| CompressionType | UInt32   | The type of compression that should be used during image capture |

## Return Value ##
The function will either return a [WIMFile](WIMFile.md) object or false on failure. Call [GetLastError](GetLastError.md) for more information about errors.

## Remarks ##

### Access Flags ###
| **Value** | **Description** |
|:----------|:----------------|
| WIM\_GENERIC\_READ | Opens the file with read access. The file will be sharable read-only if only this flag is specified |
| WIM\_GENERIC\_WRITE | Opens the file with write access, this will lock the file for reading and writing |

### Creation Disposition ###
| **Value** | **Description** |
|:----------|:----------------|
| WIM\_CREATE\_NEW | Create a new WIM file. The function fails if the file already exists |
| WIM\_CREATE\_ALWAYS | Create a new WIM file or truncate an existing file |
| WIM\_OPEN\_EXISTING | Opens an existing file. The function failes if the file doesn't exist or is invalid |
| WIM\_OPEN\_ALWAYS | Opens an existing file or create it if it doesn't exist yet |

### Flags ###
| **Value** | **Description** |
|:----------|:----------------|
| WIM\_FLAG\_VERIFY | Verifies file/images during application and capture |
| WIM\_FLAG\_SHARE\_WRITE | Only locks areas of the WIM files that are being captured, allows reading from another image stored in the same WIM file |

### Compression Type ###
| **Value** | **Description** |
|:----------|:----------------|
| WIM\_COMPRESS\_NONE | Don't do any compression during image capture |
| WIM\_COMPRESS\_XPRESS | Perform quick (low) compression during image capture |
| WIM\_COMPRESS\_LZX | Use LZX (high) compression during image capture |