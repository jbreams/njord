The nJord Platform exposes functionality for editing the Windows registry. The majority of the work for editing the registry is done using the [RegKey](RegKey.md) class. The [RegKey](RegKey.md) class can be created either by using the [RegOpenKey](RegOpenKey.md) or [RegCreateKey](RegCreateKey.md) functions. It provides all the functionality required for manipulating values and subkeys. Additionally, there are global functions for manipulating hives and keys:

  * [RegLoadHive](RegLoadHive.md)
  * [RegUnloadHive](RegUnloadHive.md)
  * [RegDeleteKey](RegDeleteKey.md)