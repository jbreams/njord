Control Service will send control signals to windows services. It encapsulates both [StartService](http://msdn.microsoft.com/en-us/library/ms686321(VS.85).aspx) and [ControlService](http://msdn.microsoft.com/en-us/library/ms682108(VS.85).aspx), taking care of properly opening and closing the services database.

## Arguments ##
| **Name** | **Type** | **Description** |
|:---------|:---------|:----------------|
| ServiceName | String   | The name of the service to control |
| ControlCode | UInt32   | The control signal you want to send to the service |
| Async    | Boolean  | Whether the service should wait for the service to change to its new state or return immediately |

## Return Value ##

If the function succeeds, the return value is true otherwise it is false. The function will return false if the service has not moved to its new state within the period of the wait hint. Call [GetLastError](GetLastError.md) to get extended error information.

## Remarks ##
By default this function will send the specified service a start signal and wait the wait hint for it to start.

This function does not implement a restart function. To restart a service call stop and start like this:
```
function RestartService(serviceName)
{
    if(!ControlService(serviceName, SERVICE_CONTROL_STOP))
        return false;
    return ControlService(serviceName);
}
```