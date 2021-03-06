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
#include "js_wimg.h"
#include "jsxml.h"

HMODULE myModule;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	myModule = hModule;
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
DWORD threadID = 0;

void wimcleanup(JSContext * cx, JSObject * obj)
{
	HANDLE wimHandle = JS_GetPrivate(cx, obj);
	if(wimHandle != NULL)
		WIMCloseHandle(wimHandle);
}

void wimfilecleanup(JSContext * cx, JSObject * obj)
{
	HANDLE wimHandle = JS_GetPrivate(cx, obj);
	if(wimHandle == NULL)
		return;
	if(threadID != 0)
	{
		while(!PostThreadMessage(threadID, WM_APP + 2, 0, (LPARAM)wimHandle))
			Sleep(200);
	}
	WIMUnregisterMessageCallback(wimHandle, NULL);
	WIMCloseHandle(wimHandle);
}

JSClass wimFileClass = 	{
	"WIMFile",  /* name */
    JSCLASS_HAS_PRIVATE,  /* flags */
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, wimfilecleanup,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
JSObject * wimFileProto;

JSBool wimg_create_file(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	LPWSTR filePath = NULL;
	DWORD desiredAccess = WIM_GENERIC_READ,
		creationDisposition = WIM_OPEN_EXISTING,
		flags = 0,
		compressionType = WIM_COMPRESS_LZX,
		creationResult;

	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "W /u u u u", &filePath, &desiredAccess, &creationDisposition, &flags, &compressionType))
	{
		JS_ReportError(cx, "Error during argument parsing in WIMCreateFile");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	JS_YieldRequest(cx);
	HANDLE wimFile = WIMCreateFile(filePath, desiredAccess, creationDisposition, flags, compressionType, &creationResult);
	if(wimFile == NULL)
	{
		*rval = JSVAL_FALSE;
		JS_EndRequest(cx);
		return JS_TRUE;
	}

	JSObject * retObj = JS_NewObject(cx, &wimFileClass, wimFileProto, obj);
	*rval = OBJECT_TO_JSVAL(retObj);
	JS_SetPrivate(cx, retObj, wimFile);
	JS_EndRequest(cx);
	return JS_TRUE;
}

JSBool wimg_close_handle(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	HANDLE hObject = (HANDLE)JS_GetPrivate(cx, obj);
	*rval = WIMCloseHandle(hObject) ? JSVAL_TRUE : JSVAL_FALSE;
	JS_SetPrivate(cx, obj, NULL);
	return JS_TRUE;
}

JSBool wimg_set_temporary_path(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	HANDLE wimFile = (HANDLE)JS_GetPrivate(cx, obj);
	LPWSTR path = NULL;
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "W", &path))
	{
		JS_ReportError(cx, "Error during argument parsing in WIMGSetTemporaryPath");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	JS_EndRequest(cx);
	*rval = WIMSetTemporaryPath(wimFile, path) ? JSVAL_TRUE : JSVAL_FALSE;
	return JS_TRUE;
}

JSBool wimg_get_image_count(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	HANDLE wimFile = JS_GetPrivate(cx, obj);
	JS_BeginRequest(cx);
	JSBool retBool = JS_NewNumberValue(cx, WIMGetImageCount(wimFile), rval);
	JS_EndRequest(cx);
	return retBool;
}
JSBool wimg_delete_image(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	HANDLE wimFile = JS_GetPrivate(cx, obj);
	DWORD imageIndex;
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "u", &imageIndex))
	{
		JS_ReportError(cx, "Error during argument parsing in WIMDeleteImage");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	JS_YieldRequest(cx);
	*rval = WIMDeleteImage(wimFile, imageIndex) ? JSVAL_TRUE : JSVAL_FALSE;
	JS_EndRequest(cx);
	return JS_TRUE;
}

JSClass wimImageClass = {
	"WIMImage",  /* name */
    JSCLASS_HAS_PRIVATE,  /* flags */
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, wimcleanup,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
JSObject * wimImageProto;

JSBool wimg_capture_image(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	LPWSTR path;
	DWORD captureFlags = 0;
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "W /u", &path, &captureFlags))
	{
		JS_ReportError(cx, "Error during argument parsing in WIMCaptureImage");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	if(wcslen(path) > MAX_PATH)
	{
		*rval = JSVAL_FALSE;
		JS_EndRequest(cx);
		return JS_TRUE;
	}
	jsrefcount rCount = JS_SuspendRequest(cx);
	HANDLE wimFile = JS_GetPrivate(cx, obj);
	HANDLE newImage = WIMCaptureImage(wimFile, path, captureFlags);
	JS_ResumeRequest(cx, rCount);
	if(newImage == NULL)
		*rval = JSVAL_FALSE;
	else
	{
		JSObject * newImageObj = JS_NewObject(cx, &wimImageClass, wimImageProto, obj);
		*rval = OBJECT_TO_JSVAL(newImageObj);
		JS_SetPrivate(cx, newImageObj, newImage);
	}
	JS_EndRequest(cx);
	return JS_TRUE;
}

JSBool wimg_load_image(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	DWORD imageIndex;
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "u", &imageIndex))
	{
		JS_ReportError(cx, "Error during argument parsing in WIMLoadImage");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	JS_YieldRequest(cx);
	HANDLE hImage = WIMLoadImage(JS_GetPrivate(cx, obj), imageIndex);
	if(hImage == NULL)
	{
		*rval = JSVAL_FALSE;
		JS_EndRequest(cx);
		return JS_TRUE;
	}
	JSObject * retObj = JS_NewObject(cx, &wimImageClass, wimImageProto, obj);
	*rval = OBJECT_TO_JSVAL(retObj);
	JS_SetPrivate(cx, retObj, hImage);
	JS_EndRequest(cx);
	return JS_TRUE;
}

JSBool wimg_apply_image(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	DWORD applyFlags = 0;
	LPWSTR path = NULL;
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "W /u", &path, &applyFlags))
	{
		JS_ReportError(cx, "Error during argument parsing in WIMApplyImage");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	HANDLE imageHandle = JS_GetPrivate(cx, obj);
	jsrefcount rCount = JS_SuspendRequest(cx);
	*rval = WIMApplyImage(imageHandle, path, applyFlags) ? JSVAL_TRUE : JSVAL_FALSE;
	JS_ResumeRequest(cx, rCount);
	if(*rval == JSVAL_FALSE)
	{
		JS_EndRequest(cx);
		return JS_TRUE;
	}

	TCHAR volumeName[MAX_PATH + 1];
	memset(volumeName, 0, sizeof(TCHAR) * (MAX_PATH + 1));
	_tcscat_s(volumeName, MAX_PATH + 1, TEXT("\\\\.\\"));
	_tcscat_s(volumeName, (MAX_PATH + 1) - 4, path);

	if(!GetVolumePathName(volumeName, volumeName, MAX_PATH + 1))
	{
		JS_EndRequest(cx);
		*rval = JSVAL_FALSE;
		return JS_TRUE;
	}
	
	LPTSTR lastSlash = _tcsrchr(volumeName, _T('\\'));
	*lastSlash = '\0';

	HANDLE volumeHandle = CreateFile(volumeName, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if(volumeHandle == INVALID_HANDLE_VALUE)
	{
		JS_EndRequest(cx);
		*rval = JSVAL_FALSE;
		return JS_TRUE;
	}
	
	rCount = JS_SuspendRequest(cx);
	if(!FlushFileBuffers(volumeHandle))
	{
		CloseHandle(volumeHandle);
		JS_ResumeRequest(cx, rCount);
		JS_EndRequest(cx);
		*rval = JSVAL_FALSE;
		return JS_TRUE;
	}
	CloseHandle(volumeHandle);
	JS_ResumeRequest(cx, rCount);
	*rval = JSVAL_TRUE;
	JS_EndRequest(cx);
	return JS_TRUE;
}

JSBool wimg_start_ui(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	HANDLE hWim = JS_GetPrivate(cx, obj);
	if(threadID == 0)
	{
		HANDLE threadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)uiThread, (LPVOID)myModule, 0, &threadID);
		if(threadHandle = INVALID_HANDLE_VALUE)
		{
			*rval = JSVAL_FALSE;
			return JS_TRUE;
		}
	}

	while(!PostThreadMessage(threadID, WM_APP + 1, 0, (LPARAM)hWim))
		Sleep(200);
	*rval = JSVAL_TRUE;
	return JS_TRUE;
}

JSBool wimg_stop_ui(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	HANDLE hWim = JS_GetPrivate(cx, obj);
	if(threadID != 0)
	{
		while(!PostThreadMessage(threadID, WM_APP + 2, 0, (LPARAM)hWim))
			Sleep(200);
	}
	return JS_TRUE;
}

JSBool wimg_image_count_getter(JSContext * cx, JSObject * obj, jsval idval, jsval * vp)
{
	JS_BeginRequest(cx);
	HANDLE hImage = JS_GetPrivate(cx, obj);
	DWORD imageCount = WIMGetImageCount(hImage);
	JS_NewNumberValue(cx, (jsdouble)imageCount, vp);
	JS_EndRequest(cx);
	return JS_TRUE;
}

JSBool wimg_image_info_getter(JSContext * cx, JSObject * obj, jsval idval, jsval * vp)
{
	JS_BeginRequest(cx);
	HANDLE hImage = JS_GetPrivate(cx, obj);
	LPVOID rawImageInfo;
	DWORD rawImageInfoSize;

	if(!WIMGetImageInformation(hImage, &rawImageInfo, &rawImageInfoSize))
		*vp = JSVAL_NULL;

	JSString * xmlStr = JS_NewUCStringCopyN(cx, (jschar*)rawImageInfo, rawImageInfoSize / sizeof(WCHAR));
	if(xmlStr == NULL)
	{
		JS_ReportError(cx, "Error creating xml string from raw image information");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	jsval xmlStrVal = STRING_TO_JSVAL(xmlStr);
	*vp = xmlStrVal;
	LocalFree(rawImageInfo);

	WIMSetImageInformation(hImage, JS_GetStringChars(xmlStr), JS_GetStringLength(xmlStr) * sizeof(jschar));
	JS_EndRequest(cx);
	return JS_TRUE;
}

JSBool wimg_image_info_setter(JSContext * cx, JSObject * obj, jsval idval, jsval * vp)
{
	JS_BeginRequest(cx);
	HANDLE hImage = JS_GetPrivate(cx, obj);

	JSString * newInfo = JS_ValueToString(cx, *vp);
	*vp = STRING_TO_JSVAL(newInfo);
	
	DWORD length = JS_GetStringLength(newInfo);
	LPWSTR chars = (LPWSTR)JS_GetStringChars(newInfo);
	if(*chars != 0xfeff)
	{
		length++;
		LPWSTR back = chars;
		chars = (LPWSTR)JS_malloc(cx, sizeof(WCHAR) * (length + 1));
		memset(chars, 0, sizeof(WCHAR) * (length + 1));
		*chars = 0xfeff;
		wcscat(chars, back);
		newInfo = JS_NewUCString(cx, (jschar*)chars, sizeof(WCHAR) * length);
		*vp = STRING_TO_JSVAL(newInfo);
	}

	DWORD errorCode = 0;
	if(!WIMSetImageInformation(hImage, (LPVOID)chars, length * sizeof(WCHAR)))
		errorCode = GetLastError();
	JS_EndRequest(cx);
	return JS_TRUE;

}

JSBool openfiledlg(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval);
extern MatchSet * setHead;
#ifdef __cplusplus
extern "C" {
#endif
BOOL __declspec(dllexport) InitExports(JSContext * cx, JSObject * global)
{
	JSFunctionSpec wimFileFuncs[] = {
		{ "Close", wimg_close_handle, 0, 0 },
		{ "SetTemporaryPath", wimg_set_temporary_path, 1, 0 },
		{ "GetImageCount", wimg_get_image_count, 0, 0 },
		{ "DeleteImage", wimg_delete_image, 1, 0 },
		{ "CaptureImage", wimg_capture_image, 2, 0 },
		{ "StartUI", wimg_start_ui, 0, 0 },
		{ "StopUI", wimg_stop_ui, 0, 0 },
		{ "LoadImage", wimg_load_image, 2, 0 },
		{ "SetExceptionsList", wimg_set_exceptions, 1, 0 },
		{ 0 },
	};

	JSFunctionSpec wimImageFuncs[] = {
		{ "ApplyImage", wimg_apply_image, 2, 0 },
		{ "Close", wimg_close_handle, 0, 0 },
		{ 0 },
	};

	JSPropertySpec wimProps[] = {
		{ "information", 0, JSPROP_PERMANENT, wimg_image_info_getter, wimg_image_info_setter },
		{ "imageCount", 0, JSPROP_PERMANENT | JSPROP_READONLY, wimg_image_count_getter, NULL },
		{ 0 }
	};

	JSConstDoubleSpec wimg_consts [] = {
		{ WIM_GENERIC_READ, "WIM_GENERIC_READ", 0, 0 },
		{ WIM_GENERIC_WRITE, "WIM_GENERIC_WRITE", 0, 0, },
		{ WIM_CREATE_NEW, "WIM_CREATE_NEW", 0, 0 },
		{ WIM_CREATE_ALWAYS, "WIM_CREATE_ALWAYS", 0, 0 },
		{ WIM_OPEN_EXISTING, "WIM_OPEN_EXISTING", 0, 0 },
		{ WIM_OPEN_ALWAYS, "WIM_OPEN_ALWAYS", 0, 0 },
		{ WIM_FLAG_VERIFY, "WIM_FLAG_VERIFY", 0, 0 },
		{ WIM_FLAG_SHARE_WRITE, "WIM_FLAG_SHARE_WRITE", 0, 0 },
		{ WIM_COMPRESS_NONE, "WIM_COMPRESS_NONE", 0, 0 },
		{ WIM_COMPRESS_XPRESS, "WIM_COMPRESS_XPRESS", 0, 0 },
		{ WIM_COMPRESS_LZX, "WIM_COMPRESS_LXZ", 0, 0 },
		{ WIM_CREATED_NEW, "WIM_CREATED_NEW", 0, 0 },
		{ WIM_OPENED_EXISTING, "WIM_OPENED_EXISTING", 0, 0 },
		{ WIM_FLAG_INDEX, "WIM_FLAG_INDEX", 0, 0 },
		{ WIM_FLAG_NO_APPLY, "WIM_FLAG_NO_APPLY", 0, 0 },
		{ WIM_FLAG_FILEINFO, "WIM_FLAG_FILEINFO", 0, 0 },
		{ WIM_FLAG_NO_DIRACL, "WIM_FLAG_NO_DIRACL", 0, 0 },
		{ WIM_FLAG_NO_FILEACL, "WIM_FLAG_NO_FILEACL", 0, 0 },
		{ WIM_FLAG_NO_RP_FIX, "WIM_FLAG_NO_RP_FIX", 0, 0 },
		{ 0 },
	};

	JS_BeginRequest(cx);
	wimFileProto = JS_InitClass(cx, global, NULL, &wimFileClass, NULL, 0, wimProps, wimFileFuncs, NULL, NULL);
	wimImageProto = JS_InitClass(cx, global, NULL, &wimImageClass, NULL, 0, wimProps, wimImageFuncs, NULL, NULL);
	JS_DefineFunction(cx, global, "WIMCreateFile", wimg_create_file, 5, 0);
	JS_DefineFunction(cx, global, "GetOpenFileName", openfiledlg, 4, 0);
	JS_DefineConstDoubles(cx, global, wimg_consts);
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