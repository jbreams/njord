// js_xdeploy.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "js_xdeploy.h"
#include <lm.h>
#include <Sddl.h>
#include <comdef.h>
#include <wbemidl.h>
#pragma comment(lib, "wbemuuid.lib")

struct JSConstDoubleSpec xdeployConsts[] = {
	{ ComputerNamePhysicalDnsDomain, "ComputerNamePhysicalDnsDomain", 0, 0 },
	{ ComputerNamePhysicalDnsHostname, "ComputerNamePhysicalDnsHostname", 0, 0 },
	{ ComputerNamePhysicalNetBIOS, "ComputerNamePhysicalNetBIOS", 0, 0 },
	{ NETSETUP_JOIN_DOMAIN, "NETSETUP_JOIN_DOMAIN", 0, 0 },
	{ NETSETUP_ACCT_CREATE, "NETSETUP_ACCT_CREATE", 0, 0 },
	{ NETSETUP_WIN9X_UPGRADE, "NETSETUP_WIN9X_UPGRADE", 0, 0},
	{ NETSETUP_DOMAIN_JOIN_IF_JOINED, "NETSETUP_DOMAIN_JOIN_IF_JOINED", 0, 0 },
	{ NETSETUP_JOIN_UNSECURE, "NETSETUP_JOIN_UNSECURE", 0, 0 },
	{ NETSETUP_MACHINE_PWD_PASSED, "NETSETUP_MACHINE_PWD_PASSED", 0, 0 },
	{ NETSETUP_DEFER_SPN_SET, "NETSETUP_DEFER_SPN_SET", 0, 0 },
	{ NETSETUP_JOIN_WITH_NEW_NAME, "NETSETUP_JOIN_WITH_NEW_NAME", 0, 0 },
	{ NETSETUP_INSTALL_INVOCATION, "NETSETUP_INSTALL_INVOCATION", 0, 0 },
	{ SidTypeUser, "SidTypeUser", 0, 0 },
	{ SidTypeGroup, "SidTypeGroup", 0, 0 },
	{ SidTypeDomain, "SidTypeDomain", 0, 0 },
	{ SidTypeAlias, "SidTypeAlias", 0, 0 },
	{ SidTypeWellKnownGroup, "SidTypeWellKnownGroup", 0, 0 },
	{ SidTypeDeletedAccount, "SidTypeInvalid", 0, 0 },
	{ SidTypeUnknown, "SidTypeUnknown", 0, 0 },
	{ SidTypeComputer, "SidTypeComputer", 0, 0 },
	{ SidTypeLabel, "SidTypeLabel", 0, 0 },
	{ 0 }
};

IWbemServices * pSvc = NULL;
IWbemLocator * pLoc = NULL;

JSBool setcomputername(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JSString * newName;
	COMPUTER_NAME_FORMAT nameType = ComputerNamePhysicalNetBIOS;

	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "S /u", &newName, (DWORD*)&nameType))
	{
		JS_ReportError(cx, "Unable to parse arguments in setcomputername.");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	JS_EndRequest(cx);

	*rval = SetComputerNameEx(nameType, (LPWSTR)JS_GetStringChars(newName)) != 0 ? JSVAL_TRUE : JSVAL_FALSE;
	return JS_TRUE;
}

JSBool netjoindomain(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JSString * serverStr = NULL, * domainStr = NULL, *accountOUStr = NULL, *accountStr = NULL, *passwordStr = NULL;
	DWORD joinOptions = 0;

	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "S S S S u", &domainStr, &accountOUStr, &accountStr, &passwordStr, &joinOptions))
	{
		JS_ReportError(cx, "Unable to parse arguments in netjoindomain");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	JS_YieldRequest(cx);
	DWORD result = NetJoinDomain(NULL, (LPWSTR)JS_GetStringChars(domainStr), (LPWSTR)JS_GetStringChars(accountOUStr),
		(LPWSTR)JS_GetStringChars(accountStr), (LPWSTR)JS_GetStringChars(passwordStr), joinOptions);
	if(result == NERR_Success)
	{
		*rval = JSVAL_TRUE;
		return JS_TRUE;
	}
	JSBool retBool = JS_NewNumberValue(cx, result, rval);
	JS_EndRequest(cx);
	return retBool;
}

JSBool netlocalgroupaddsid(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JSString * groupName, *sidString;
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "S S", &groupName, &sidString))
	{
		JS_ReportError(cx, "Unable to parse argument in netlocalgroupaddsid");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	PSID realSid;
	ConvertStringSidToSid((LPWSTR)JS_GetStringChars(sidString), &realSid);

	LOCALGROUP_MEMBERS_INFO_0 passed;
	passed.lgrmi0_sid = realSid;

	JS_YieldRequest(cx);
	NET_API_STATUS status = NetLocalGroupAddMembers(NULL, (LPWSTR)JS_GetStringChars(groupName), 0, (LPBYTE)&passed, 1);
	LocalFree(realSid);
	
	if(status == NERR_Success)
		*rval = JSVAL_TRUE;
	else
		JS_NewNumberValue(cx, status, rval);
	JS_EndRequest(cx);
	return JS_TRUE;
}

JSBool netlocalgroupaddmembers(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JSString * groupName;
	JSObject * membersArray = NULL;
	JSString * singleMember = NULL;

	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "S *", &groupName) || argc < 2)
	{
		JS_ReportError(cx, "Unable to parse argument in netlocalgroupaddmembers");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	if(JSVAL_IS_OBJECT(argv[1]))
	{
		JS_ValueToObject(cx, argv[1], &membersArray);
		if(!JS_IsArrayObject(cx, membersArray))
		{
			JS_ReportError(cx, "netlocalgroupaddmembers takes either a string or an array of strings as an argument. Invalid type specified.");
			JS_EndRequest(cx);
			return JS_FALSE;
		}
	}
	else if(JSVAL_IS_STRING(argv[1]))
		singleMember = JS_ValueToString(cx, argv[1]);
	else
	{
		JS_ReportError(cx, "netlocalgroupaddmembers takes either a string or an array of strings as an argument. Invalid type specified.");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	
	LOCALGROUP_MEMBERS_INFO_3 * members = NULL;
	DWORD memberCount = 0;
	if(singleMember != NULL)
	{
		members = new LOCALGROUP_MEMBERS_INFO_3;
		members->lgrmi3_domainandname = (LPWSTR)JS_GetStringChars(singleMember);
		memberCount = 1;
	}
	else if(membersArray != NULL)
	{
		JS_GetArrayLength(cx, membersArray, &memberCount);
		members = new LOCALGROUP_MEMBERS_INFO_3[memberCount];
		for(DWORD i = 0; i < memberCount; i++)
		{
			jsval curMemberVal;
			JS_GetElement(cx, membersArray, i, &curMemberVal);
			JSString * curMemberStr = JS_ValueToString(cx, curMemberVal);
			members[i].lgrmi3_domainandname = (LPWSTR)JS_GetStringChars(curMemberStr);
		}
	}

	DWORD result = NetLocalGroupAddMembers(NULL, (LPWSTR)JS_GetStringChars(groupName), 3, (LPBYTE)members, memberCount);
	delete [] members;

	JSBool retBool = JS_NewNumberValue(cx, result, rval);
	JS_EndRequest(cx);
	return retBool;
}

JSBool lookupAccountName(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JSString * accountName;
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "S", &accountName))
	{
		JS_ReportError(cx, "Error parsing arguments in lookupAccountName");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	PSID sid;
	LPWSTR domainName;
	DWORD sidSize = 0, rdSize = 0;
	SID_NAME_USE peUse;

	LookupAccountName(NULL, (LPWSTR)JS_GetStringChars(accountName), NULL, &sidSize, NULL, &rdSize, &peUse);
	domainName = (LPWSTR)JS_malloc(cx, (rdSize + 1) * sizeof(TCHAR));
	sid = (PSID)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sidSize);

	JS_YieldRequest(cx);
	if(!LookupAccountName(NULL, (LPWSTR)JS_GetStringChars(accountName), sid, &sidSize, domainName, &rdSize, &peUse))
	{
		HeapFree(GetProcessHeap(), 0, sid);
		JS_free(cx, domainName);
		*rval = JSVAL_FALSE;
		JS_EndRequest(cx);
		return JS_TRUE;
	}

	JSObject * retObj = JS_NewObject(cx, NULL, NULL, obj);
	*rval = OBJECT_TO_JSVAL(retObj);

	LPTSTR stringSid = NULL;

	ConvertSidToStringSid(sid, &stringSid);
	HeapFree(GetProcessHeap(), 0, sid);

	JS_DefineProperty(cx, retObj, "sid", STRING_TO_JSVAL(JS_NewUCStringCopyZ(cx, (jschar*)stringSid)), NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY);
	LocalFree(stringSid);
	JS_DefineProperty(cx, retObj, "domain", STRING_TO_JSVAL(JS_NewUCString(cx, (jschar*)domainName, rdSize)), NULL, NULL,  JSPROP_ENUMERATE | JSPROP_READONLY);
	jsval peUseVal;
	JS_NewNumberValue(cx, peUse, &peUseVal);
	JS_DefineProperty(cx, retObj, "type", peUseVal, NULL, NULL,  JSPROP_ENUMERATE | JSPROP_READONLY);
	JS_EndRequest(cx);
	return JS_TRUE;
}

JSBool GetLastNetErrorMessage(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	DWORD dwLastError = NERR_Success;
	HANDLE hModule = GetModuleHandle(NULL);
	if(argc < 1 || !JSVAL_IS_NUMBER(*argv))
	{
		JS_ReportError(cx, "Must provide valid error code to GetLastNetErrorMessage");
		return JS_FALSE;
	}

	JS_BeginRequest(cx);
	JS_ValueToECMAUint32(cx, *argv, &dwLastError);
	DWORD dwFormatFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_IGNORE_INSERTS |
        FORMAT_MESSAGE_FROM_SYSTEM ;

	if(dwLastError >= NERR_BASE && dwLastError <= MAX_NERR) 
	{
        hModule = LoadLibraryEx(TEXT("netmsg.dll"), NULL, LOAD_LIBRARY_AS_DATAFILE);
        if(hModule != NULL)
            dwFormatFlags |= FORMAT_MESSAGE_FROM_HMODULE;
    }

	LPWSTR buffer = NULL;
	DWORD size = FormatMessage(dwFormatFlags, hModule, dwLastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&buffer, 0, NULL);
	if(size == 0)
	{
		*rval = JSVAL_FALSE;
		return JS_TRUE;
	}

	JSString * rString = JS_NewUCStringCopyN(cx, (jschar*)buffer, size);
	*rval = STRING_TO_JSVAL(rString);
	JS_EndRequest(cx);
	LocalFree(buffer);
	return JS_TRUE;
}

#ifdef __cplusplus
extern "C" {
#endif

BOOL __declspec(dllexport) InitExports(JSContext * cx, JSObject * global)
{
	CoInitialize(NULL);
	if(CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc) != ERROR_SUCCESS)
	{
		JS_ReportError(cx, "Error creating WbemLocator.");
		return FALSE;
	}
	
	if(pLoc->ConnectServer(_bstr_t(TEXT("ROOT\\CIMV2")), NULL, NULL, 0, NULL, 0, 0, &pSvc) != ERROR_SUCCESS)
	{
		JS_ReportError(cx, "Error connecting to local WMI.");
		pLoc->Release();
		return FALSE;
	}

	if(CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE) != ERROR_SUCCESS)
	{
		JS_ReportError(cx, "Error setting security parameters for WMI query.");
		return FALSE;
	}

	JSFunctionSpec xdeployFunctions[] = {
		{ "SetComputerName", setcomputername, 2, 0 },
		{ "NetJoinDomain", netjoindomain, 6, 0 },
		{ "NetLocalGroupAddMembers", netlocalgroupaddmembers, 2, 0 },
		{ "NetLocalGroupAddSid", netlocalgroupaddsid, 2, 0 },
		{ "NetGetLastErrorMessage", GetLastNetErrorMessage, 1, 0 },
		{ "LookupAccountName", lookupAccountName, 1, 0 },
		{ 0 },
	};

	JS_BeginRequest(cx);
	JS_DefineFunctions(cx, global, xdeployFunctions);
	JS_DefineConstDoubles(cx, global, xdeployConsts);
	JS_EndRequest(cx);
	return TRUE;
}

BOOL __declspec(dllexport) CleanupExports(JSContext * cx, JSObject * global)
{
	pSvc->Release();
	pLoc->Release();
	return TRUE;
}

#ifdef __cplusplus
}
#endif