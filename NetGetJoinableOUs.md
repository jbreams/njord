NetGetJoinableOUs will return an array of OUs in a domain that can be joined using the supplied credentials. This function encapsulates [NetGetJoinableOUs](http://msdn.microsoft.com/en-us/library/bb720840.aspx) from the Windows Networking API.

## Parameters ##
| **Name** | **Type** | **Description** |
|:---------|:---------|:----------------|
| DomainName | String   | The domain to lookup domains in |
| Username | String   | The username to authenticate against the domain with |
| Password | String   | The password to authenticate against the domain with |

## Return Value ##
If the function succeeds, it will return an Array object with the returned OUs that are joinable. Otherwise it will return the error code returned.