WbemEnumerator will enumerate the result of a query from [WbemServices](WbemServices.md). Its two methods take no parameters and return false on failure.

## Methods ##
| **Name** | **Description** |
|:---------|:----------------|
| Next     | Advances the enumerator to the next result and returns it as a [WbemClass](WbemClass.md) object or false on failure. |
| Reset    | Resets the enumerator to the beginning of the results. Returns true on success. |