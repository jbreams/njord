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
#include "jsdate.h"

JSBool fs_copy_file(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	LPWSTR source, destination;
	JSBool overwrite = JS_TRUE;

	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "W W /b", &source, &destination, &overwrite))
	{
		JS_ReportError(cx, "Unable to parse arguments in fs_copy_file.");
		return JS_FALSE;
	}
	JS_EndRequest(cx);

	*rval = CopyFile(source, destination, (BOOL)overwrite) ? JSVAL_TRUE : JSVAL_FALSE;
	return JS_TRUE;
}

JSBool fs_move_file(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	LPWSTR source, destination;
	DWORD flags = 0;

	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "W W /u", &source, &destination, &flags))
	{
		JS_ReportError(cx, "Unable to parse arguments in fs_copy_file.");
		return JS_FALSE;
	}
	JS_EndRequest(cx);

	*rval = MoveFileEx(source, destination, flags) ? JSVAL_TRUE : JSVAL_FALSE;
	return JS_TRUE;
}

JSBool fs_delete_file(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JS_BeginRequest(cx);
	JSString * sourceStr = JS_ValueToString(cx, argv[0]);
	JS_EndRequest(cx);
	jschar * source = JS_GetStringChars(sourceStr);

	*rval = DeleteFile((LPWSTR)source) ? JSVAL_TRUE : JSVAL_FALSE;
	return JS_TRUE;
}

JSBool file_set_contents(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	LPWSTR fileName = NULL;
	LPWSTR fileContents = NULL;
	JSBool unicode = JS_FALSE;
	DWORD flags = FILE_FLAG_SEQUENTIAL_SCAN;
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "W W/ * b u", &fileName, &fileContents, &unicode, &flags))
	{
		JS_ReportError(cx, "Error parsing arguments in file_set_contents");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	HANDLE fileToWrite = CreateFile(fileName, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, flags, NULL);
	if(fileToWrite == INVALID_HANDLE_VALUE)
	{
		*rval = JSVAL_FALSE;
		JS_EndRequest(cx);
		return JS_TRUE;
	}

	if(argc > 2)
	{
		if(JSVAL_IS_BOOLEAN(argv[2]))
		{
			JSBool append = JS_FALSE;
			JS_ValueToBoolean(cx, argv[2], &append);
			if(append)
				SetFilePointer(fileToWrite, 0, NULL, FILE_END);
		}
		else if(JSVAL_IS_NUMBER(argv[2]))
		{
			LARGE_INTEGER li;
			GetFileSizeEx(fileToWrite, &li);
			jsdouble fromStart = 0;
			JS_ValueToNumber(cx, argv[2], &fromStart);
			if(fromStart > 0)
			{
				if(fromStart < li.QuadPart)
					li.QuadPart = fromStart;
				SetFilePointerEx(fileToWrite, li, NULL, FILE_BEGIN);
			}
		}
	}
	else
		SetEndOfFile(fileToWrite);

	DWORD nChars = wcslen(fileContents);
	BOOL writeOK = FALSE;
	if(!unicode)
	{
		DWORD ansiLen = WideCharToMultiByte(CP_UTF8, 0, fileContents, nChars, NULL, 0, NULL, NULL);
		LPSTR toWrite = (LPSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ansiLen + 1);
		ansiLen = WideCharToMultiByte(CP_UTF8, 0, fileContents, nChars, toWrite, ansiLen, NULL, NULL);
		writeOK = WriteFile(fileToWrite, toWrite, ansiLen, &nChars, NULL);
		HeapFree(GetProcessHeap(), 0, toWrite);
	}
	else
		writeOK = WriteFile(fileToWrite, fileContents, nChars * sizeof(WCHAR), &nChars, NULL);
	CloseHandle(fileToWrite);
	*rval = (writeOK ? JSVAL_TRUE : JSVAL_FALSE);
	JS_EndRequest(cx);
	return JS_TRUE;
}

JSBool file_get_contents(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	LPWSTR fileName = NULL;
	DWORD start = 0, end = 0;
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "W/ u u", &fileName, &start, &end))
	{
		JS_ReportError(cx, "Error parsing arguments in file_set_contents");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	HANDLE fileToRead = CreateFile(fileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if(fileToRead == INVALID_HANDLE_VALUE)
	{
		*rval = JSVAL_FALSE;
		JS_EndRequest(cx);
		return JS_TRUE;
	}

	LARGE_INTEGER fileSize;
	GetFileSizeEx(fileToRead, &fileSize);
	LPBYTE buffer = (LPBYTE)JS_malloc(cx, fileSize.LowPart + sizeof(jschar));
	memset(buffer, 0, fileSize.LowPart + sizeof(jschar));
	if(buffer == NULL)
	{
		SetLastError(ERROR_OUTOFMEMORY);
		CloseHandle(fileToRead);
		*rval = JSVAL_FALSE;
		JS_EndRequest(cx);
		return JS_TRUE;
	}
	DWORD actualCount = 0;
	BOOL readOK = ReadFile(fileToRead, (LPVOID) buffer, fileSize.LowPart, &actualCount, NULL);
	CloseHandle(fileToRead);
	if(!readOK)
	{
		JS_free(cx, buffer);
		*rval = JSVAL_FALSE;
		JS_EndRequest(cx);
		return JS_TRUE;
	}

	JSString * retString = NULL;
	if(IsTextUnicode((LPVOID)buffer, actualCount, NULL))
		retString = JS_NewUCString(cx, (jschar*)buffer, actualCount / sizeof(jschar));
	else
		retString = JS_NewString(cx, (char*)buffer, actualCount);
	if(retString == NULL)
	{
		JS_ReportOutOfMemory(cx);
		JS_free(cx, buffer);
		*rval = JSVAL_FALSE;
	}
	else
		*rval = STRING_TO_JSVAL(retString);
	JS_EndRequest(cx);
	return JS_TRUE;
}

JSBool fs_create_directory(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	LPWSTR path;
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "W", &path))
	{
		JS_ReportError(cx, "Error parsing arguments in fs_create_directory");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	JS_EndRequest(cx);
	*rval = CreateDirectory(path, NULL) ? JSVAL_TRUE : JSVAL_FALSE;
	return JS_TRUE;
}

JSBool file_get_attributes(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	LPWSTR path;
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "W", &path))
	{
		JS_ReportError(cx, "Error parsing arguments in fs_get_attributes.");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	
	WIN32_FILE_ATTRIBUTE_DATA ad;
	if(!GetFileAttributesExW(path, GetFileExInfoStandard, &ad))
	{
		*rval = JSVAL_FALSE;
		JS_EndRequest(cx);
		return JS_TRUE;
	}

	JSObject * ro = JS_NewObject(cx, NULL, NULL, obj);
	*rval = OBJECT_TO_JSVAL(ro);
	jsval curVal;
	JSObject * date;
	JS_NewNumberValue(cx, ad.dwFileAttributes, &curVal);
	JS_DefineProperty(cx, ro, "attributes", curVal, NULL, NULL, JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_READONLY);

	SYSTEMTIME st;
	FileTimeToSystemTime(&ad.ftCreationTime, &st);
	date = js_NewDateObject(cx, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	JS_DefineProperty(cx, ro, "created", OBJECT_TO_JSVAL(date), NULL, NULL, JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_READONLY);

	FileTimeToSystemTime(&ad.ftLastAccessTime, &st);
	date = js_NewDateObject(cx, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	JS_DefineProperty(cx, ro, "accessed", OBJECT_TO_JSVAL(date), NULL, NULL, JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_READONLY);

	FileTimeToSystemTime(&ad.ftLastWriteTime, &st);
	date = js_NewDateObject(cx, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	JS_DefineProperty(cx, ro, "written", OBJECT_TO_JSVAL(date), NULL, NULL, JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_READONLY);

	LARGE_INTEGER li;
	li.HighPart = ad.nFileSizeHigh;
	li.LowPart = ad.nFileSizeLow;
	JS_NewNumberValue(cx, li.QuadPart, &curVal);
	JS_DefineProperty(cx, ro, "size", curVal, NULL, NULL, JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_READONLY);

	JS_EndRequest(cx);
	return JS_TRUE;
}

JSBool file_set_attributes(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	LPWSTR path;
	DWORD fileAttributes = FILE_ATTRIBUTE_NORMAL;
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "W u", &path, &fileAttributes))
	{
		JS_ReportError(cx, "Error parsing arguments in fs_get_attributes.");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	*rval = SetFileAttributes(path, fileAttributes) ? JSVAL_TRUE : JSVAL_FALSE;
	JS_EndRequest(cx);
	return JS_TRUE;
}

JSBool fs_remove_directory(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	LPWSTR path;
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "W", &path))
	{
		JS_ReportError(cx, "Error parsing arguments in fs_remove_directory");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	JS_EndRequest(cx);
	*rval = RemoveDirectory(path) ? JSVAL_TRUE : JSVAL_FALSE;
	return JS_TRUE;
}

JSBool InitFile(JSContext * cx, JSObject * global)
{
	JSFunctionSpec fsStaticMethods[] = {
		{ "CopyFile", fs_copy_file, 2, 0, 0 },
		{ "MoveFile", fs_move_file, 2, 0, 0 },
		{ "DeleteFile", fs_delete_file, 1, 0, 0 },
		{ "CreateDirectory", fs_create_directory, 1, 0, 0 },
		{ "RemoveDirectory", fs_remove_directory, 1, 0, 0 },
		{ "FileGetContents", file_get_contents, 1, 0, 0 },
		{ "FileSetContents", file_set_contents, 2, 0, 0 },
		{ "GetFileAttributes", file_get_attributes, 1, 0, 0 },
		{ "SetFileAttributes", file_set_attributes, 2, 0, 0 },
		{ 0 },
	};

	JSConstDoubleSpec fsConsts[] = {
		{ MOVEFILE_COPY_ALLOWED, "MOVEFILE_COPY_ALLOWED", 0, 0 },
		{ MOVEFILE_DELAY_UNTIL_REBOOT, "MOVEFILE_DELAY_UNTIL_REBOOT", 0, 0 },
		{ MOVEFILE_FAIL_IF_NOT_TRACKABLE, "MOVEFILE_FAIL_IF_NOT_TRACKABLE", 0, 0 },
		{ MOVEFILE_REPLACE_EXISTING, "MOVEFILE_REPLACE_EXISTING", 0, 0 },
		{ MOVEFILE_WRITE_THROUGH, "MOVEFILE_WRITE_THROUGH", 0, 0 },
		{ FILE_ATTRIBUTE_ARCHIVE, "FILE_ATTRIBUTE_ARCHIVE", 0, 0 },
		{ FILE_ATTRIBUTE_COMPRESSED, "FILE_ATTRIBUTE_COMPRESSED", 0, 0 },
		{ FILE_ATTRIBUTE_DEVICE, "FILE_ATTRIBUTE_DEVICE", 0, 0 },
		{ FILE_ATTRIBUTE_DIRECTORY, "FILE_ATTRIBUTE_DIRECTORY", 0, 0 },
		{ FILE_ATTRIBUTE_ENCRYPTED, "FILE_ATTRIBUTE_ENCRYPTED", 0, 0 },
		{ FILE_ATTRIBUTE_HIDDEN, "FILE_ATTRIBUTE_HIDDEN", 0, 0 },
		{ FILE_ATTRIBUTE_NORMAL, "FILE_ATTRIBUTE_NORMAL", 0, 0 },
		{ FILE_ATTRIBUTE_NOT_CONTENT_INDEXED, "FILE_ATTRIBUTE_NOT_CONTENT_INDEXED", 0, 0 },
		{ FILE_ATTRIBUTE_OFFLINE, "FILE_ATTRIBUTE_OFFLINE", 0, 0 },
		{ FILE_ATTRIBUTE_READONLY, "FILE_ATTRIBUTE_READONLY", 0, 0 },
		{ FILE_ATTRIBUTE_REPARSE_POINT, "FILE_ATTRIBUTE_REPARSE_POINT", 0, 0 },
		{ FILE_ATTRIBUTE_SPARSE_FILE, "FILE_ATTRIBUTE_SPARSE_FILE", 0, 0 },
		{ FILE_ATTRIBUTE_SYSTEM, "FILE_ATTRIBUTE_SYSTEM", 0, 0 },
		{ FILE_ATTRIBUTE_TEMPORARY, "FILE_ATTRIBUTE_TEMPORARY", 0, 0 },
		{ FILE_ATTRIBUTE_VIRTUAL, "FILE_ATTRIBUTE_VIRTUAL", 0, 0 },
		{ 0, 0, 0, 0 }
	};

	JS_DefineConstDoubles(cx, global, fsConsts);
	return JS_DefineFunctions(cx, global, fsStaticMethods);
}