RegCreateKey will create new registry keys or open existing ones. It returns the resulting key as a [RegKey](RegKey.md) object or false on failure.

## Parameters ##
| **Name** | **Type** | **Description** |
|:---------|:---------|:----------------|
| Hive     | UInt32   | The hive that contains or will contain the specified key. |
| subKey   | String   | the path to the subkey that will be opened or created |
| desiredAccess | UInt32   | The desired access to the key. |

## Hives ##

The first parameter for the function specifies which hive the key is located. The hives are defined statically, based on the hives from the [MSDN documentation](http://msdn.microsoft.com/en-us/library/ms724836(VS.85).aspx).

## Desired Access ##
By default, the function will try to acquire KEY\_READ | KEY\_WRITE access to the created/opened key, but this can be overridden by the desiredAccess parameter. This parameter can contain one or more of the following values.

| **Value** | **Description** |
|:----------|:----------------|
| KEY\_ALL\_ACCESS | Provides all possible rights to the key |
| KEY\_CREATE\_SUB\_KEY | Allows for the creation of sub-keys under the key |
| KEY\_ENUMERATE\_SUB\_KEYS | Allows enumerating sub keys |
| KEY\_QUERY\_VALUE | Required to query the values of a registry key |
| KEY\_SET\_VALUE | Required to create, delete, or set a registry value |
| KEY\_READ | Combines the STANDARD\_RIGHTS\_READ, KEY\_QUERY\_VALUE, KEY\_ENUMERATE\_SUB\_KEYS, and KEY\_NOTIFY values |
| KEY\_WRITE | Combines the STANDARD\_RIGHTS\_WRITE, KEY\_SET\_VALUE, and KEY\_CREATE\_SUB\_KEY access rights |
| KEY\_WOW64\_32KEY | Indicates that an application on 64-bit Windows should operate on the 32-bit registry view |
| KEY\_WOW64\_64KEY | Indicates that an application on 64-bit Windows should operate on the 64-bit registry view |