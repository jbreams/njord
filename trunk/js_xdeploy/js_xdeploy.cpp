/*
 * Copyright 2009 Drew University
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); 
 * you may not use this file except in compliance with the License. 
 * You may obtain a copy of the License at 
 *
 * 		http://www.apache.org/licenses/LICENSE-2.0
 *	
 * Unless required by applicable law or agreed to in writing, software 
 * distributed under the License is distributed on an "AS IS" BASIS, 
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
 * See the License for the specific language governing permissions and 
 * limitations under the License. 
 */

// js_xdeploy.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include <lm.h>
#include <Sddl.h>

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
	{ 0 }
};

JSBool setcomputername(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	LPWSTR newName;
	COMPUTER_NAME_FORMAT nameType = ComputerNamePhysicalNetBIOS;

	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "W /u", &newName, (DWORD*)&nameType))
	{
		JS_ReportError(cx, "Unable to parse arguments in setcomputername.");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	JS_EndRequest(cx);

	*rval = SetComputerNameEx(nameType, newName) != 0 ? JSVAL_TRUE : JSVAL_FALSE;
	return JS_TRUE;
}

JSBool netgetjoinableous(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	LPWSTR domain, account, password;
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "W W W", &domain, &account, &password))
	{
		JS_ReportError(cx, "Error parsing arguments in NetGetJoinableOUs");
		JS_EndRequest(cx);
	}

	JS_YieldRequest(cx);
	DWORD ouCount = 0;
	LPWSTR * ous = NULL;
	DWORD status = NetGetJoinableOUs(NULL, domain, (JSVAL_IS_NULL(argv[1]) ? NULL : account), (JSVAL_IS_NULL(argv[2]) ? NULL : password), &ouCount, &ous);
	if(status != NERR_Success)
	{
		JS_NewNumberValue(cx, status, rval);
		JS_EndRequest(cx);
		return JS_TRUE;
	}

	JSObject * arrayObj = JS_NewArrayObject(cx, 0, NULL);
	*rval = OBJECT_TO_JSVAL(arrayObj);
	for(DWORD i = 0; i < ouCount; i++)
	{
		JSString * curOu = JS_NewUCStringCopyZ(cx, (jschar*)ous[i]);
		JS_DefineElement(cx, arrayObj, i, STRING_TO_JSVAL(curOu), NULL, NULL, 0);
	}
	NetApiBufferFree(ous);
	JS_EndRequest(cx);
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
	DWORD result = NetJoinDomain(NULL, (LPWSTR)JS_GetStringChars(domainStr), (JSVAL_IS_NULL(argv[1]) ? NULL : (LPWSTR)JS_GetStringChars(accountOUStr)),
		(JSVAL_IS_NULL(argv[2]) ? NULL : (LPWSTR)JS_GetStringChars(accountStr)), (JSVAL_IS_NULL(argv[3]) ? NULL : (LPWSTR)JS_GetStringChars(passwordStr)), 
		joinOptions);
	if(result == NERR_Success)
	{
		*rval = JSVAL_TRUE;
		return JS_TRUE;
	}
	JSBool retBool = JS_NewNumberValue(cx, result, rval);
	JS_EndRequest(cx);
	return retBool;
}

PSID convert_jsstring_to_sid(JSContext * cx, JSString * curMemberString, DWORD * errorCode)
{
	PSID curMember;
	if(!ConvertStringSidToSid((LPWSTR)JS_GetStringChars(curMemberString), &curMember))
	{
		DWORD sidSize = 0, cbDomain;
		SID_NAME_USE peUse;
		*errorCode = GetLastError();
		JS_YieldRequest(cx);
		if(!LookupAccountName(NULL, (LPWSTR)JS_GetStringChars(curMemberString), NULL, &sidSize, NULL, &cbDomain, &peUse) && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		{
			*errorCode = GetLastError();
			return NULL;
		}
		curMember = (PSID)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sidSize);
		JS_YieldRequest(cx);
		LPTSTR domainName = (LPTSTR)HeapAlloc(GetProcessHeap(), 0, cbDomain * sizeof(TCHAR));
		if(!LookupAccountName(NULL, (LPWSTR)JS_GetStringChars(curMemberString), curMember, &sidSize, domainName, &cbDomain, &peUse))
		{
			*errorCode = GetLastError();
			HeapFree(GetProcessHeap(), 0, curMember);
			HeapFree(GetProcessHeap(), 0, domainName);
			return NULL;
		}
		HeapFree(GetProcessHeap(), 0, domainName);
		*errorCode = ERROR_SUCCESS;
	}
	else
	{
		DWORD sidSize = GetLengthSid(curMember);
		PSID retMember = (PSID)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sidSize);
		CopySid(sidSize, retMember, curMember);
		LocalFree(curMember);
		curMember = retMember;
	}
	return curMember;
}

JSBool netlocalgroupaddmembers(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JS_BeginRequest(cx);
	if(argc < 2)
	{
		JS_ReportError(cx, "Must pass members to be added.");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	LOCALGROUP_MEMBERS_INFO_0 * members;
	DWORD * lookupResult;
	DWORD memberCount = 0;
	JSString * groupName = JS_ValueToString(cx, argv[0]);
	argv[0] = STRING_TO_JSVAL(groupName);
	if(JSVAL_IS_OBJECT(argv[1]) && JS_IsArrayObject(cx, JSVAL_TO_OBJECT(argv[1])))
	{
		JSObject * memberArray;
		JS_ValueToObject(cx, argv[1], &memberArray);
		argv[1] = OBJECT_TO_JSVAL(memberArray);
		JS_GetArrayLength(cx, memberArray, (jsuint*)&memberCount);
		members = (LOCALGROUP_MEMBERS_INFO_0*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LOCALGROUP_MEMBERS_INFO_0) * memberCount);
		lookupResult = (DWORD*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DWORD) * memberCount);

		JS_EnterLocalRootScope(cx);
		for(DWORD i = 0; i < memberCount; i++)
		{
			jsval curMemberVal;
			JSString * curMemberString;
			JS_GetElement(cx, memberArray, (jsint)i, &curMemberVal);
			curMemberString = JS_ValueToString(cx, curMemberVal);
			members[i].lgrmi0_sid = convert_jsstring_to_sid(cx, curMemberString, &lookupResult[i]);
		}
		JS_LeaveLocalRootScope(cx);
	}
	else
	{
		JSString * memberString = JS_ValueToString(cx, argv[1]);
		argv[1] = STRING_TO_JSVAL(memberString);
		members = (LOCALGROUP_MEMBERS_INFO_0*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LOCALGROUP_MEMBERS_INFO_0));
		lookupResult = (DWORD*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DWORD));
		members->lgrmi0_sid = convert_jsstring_to_sid(cx, memberString, lookupResult);
		memberCount = 1;
	}

	for(DWORD i = 0; i < memberCount; i++)
	{
		if(lookupResult[i] == 0)
		{
			JS_YieldRequest(cx);
			lookupResult[i] = NetLocalGroupAddMembers(NULL, (LPWSTR)JS_GetStringChars(groupName), 0, (LPBYTE)&members[i], 1);
		}
	}

	JSObject * retArray = JS_NewArrayObject(cx, 0, NULL);
	*rval = OBJECT_TO_JSVAL(retArray);
	for(DWORD i = 0; i < memberCount; i++)
	{
		jsval curResultVal;
		JS_NewNumberValue(cx, lookupResult[i], &curResultVal);
		JS_DefineElement(cx, retArray, i, curResultVal, NULL, NULL, 0);
		HeapFree(GetProcessHeap(), 0, members[i].lgrmi0_sid);
	}
	HeapFree(GetProcessHeap(), 0, members);
	HeapFree(GetProcessHeap(), 0, lookupResult);
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

JSBool close_FirstUXWnd(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	HWND findResult = FindWindow(NULL, TEXT("FirstUXWnd"));
	*rval = JSVAL_FALSE;
	if(findResult)
	{
		if(ShowWindow(findResult, SW_HIDE))
			*rval = JSVAL_TRUE;
	}
	return JS_TRUE;
}

extern void InitCrypto(JSContext * cx, JSObject * obj);
extern void InitMsi(JSContext * cx, JSObject * obj);
extern void InitProgressDlg(JSContext * cx, JSObject * obj);

#ifdef __cplusplus
extern "C" {
#endif

BOOL __declspec(dllexport) InitExports(JSContext * cx, JSObject * global)
{
	JSFunctionSpec xdeployFunctions[] = {
		{ "SetComputerName", setcomputername, 2, 0 },
		{ "NetJoinDomain", netjoindomain, 6, 0 },
		{ "NetLocalGroupAddMembers", netlocalgroupaddmembers, 2, 0 },
		{ "NetGetLastErrorMessage", GetLastNetErrorMessage, 1, 0 },
		{ "NetGetJoinableOUs", netgetjoinableous, 3, 0 },
		{ "HideFirstUXWnd", close_FirstUXWnd, 0, 0 },
		{ 0 },
	};

	JS_BeginRequest(cx);
	JS_DefineFunctions(cx, global, xdeployFunctions);
	JS_DefineConstDoubles(cx, global, xdeployConsts);
	InitCrypto(cx, global);
	InitMsi(cx, global);
	InitProgressDlg(cx, global);
	JS_EndRequest(cx);
	return TRUE;
}

BOOL __declspec(dllexport) CleanupExports(JSContext * cx, JSObject * global)
{
	return TRUE;
}

#ifdef __cplusplus
}
#endif