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

typedef BOOL (__cdecl * InitExports)(JSContext * cx, JSObject * global);
typedef BOOL (__cdecl * CleanupExports)(JSContext * cx, JSObject * global);
struct ceContainer {
	LPWSTR libName;
	CleanupExports ceFn;
	HMODULE libHandle;
} * cleanupRoutines = NULL;
WORD jsExtCount = 0, craSize = 0;

JSBool xprep_load_native(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	if(!JSVAL_IS_STRING(argv[0]))
	{
		JS_ReportError(cx, "Argument passed is not a string. Must pass filename of library to load.");
		return JS_FALSE;
	}
	JS_BeginRequest(cx);
	JSString * fileNameStr = JS_ValueToString(cx, argv[0]);
	LPWSTR fileName = (LPWSTR)JS_GetStringChars(fileNameStr);
	HMODULE jsLib = LoadLibrary(fileName);
	if(jsLib == NULL)
	{
		JS_ReportError(cx, "Error in LoadLibrary.");
		return JS_FALSE;
	}

	jsrefcount rCount = JS_SuspendRequest(cx);
	InitExports ie = (InitExports)GetProcAddress(jsLib, "InitExports");
	if(ie == NULL)
	{
		JS_ReportError(cx, "Unable to load InitExports, this library may not be a JS extension. Error number %u", GetLastError());
		return JS_FALSE;
	}

	if(ie(cx, obj) == FALSE)
	{
		FreeLibrary(jsLib);
		JS_ReportError(cx, "Error in InitExports.");
		return JS_FALSE;
	}
	JS_ResumeRequest(cx, rCount);

	CleanupExports ce = (CleanupExports)GetProcAddress(jsLib, "CleanupExports");
	if(ce == NULL)
	{
		JS_ReportError(cx, "Unable to load CleanupExports, this library may not be a JS extension.");
		return JS_FALSE;
	}

	if(jsExtCount == 0)
	{
		cleanupRoutines = (struct ceContainer*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct ceContainer) * 10);
		craSize = 10;
	}
	if(jsExtCount >= craSize)
	{
		craSize += 10;
		cleanupRoutines = (struct ceContainer*)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cleanupRoutines, sizeof(struct ceContainer) * craSize);
	}
	cleanupRoutines[jsExtCount].ceFn = ce;
	cleanupRoutines[jsExtCount].libName = ::_tcsdup(fileName);
	cleanupRoutines[jsExtCount].libHandle = jsLib;
	jsExtCount++;
	*rval = JSVAL_TRUE;
	JS_EndRequest(cx);
	return JS_TRUE;
}

JSBool xprep_unload_native(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	if(jsExtCount == 0)
	{
		JS_ReportError(cx, "No libraries are loaded.");
		return JS_FALSE;
	}

	JSString * moduleName = JS_ValueToString(cx, argv[0]);
	if(moduleName == NULL)
	{
		JS_ReportError(cx, "Unable to get name, the only argument for this function is a string with the library's name.");
		return JS_FALSE;
	}

	LPWSTR nameStr = (LPWSTR)JS_GetStringChars(moduleName);
	BOOL unLoaded = FALSE;
	for(WORD i = 0; i < jsExtCount; i++)
	{
		if(::_tcsicmp(nameStr, cleanupRoutines[i].libName) == 0)
		{
			cleanupRoutines[i].ceFn(cx, obj);
			FreeLibrary(cleanupRoutines[i].libHandle);
			free(cleanupRoutines[i].libName);
			unLoaded = TRUE;
			break;
		}
	}

	if(unLoaded == FALSE)
	{
		JS_ReportError(cx, "The library specified is not currently loaded.");
		return JS_FALSE;
	}
	else
	{
		jsExtCount--;
		if(jsExtCount == 0)
		{
			HeapFree(GetProcessHeap(), 0, cleanupRoutines);
			cleanupRoutines = NULL;
			craSize = 0;
		}
	}
	return JS_TRUE;
}

JSBool xprep_unload_all_native(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	if(jsExtCount == 0)
		return JS_TRUE;

	for(WORD i = 0; i < jsExtCount; i++)
	{
		if(cleanupRoutines[i].ceFn == NULL)
			continue;
		cleanupRoutines[i].ceFn(cx, obj);
	//	FreeLibrary(cleanupRoutines[i].libHandle);
		free(cleanupRoutines[i].libName);
	}
	jsExtCount = 0;
	HeapFree(GetProcessHeap(), 0, cleanupRoutines);
	cleanupRoutines = NULL;
	return JS_TRUE;
}

JSBool load_js_lib(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JS_BeginRequest(cx);
	JSString * fileName = JS_ValueToString(cx, argv[0]);

	LPWSTR loadedFileContents = LoadFile((LPWSTR)JS_GetStringChars(fileName));
	if(loadedFileContents == NULL)
		*rval = JSVAL_FALSE;
	else
	{
		if(JS_EvaluateUCScript(cx, obj, loadedFileContents, wcslen(loadedFileContents), JS_GetStringBytes(fileName), 1, rval))
			*rval = JSVAL_TRUE;
		else
			*rval = JSVAL_FALSE;
		HeapFree(GetProcessHeap(), 0, loadedFileContents);
	}
	JS_EndRequest(cx);
	return JS_TRUE;
}

JSBool InitNativeLoad(JSContext * cx, JSObject * global)
{
	JSFunctionSpec nativeLoadMethods[] = {
		{ "LoadNative", xprep_load_native, 1, 0, 0 },
		{ "UnloadNative", xprep_unload_native, 1, 0, 0 },
		{ "UnloadAllNative", xprep_unload_all_native, 0, 0, 0 },
		{ "LoadJSLib", load_js_lib, 1, 0, 0 },
		{ 0, 0, 0, 0, 0 }
	};
	return JS_DefineFunctions(cx, global, nativeLoadMethods);
}