WbemClass is a special class since it has no methods and no properties on the prototype. It implements all the properties of the underlying WMI class as lazy properties - so getting information from a WMI class is as simple as getting information from the class's properties. It will convert properties from their CIMTYPE into the proper ECMAScript value before returning it. All the properties are also available in for each loops. You could, for example, enumerating the properties of a Win32\_DiskParition could be accomplished with the following code.
```
var wmiSvcs = WbemConnect("root\\cimv2");
var partEnum = wmiSvc.OpenClass("Win32_DiskPartition");
var part = partEnum.Next();
while(part != false)
{
    for each(var propName in part)
        MessageBox(part[propName]);
    part = partEnum.Next();
}
```