NetLocalGroupAddMembers will add one or more users to a local security group. The function encapsulates [NetLocalGroupAddMembers](http://msdn.microsoft.com/en-us/library/aa370436%28VS.85%29.aspx) function from the Windows Network API using level 0 parameters.

## Parameters ##
| **Name** | **Type** | **Description** |
|:---------|:---------|:----------------|
| GroupName | String   | The name of the local group to add members to. |
| Members  | String or Array | The user or users/groups to add to the group. |

## Return Value ##
The function will return an array of error codes that specify whether each specified user was added. If only one user was specified, it will return an array with a single value. A value of 0 from the array indicates success.

## Remarks ##
The members parameter will accept either usernames or a string version of a valid SID. If you pass a username, the function will internally call [LookupAccountName](http://msdn.microsoft.com/en-us/library/aa379159%28VS.85%29.aspx) to resolve it into a SID. Calling [LookupAccountName](http://msdn.microsoft.com/en-us/library/aa379159%28VS.85%29.aspx) is inefficient because it will have to contact and authenticate against a domain controller if the specified user/group is in a domain.