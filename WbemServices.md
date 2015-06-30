WbemServices encapsulates a single WMI namespace and is what will perform all the queries into that namespace.

## Methods ##
| **Name** | **Description** |
|:---------|:----------------|
| [OpenClass](OpenClass.md) | Opens an enumerator for instances or the definition of a specified class. |
| [ExecQuery](ExecQuery.md) | Executes a WQL query against the namespace and returns the result as an enumerator |
| [OpenInstance](OpenInstance.md) | Opens a single instance of a class given the path to that class |