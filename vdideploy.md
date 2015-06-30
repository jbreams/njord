# Source Code for VDI Deployment Script #
This is a good example of a simple deployment script with emphasis on size and speed. It has no user interface, but does produce a log. It also includes the [XMLRPC](xmlrpcbinding.md) bindings.

```
SetCurrentDirectory("c:\\xdeploy");
SetDllDirectory(".\\libs");

function AppendToLog(toadd, quit)
{
	var toWrite = Date() + ": " + toadd + "\r\n";
	var retVal = FileSetContents("c:\\deploy.log", toWrite, true);
	if(quit === true)
		Exit();
	return retVal;
}

FileSetContents("c:\\deploy.log", Date() + ": Starting deployment.\r\n");
var setupKey = RegOpenKey(HKEY_LOCAL_MACHINE, "SYSTEM\\Setup");
if(setupKey == false)
	AppendToLog("Unable to open HKLM\\System\\Setup. Cannot continue. Error code: " + GetLastError(), true);
setupKey.SetValue("SystemSetupInProgress", 0);
setupKey.SetValue("SetupType", 0);
setupKey.Flush();
Sleep(1000);
AppendToLog("Disabled system setup in progress.");

if(!ControlService("dhcp"))
	AppendToLog("Unable to start the DHCP service. Error code: " + GetLastError(), true);
if(!ControlService("dnscache"))
	AppendToLog("Unable to start the DNS Client service. Error code: " + GetLastError(), true);

LoadNative("js_wmi.dll");
LoadNative("js_curl.dll");
LoadNative("js_xdeploy.dll");
LoadJSLib("xmlrpc.js");

var macaddresses = new Array();
var wmiSvcs = WbemConnect("root\\cimv2");
if(wmiSvcs === false)
	AppendToLog("Unable to connect to WMI. Error code: " + GetLastError(), true);
var netEnum = wmiSvcs.ExecQuery("select MacAddress from Win32_NetworkAdapter where MacAddress <> null and NetConnectionStatus = 2");
var adapter = netEnum.Next();
while(adapter != false)
{
	macaddresses.push(adapter.MacAddress);
	AppendToLog("Found a network connection! MAC Address: " + adapter.MacAddress);
	adapter = netEnum.Next();
}
if(macaddresses.length == 0)
	AppendToLog("No active network connections were found. Cannot query for computer name without an active network connection.", true);
else
	AppendToLog("Completed enumeration of MAC Addresses, " + macaddresses.length + " found.");
var contactAttempts = 0;
var onNetwork = false;
do
{
	AppendToLog("Checking for network connectivity...attempt #" + contactAttempts);
	onNetwork = xd_rpc_call('on_network', new Array());
	Sleep(5000);
} while(!(onNetwork === true) && contactAttempts++ < 3);
if(!onNetwork)
	AppendToLog("Unable to contact deployment server.", true);
AppendToLog("I'm on the network! Successfully contacted the deployment server.");

var compName = xd_rpc_call('vdi_get_name', new Array(macaddresses));
if(compName === false)
	AppendToLog("Unable to execute RPC call to retrieve computer name.", true);
AppendToLog("The deployment server says that I'm named '" + compName + "'");
if(!SetComputerName(compName, ComputerNamePhysicalDnsHostname))
	AppendToLog("Unable to set computer name. Error code: " + GetLastError(), true);
AppendToLog("SetComputerName succeeded.");
var nameKey = RegOpenKey(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\ComputerName\\ActiveComputerName");
if(!nameKey)
	AppendToLog("Unable to open ComputerName key. Error code: " + GetLastError(), true);
else
{
	nameKey.SetValue("ComputerName", compName);
	AppendToLog("ActiveComputerName value set.");
}
nameKey.Close();
		
nameKey = RegOpenKey(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters");
if(!nameKey)
	AppendToLog("Unable to open TCP/IP parameters key. Error code: " + GetLastError(), true);
else
{
	nameKey.SetValue("Hostname", compName);
	AppendToLog("TCP/IP Hostname value set.");
}
nameKey.Close();

if(!ControlService("lanmanworkstation"))
	AppendToLog("Unable to start the workstation service. Cannot continue. Error code: " + GetLastError(), true);
AppendToLog("Workstation service started");
var domPasswd = FileGetContents("dom.txt");
if(domPasswd === false)
	AppendToLog("Unable to open the password file, cannot continue.  Error code: " + GetLastError(), true);
AppendToLog("vdiuser password retrieved. Deleting the file.");
DeleteFile("dom.txt");
var joinResult = NetJoinDomain('drew-ad', 'OU=VPC,OU=VDI,DC=ad,DC=drew,DC=edu', 'drew-ad\\vdiuser', domPasswd, NETSETUP_JOIN_DOMAIN | NETSETUP_ACCT_CREATE);
if(typeof(joinResult) == 'number')
	AppendToLog("Unable to join the domain. " + NetGetLastErrorMessage(joinResult), true);

var firewallPorts = RegOpenKey(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\SharedAccess\\Parameters\\FirewallPolicy\\DomainProfile\\GloballyOpenPorts\\List");
if(firewallPorts == false)
	AppendToLog("Unable to open firewall policy, the Data Collector service will fail.", true);
firewallPorts.SetValue("5203:TCP", "5203:TCP:*:Enabled:Quest Collector Port");
firewallPorts.Close();
AppendToLog("Set firewall exception for Quest Collector Service");
AppendToLog("Deployment complete!");
```