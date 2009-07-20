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
#include "njord.h"
#include <jsdate.h>

void find_file_cleanup(JSContext * cx, JSObject * obj)
{
	HANDLE findHandle = JS_GetPrivate(cx, obj);
	if(findHandle != INVALID_HANDLE_VALUE && findHandle != NULL)
		FindClose(findHandle);
}

JSClass findFileClass = {
    "FindFile",  /* name */
    JSCLASS_HAS_PRIVATE,  /* flags */
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, find_file_cleanup,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
JSObject * findFileProto = NULL;

JSBool find_file(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	HANDLE findHandle = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA w32FD;
	JSObject * retObj = NULL;
	JS_BeginRequest(cx);
	if(JS_InstanceOf(cx, obj, &findFileClass, NULL))
	{
		findHandle = JS_GetPrivate(cx, obj);
		JS_YieldRequest(cx);
		if(FindNextFile(findHandle, &w32FD) == FALSE)
		{
			JS_EndRequest(cx);
			*rval = JSVAL_FALSE;
			return JS_TRUE;
		}
		retObj = obj;
		*rval = JSVAL_TRUE;
	}
	else
	{
		if(argc < 1)
		{
			JS_ReportError(cx, "Must provide at least one argument for find_first_file.");
			return JS_FALSE;
		}
		JSString * searchString = JS_ValueToString(cx, argv[0]);
		JS_YieldRequest(cx);
		findHandle = FindFirstFile((LPWSTR)JS_GetStringChars(searchString), &w32FD);
		if(findHandle == INVALID_HANDLE_VALUE)
		{
			JS_EndRequest(cx);
			*rval = JSVAL_FALSE;
			return JS_TRUE;
		}
		retObj = JS_NewObject(cx, &findFileClass, findFileProto, obj);
		*rval = OBJECT_TO_JSVAL(retObj);
	}

	jsval fileNameVal, altFileNameVal, attributes, fileSize;

	fileNameVal = STRING_TO_JSVAL(JS_NewUCStringCopyZ(cx, (jschar*)w32FD.cFileName));
	altFileNameVal = STRING_TO_JSVAL(JS_NewUCStringCopyZ(cx, (jschar*)w32FD.cAlternateFileName));
	JS_SetProperty(cx, retObj, "fileName", &fileNameVal);
	JS_SetProperty(cx, retObj, "altFileName", &altFileNameVal);

	JS_NewNumberValue(cx, w32FD.dwFileAttributes, &attributes);
	JS_NewNumberValue(cx, (jsdouble)(LONGLONG)(w32FD.nFileSizeLow | w32FD.nFileSizeHigh << 31), &fileSize);
	JS_SetProperty(cx, retObj, "attributes", &attributes);
	JS_SetProperty(cx, retObj, "fileSize", &fileSize);

	SYSTEMTIME systemTime;
	FileTimeToSystemTime(&w32FD.ftCreationTime, &systemTime);
	jsval createTime = OBJECT_TO_JSVAL(js_NewDateObject(cx, systemTime.wYear, systemTime.wMonth, systemTime.wDay, systemTime.wHour, systemTime.wMinute, systemTime.wSecond));
	FileTimeToSystemTime(&w32FD.ftLastWriteTime, &systemTime);
	jsval lastWriteTime = OBJECT_TO_JSVAL(js_NewDateObject(cx, systemTime.wYear, systemTime.wMonth, systemTime.wDay, systemTime.wHour, systemTime.wMinute, systemTime.wSecond));
	JS_SetProperty(cx, retObj, "createTime", &createTime);
	JS_SetProperty(cx, retObj, "lastWriteTime", &lastWriteTime);
	JS_SetPrivate(cx, retObj, findHandle);
	JS_EndRequest(cx);
	return JS_TRUE;
}

JSBool InitFindFile(JSContext * cx, JSObject * global)
{
	struct JSPropertySpec findFileProps[] = {
		{ "fileName", 1, JSPROP_PERMANENT | JSPROP_ENUMERATE, NULL, NULL },
		{ "altFileName", 2, JSPROP_PERMANENT | JSPROP_ENUMERATE, NULL, NULL },
		{ "attributes", 3,  JSPROP_PERMANENT | JSPROP_ENUMERATE, NULL, NULL },
		{ "fileSize", 4,  JSPROP_PERMANENT | JSPROP_ENUMERATE, NULL, NULL },
		{ "createTime", 5,  JSPROP_PERMANENT | JSPROP_ENUMERATE, NULL, NULL },
		{ "lastWriteTime", 6,  JSPROP_PERMANENT | JSPROP_ENUMERATE, NULL, NULL },
		{ 0 },
	};

	struct JSFunctionSpec findFileMethods[] = {
		{ "Next", find_file, 0, 0, 0 },
		{ 0 },
	};
	findFileProto = JS_InitClass(cx, global, NULL, &findFileClass, NULL, 0, findFileProps, findFileMethods, NULL, NULL);
	JS_DefineFunction(cx, global, "FindFile", find_file, 1, 0);
	return TRUE;
}