#include <windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <api\setupapi.h>
#include <api\regstr.h>
#include <api\infstr.h>
#include <api\cfgmgr32.h>

BOOL RescanDevices()
{
	DEVINST rootnode;
	CM_WaitNoPendingInstallEvents(INFINITE);
	if(CM_Locate_DevNode(&rootnode, NULL, CM_LOCATE_DEVNODE_NORMAL) != CR_SUCCESS)
		return FALSE;
	if(CM_Reenumerate_DevNode(rootnode, CM_REENUMERATE_NORMAL) != CR_SUCCESS)
		return FALSE;
	return TRUE;
}