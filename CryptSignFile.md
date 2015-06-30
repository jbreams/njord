SignFile will sign a file using the private key of a certificate installed on the calling user's certificate store. It will return the signature as a base-64 encoded string with headers and line-breaks.

## Parameters ##
| **Name** | **Type** | **Description** |
|:---------|:---------|:----------------|
| FileName | String   | The path to file to hash and sign |
| SubjectName | String   | The subject of the certificate to use in signing |

## Return Value ##
If the function succeeds, it will return the signature as a string. Otherwise it will return false, more information about any error can be retrieved with [GetLastError](GetLastError.md).

## Remarks ##
Internally the function finds certificates in the local "my" store based on the supplied subject name. The file to be signed is read in 4kb at a time and put into an SHA1 hash. The hash is signed using the certificate and base-64 encoded. The function uses the Windows Cryptography API and requires no dependencies.