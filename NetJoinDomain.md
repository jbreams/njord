NetJoinDomain will join the local computer to an active directory. It encapsulates the [NetJoinDomain](http://msdn.microsoft.com/en-us/library/aa370433%28VS.85%29.aspx) function from the Windows Networking API.

## Parameters ##
| **Name** | **Type** | **Description** |
|:---------|:---------|:----------------|
| DomainName | String   | The name of the domain to join. This can be either the NetBIOS name or the DNS name. |
| OU       | String   | The OU to join the computer into. If this is null, it will attempt to join the computer to the default OU. |
| Username | String   | The username to be used to join the domain, the account must have rights to join to an existing object or to create new objects. |
| Password | String   | The password of the account used to join the domain. |
| Options  | UInt32   | A bitmask that specifies options for joining the domain. |

## Return Value ##
If the call succeeds, it will return true, otherwise it will return an error code as a number. Use [NetGetLastErrorMessage](NetGetLastErrorMessage.md) to get more information about the error.

## Remarks ##
The options parameter will accept one or a combination of the following values.
| Name | Description |
|:-----|:------------|
| NETSETUP\_JOIN\_DOMAIN | Join a domain instead of a workgroup. If this is not set, the call will join a workgroup. |
| NETSETUP\_ACCT\_CREATE | Creates the computer's account in the domain |
| NETSETUP\_WIN9X\_UPGRADE | The join operation is taking place during an upgrade...from Win9x. Oh, compatibility. |
| NETSETUP\_DOMAIN\_JOIN\_IF\_JOINED | Specifies that the join should occur even if the computer is already joined. |
| NETSETUP\_JOIN\_UNSECURE | Joins the computer without authenticating the account specified by Username. Can be used with NETSETUP\_MACHINE\_PWD\_PASSED. |
| NETSETUP\_MACHINE\_PWD\_PASSED | Specifies that the Username/Password parameters are those of a computer object account rather than a user. |
| NETSETUP\_DEFER\_SPN\_SET | Does not update the hostname or domain name on the local computer during the call. |
| NETSETUP\_JOIN\_WITH\_NEW\_NAME | If [SetComputerName](SetComputerName.md) was called and there has been no reboot, use the name set in [SetComputerName](SetComputerName.md) rather than the active one. |
| NETSETUP\_INSTALL\_INVOCATION | Specifies that the join is taking place during Windows Setup. |