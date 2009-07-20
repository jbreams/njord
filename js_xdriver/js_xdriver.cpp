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

#include "stdafx.h"
#include "difxapi.h"
#include <msi.h>
#include <commctrl.h>
#include <Wincrypt.h>
#pragma comment(lib, "crypt32.lib")

JSBool xdriver_install_inf(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	LPWSTR infPath;
	DWORD flags = 0;
	BOOL needsReboot;
	JSBool preInstall = JS_FALSE;

	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "W/ u b", &infPath, &flags, &preInstall))
	{
		JS_ReportError(cx, "Error parsing arguments in DriverPackageInstall");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	JS_EndRequest(cx);

	DWORD errorCode = ERROR_SUCCESS;
	if(preInstall)
		errorCode = DriverPackagePreinstall(infPath, flags);
	else
		errorCode = DriverPackageInstall(infPath, flags, NULL, &needsReboot);
	*rval = errorCode == ERROR_SUCCESS ? JSVAL_TRUE : JSVAL_FALSE;
	if(errorCode != ERROR_SUCCESS)
		SetLastError(errorCode);
	return JS_TRUE;
}

JSBool xdriver_uninstall_inf(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	LPWSTR infPath;
	DWORD flags = 0;
	BOOL needsReboot;
	JSBool preInstall = JS_FALSE;

	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "W/ u b", &infPath, &flags, &preInstall))
	{
		JS_ReportError(cx, "Error parsing arguments in DriverPackageInstall");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	JS_EndRequest(cx);

	DWORD errorCode = DriverPackageUninstall(infPath, flags, NULL, &needsReboot);
	*rval = errorCode == ERROR_SUCCESS ? JSVAL_TRUE : JSVAL_FALSE;
	if(errorCode != ERROR_SUCCESS)
		SetLastError(errorCode);
	return JS_TRUE;
}

JSBool xdriver_get_inf_path(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	LPWSTR infPath;
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "W", &infPath))
	{
		JS_ReportError(cx, "Error parsing arguments in DriverPackageGetPath");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	DWORD bufSize = 0;
	if(DriverPackageGetPath(infPath, NULL, &bufSize) != ERROR_INSUFFICIENT_BUFFER)
	{
		*rval = JSVAL_FALSE;
		JS_EndRequest(cx);
		return JS_TRUE;
	}
	LPWSTR buffer = (LPWSTR)JS_malloc(cx, sizeof(WCHAR) * bufSize);
	DWORD errorCode = DriverPackageGetPath(infPath, buffer, &bufSize);
	if(errorCode != ERROR_SUCCESS)
	{
		JS_free(cx, buffer);
		*rval = JSVAL_FALSE;
		SetLastError(errorCode);
		JS_EndRequest(cx);
		return JS_TRUE;
	}

	JSString * retStr = JS_NewUCString(cx, (jschar*)buffer, bufSize);
	*rval = STRING_TO_JSVAL(retStr);
	JS_EndRequest(cx);
	return JS_TRUE;
}

JSBool msi_set_internal_ui(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JSObject * ownerObject = NULL;
	DWORD uiLevel;

	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "u /o", &uiLevel, &ownerObject))
	{
		JS_ReportError(cx, "Error parsing arguments in DriverPackageGetPath");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	JS_EndRequest(cx);

	HWND ownerWindow = NULL;
	if(ownerObject != NULL)
		ownerWindow = (HWND)JS_GetPrivate(cx, ownerObject);
	MsiSetInternalUI((INSTALLUILEVEL)uiLevel, &ownerWindow);
	return JS_TRUE;
}

JSBool msi_install_product(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	LPWSTR packagePath;
	JSObject * propObj;
	LPWSTR propString;
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "W *", &packagePath))
	{
		JS_ReportError(cx, "Error parsing arguments in DriverPackageGetPath");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	if(JSVAL_IS_STRING(argv[1]))
	{
		JSString * propStringStr = JS_ValueToString(cx, argv[1]);
		propString = (LPWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (JS_GetStringLength(propStringStr) + 1) * sizeof(WCHAR));
		wcscpy_s(propString, JS_GetStringLength(propStringStr) + 1, (LPWSTR)JS_GetStringChars(propStringStr));
	}
	else if(JSVAL_IS_OBJECT(argv[1]))
	{
		LPWSTR outString = (LPWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 512 * sizeof(WCHAR));
		DWORD used = 0, size = 512;
		JS_ValueToObject(cx, argv[1], &propObj);
		JSIdArray * ids = JS_Enumerate(cx, propObj);
		for(jsint i = 0; i < ids->length; i++)
		{
			jsval curPropVal;
			JS_IdToValue(cx, ids->vector[i], &curPropVal);
			JSString * curPropName = JS_ValueToString(cx, curPropVal);
			JS_AddRoot(cx, curPropName);
			JS_GetUCProperty(cx, propObj, JS_GetStringChars(curPropName), JS_GetStringLength(curPropName), &curPropVal);
			JSString * curPropValue = JS_ValueToString(cx, curPropVal);

			DWORD CurPropPairLen = JS_GetStringLength(curPropName) + JS_GetStringLength(curPropValue) + 2;
			LPWSTR CurPropPair = (LPWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WCHAR) * CurPropPairLen);
			wsprintf(CurPropPair, TEXT("%s=%s "), JS_GetStringChars(curPropName), JS_GetStringChars(curPropValue));
			JS_RemoveRoot(cx, curPropName);
			if(used + wcslen(CurPropPair) > size)
			{
				size += 128;
				outString = (LPWSTR)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, outString, size * sizeof(WCHAR));
			}
			wcscat_s(outString, size - used, CurPropPair);
			used += wcslen(CurPropPair);
		}
		JS_DestroyIdArray(cx, ids);
		DWORD len = wcslen(outString);
		outString[len - 1] = 0;
	}

	jsrefcount rCount = JS_SuspendRequest(cx);
	UINT errorCode = MsiInstallProductW(packagePath, propString);
	JS_ResumeRequest(cx, rCount);
	HeapFree(GetProcessHeap(), 0, propString);
	if(errorCode == ERROR_SUCCESS)
		*rval = JSVAL_TRUE;
	else
	{
		*rval = JSVAL_FALSE;
		SetLastError(errorCode);
	}
	JS_EndRequest(cx);
	return JS_TRUE;
}


void finalizeUI(JSContext * cx, JSObject * uiObj)
{
	HWND myWnd = (HWND)JS_GetPrivate(cx, uiObj);
	SendMessage(myWnd, WM_DESTROY, 0, 0);
}
JSClass xDriverUI = {	"xDriverUI",  /* name */
    JSCLASS_HAS_PRIVATE,  /* flags */
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, finalizeUI,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
JSObject * xDriverUIProto = NULL;

extern HMODULE myModule;

BOOL xDriver_DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
		return 1;
	case WM_DESTROY:
		PostQuitMessage(0);
	}
	return 0;
}
DWORD xDriverUIThread(LPVOID param)
{
	HWND * myWindow = (HWND*)param;
	INITCOMMONCONTROLSEX icce;
	icce.dwSize = sizeof(icce);
	icce.dwICC = ICC_PROGRESS_CLASS;
	BOOL ok = InitCommonControlsEx(&icce);
	if(!ok)
	{
		MessageBox(NULL, TEXT("Error initialzing common controls. Cannot continue."), TEXT("xDriver"), MB_OK);
		return 1;
	}

	*myWindow = CreateDialogW(myModule, MAKEINTRESOURCE(IDD_DIALOG), NULL, (DLGPROC)xDriver_DlgProc);
	if(*myWindow == NULL)
	{
		MessageBox(NULL, TEXT("Error creating window. Cannot continue."), TEXT("xDriver"), MB_OK);
		return 1;
	}
	DWORD errorCode = GetLastError();
	ShowWindow(*myWindow, SW_SHOW);
	MSG msg;
	while(GetMessage(&msg, NULL, 0, 0))
	{
		if(!IsDialogMessage(*myWindow, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return 0;
}

JSBool create_ui(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	HWND myWnd = NULL;
	HANDLE uiThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)xDriverUIThread, &myWnd, 0, NULL);
	CloseHandle(uiThread);
	while(myWnd == NULL)
		Sleep(5);

	JS_BeginRequest(cx);
	JSObject * retObj = JS_NewObject(cx, &xDriverUI, xDriverUIProto, obj);
	*rval = OBJECT_TO_JSVAL(retObj);
	JS_SetPrivate(cx, retObj, myWnd);
	JS_EndRequest(cx);
	return JS_TRUE;
}

JSBool set_static_text(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	LPWSTR message;
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "W", &message))
	{
		JS_ReportError(cx, "Error parsing arguments in SetStaticText");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	JS_EndRequest(cx);

	HWND myWnd = (HWND)JS_GetPrivate(cx, obj);
	SetDlgItemText(myWnd, IDC_STATICTEXT, message);
	return JS_TRUE;
}

JSBool set_progress(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	WORD pos;
	BOOL max = FALSE;
	JS_BeginRequest(cx);
	JS_ConvertArguments(cx, argc, argv, "c b", &pos, &max);
	JS_EndRequest(cx);

	HWND myWnd = (HWND)JS_GetPrivate(cx, obj);
	if(max)
		SendDlgItemMessage(myWnd, IDC_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0, pos));
	else
		SendDlgItemMessage(myWnd, IDC_PROGRESS, PBM_SETPOS, pos, 0);
	return JS_TRUE;
}

char bytetochar(BYTE c)
{
	if(c >= 0x00 && c <= 0x09)
		return '0' + c;
	if(c >= 0x0a && c <= 0x0f)
		return 'a' + (c - 0x0a);
	return 'f';
}

JSBool hashsomearbitrarystuff(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	*rval = JSVAL_FALSE;
	if(argc == 0)
		return JS_TRUE;

	HCRYPTPROV hProv; 
	if(!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
		return JS_TRUE;

	HCRYPTHASH hHash;
	if(!CryptCreateHash(hProv, CALG_MD5, NULL, 0, &hHash))
	{
		CryptReleaseContext(hProv, 0);
		return JS_TRUE;
	}

	JS_BeginRequest(cx);
	JSString * inString = JS_ValueToString(cx, *argv);
	CryptHashData(hHash, (LPBYTE)JS_GetStringChars(inString), JS_GetStringLength(inString) * sizeof(jschar), 0);

	DWORD hashLen;
	DWORD bufSize = sizeof(DWORD);
	CryptGetHashParam(hHash, HP_HASHSIZE, (LPBYTE)&hashLen, &bufSize, 0);
	LPBYTE theHash = (LPBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, hashLen);
	CryptGetHashParam(hHash, HP_HASHVAL, theHash, &hashLen, 0);
	CryptDestroyHash(hHash);
	CryptReleaseContext(hProv, 0);

	bufSize = (hashLen * 2) + 1;
	LPSTR hashString = (LPSTR)JS_malloc(cx, bufSize);
	memset(hashString, 0, bufSize);
	DWORD c = 0;
	for(DWORD i = 0; i < hashLen; i++)
	{
		hashString[c++] = bytetochar(theHash[i] >> 4);
		hashString[c++] = bytetochar(theHash[i] & 0x0f);
	}
	HeapFree(GetProcessHeap(), 0, theHash);
	JSString * outString = JS_NewString(cx, hashString, bufSize);
	*rval = STRING_TO_JSVAL(outString);
	JS_EndRequest(cx);
	return JS_TRUE;	
}

#ifdef __cplusplus
extern "C" {
#endif

BOOL __declspec(dllexport) InitExports(JSContext * cx, JSObject * global)
{
	struct JSConstDoubleSpec xdriver_consts[] = {
		{ DRIVER_PACKAGE_FORCE, "DRIVER_PACKAGE_FORCE", 0, 0 },
		{ DRIVER_PACKAGE_LEGACY_MODE, "DRIVER_PACKAGE_LEGACY_MODE", 0, 0 },
		{ DRIVER_PACKAGE_ONLY_IF_DEVICE_PRESENT, "DRIVER_PACKAGE_ONLY_IF_DEVICE_PRESENT", 0, 0 },
		{ DRIVER_PACKAGE_REPAIR, "DRIVER_PACKAGE_REPAIR", 0, 0 },
		{ DRIVER_PACKAGE_SILENT, "DRIVER_PACKAGE_SILENT", 0, 0 },
		{ DRIVER_PACKAGE_DELETE_FILES, "DRIVER_PACKAGE_DELETE_FILES", 0, 0 },
		{ INSTALLUILEVEL_NONE, "INSTALLUILEVEL_NONE", 0, 0 },
		{ INSTALLUILEVEL_BASIC, "INSTALLUILEVEL_BASIC", 0, 0 },
		{ INSTALLUILEVEL_REDUCED, "INSTALLUILEVEL_REDUCED", 0, 0 },
		{ INSTALLUILEVEL_FULL, "INSTALLUILEVEL_FULL", 0, 0 },
		{ 0 }
	};

	JSFunctionSpec xdriver_funcs[] = {
		{ "DriverPackageInstall", xdriver_install_inf, 2, 0, 0 },
		{ "DriverPackageGetPath", xdriver_get_inf_path, 1, 0, 0 },
		{ "DriverPackageUninstall", xdriver_uninstall_inf, 2, 0, 0 },
		{ "MsiInstallProduct", msi_install_product, 2, 0, 0 },
		{ "MsiSetInternalUI", msi_set_internal_ui, 2, 0, 0 },
		{ "xDriverCreateDialog", create_ui, 0, 0, 0 },
		{ "MD5", hashsomearbitrarystuff, 1, 0, 0 },
		{ 0 },
	};

	JSFunctionSpec ui_funcs[] = {
		{ "SetStaticText", set_static_text, 1, 0, 0 },
		{ "SetProgress", set_progress, 2, 0, 0 },
		{ 0 }
	};

	JS_DefineFunctions(cx, global, xdriver_funcs);
	xDriverUIProto = JS_InitClass(cx, global, NULL, &xDriverUI, NULL, 0, NULL, ui_funcs, NULL, NULL);
	JS_DefineConstDoubles(cx, global, xdriver_consts);
	return TRUE;
}

BOOL __declspec(dllexport) CleanupExports(JSContext * cx, JSObject * global)
{

	return TRUE;
}

#ifdef __cplusplus
}
#endif