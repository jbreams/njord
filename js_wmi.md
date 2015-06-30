JS\_WMI exposes read-only access into the Windows Management Instrumentation for querying properties about the system. It wraps up several of the COM objects from the WMI C++ API, and requires the computer not to be in setup mode.

The module exposes one function for creating an object that represents a single WMI namespace, [WbemConnect](WbemConnect.md).  The module creates the WbemLocator when it is loaded. It exposes classes to access the WMI namespaces.

## Classes ##
| **Name** | **Description** |
|:---------|:----------------|
| [WbemServices](WbemServices.md) | Encapsulates a single WMI namespace and the functions that can be performed on it. |
| [WbemEnumerator](WbemEnumerator.md) | An enumerator for results of queries. |
| [WbemClass](WbemClass.md) | Encapsulates a single instance or definition of a WMI class. |

All objects are cleaned up automatically when they fall out of scope.