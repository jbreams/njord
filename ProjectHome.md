The nJord! Platform is an environment for administrative scripting in Windows. It encapsulates Mozilla's SpiderMonkey JavaScript interpreter with native Win32 API functions and the ability to load additional native JavaScript plugins at runtime. nJord! scripts are ECMAscripts and follow the same syntax.

## Builtin Functionality ##

The nJord! Platform has some native functions built in without having to load any plugins. These include:
  * [Basic File Handling](FileIO.md)
  * [Process & service execution](Execution.md)
  * [Filesystem Searching](FindFile.md)
  * [Libraries Native & JS library loading](librarysupport.md)
  * [Registry manipulation](Registry.md)
  * [Misc Functions](Misc.md)


## Plugins ##

Writing plugins for the nJord! Platform is straightforward and there are already several plugins written for scripting.

  * [js\_curl](js_curl.md) - the Curl libraries.
  * [js\_gecko2](js_gecko2.md) - embedded Gecko API version 2.
  * [js\_hotkey](js_hotkey.md) - Supports global hotkey callbacks in JavaScript
  * [js\_wimg](js_wimg.md) - Implements the Windows Imaging API for nJord.
  * [js\_wmi](js_wmi.md) - Implements access to instances of WMI classes.
  * [js\_xdeploy](js_xdeploy.md) - Various functions for deploying workstations

## Examples ##

  * [vdideploy](vdideploy.md) - A barebones UI-less deployment script for virtual desktops
  * [xmlrpcbinding](xmlrpcbinding.md) - An XMLRPC binding using [js\_curl](js_curl.md) and E4X
  * [imageclient](imageclient.md) - Imaging software using [js\_gecko2](js_gecko2.md), [js\_wmi](js_wmi.md), and [js\_wimg](js_wimg.md) for windows imaging support.

## Tunable Parameters ##

nJord is invoked as you would invoke any script interpreter "njord scriptname scriptargs". After the filename of the script to load, the command line is split by spaces and put into an array named "argv" that is accessible to scripts. There is no argc, because you can access the number of arguments with argv.length.

The nJord platform exposes several parameters that can help tune performance and setup a consistent execution environment. These parameters are exposed in the nJord registry key at "HKEY\_LOCAL\_MACHINE\SOFTWARE\nJord". The parameters exposed are:
| Name | Type | Description ||
|:-----|:-----|:------------|
| RuntimeSize | REG\_DWORD | The size of the runtime heap before garbage collection will be run. (Defaults to 64mb) |
| StackSize | REG\_DWORD | The size of the stack for each context created. (Defaults to 8kb) |
| BranchLimit | REG\_DWORD | The number of branches before the garbage collector is run (Defaults to 5000) |
| LibPath | REG\_SZ | The path to a folder where nJord libraries are set. |
| WorkingPath | REG\_SZ | Sets the default working directory for nJord scripts. |

## Security ##
The nJord platform supports code-signing based off the Microsoft Cryptography libraries. The platform comes with a tool called "signtool" which will sign an nJord script using the private key of a certificate installed on the local system. On the client-side, you can setup code-signing by setting values in registry key "HKEY\_LOCAL\_MACHINE\SOFTWARE\nJord\Security" or simply by placing a X509 encoded certificate in the same directory as the script you want to load. The registry-based signing also supports loading certificates from system certificate store and from a binary blob stored in the registry itself. Code signing is setup before loading the main script and, at present, there is no way to load additional certificates later during execution. Read more about the technical details of code signing in nJord [here](CodeSigning.md).