#include "stdafx.h"
#include "njord.h"

JSBool fs_copy_file(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JSString * sourceStr, * destinationStr;
	JSBool overwrite = JS_TRUE;

	if(!JS_ConvertArguments(cx, argc, argv, "S S /b", &sourceStr, &destinationStr, &overwrite))
	{
		JS_ReportError(cx, "Unable to parse arguments in fs_copy_file.");
		return JS_FALSE;
	}

	jschar * source = JS_GetStringChars(sourceStr);
	jschar * destination = JS_GetStringChars(destinationStr);
	*rval = CopyFile((LPWSTR)source, (LPWSTR)destination, (BOOL)overwrite) ? JSVAL_TRUE : JSVAL_FALSE;
	return JS_TRUE;
}

JSBool fs_move_file(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JSString * sourceStr, * destinationStr;
	DWORD flags;

	if(!JS_ConvertArguments(cx, argc, argv, "S S /u", &sourceStr, &destinationStr, &flags))
	{
		JS_ReportError(cx, "Unable to parse arguments in fs_copy_file.");
		return JS_FALSE;
	}

	jschar * source = JS_GetStringChars(sourceStr);
	jschar * destination = JS_GetStringChars(destinationStr);

	*rval = MoveFileEx((LPWSTR)source, (LPWSTR)destination, flags) ? JSVAL_TRUE : JSVAL_FALSE;
	return JS_TRUE;
}
JSBool fs_delete_file(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JSString * sourceStr = JS_ValueToString(cx, argv[0]);
	jschar * source = JS_GetStringChars(sourceStr);

	*rval = DeleteFile((LPWSTR)source) ? JSVAL_TRUE : JSVAL_FALSE;
	return JS_TRUE;
}
JSBool fs_read_file(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	DWORD nBytesToRead = 0, actualRead = 0;
	HANDLE hFile = JS_GetPrivate(cx, obj);
	if(hFile == NULL)
	{
		JS_ReportError(cx, "File not opened, cannot read.");
		return JS_FALSE;
	}

	if(argc == 1)
		JS_ValueToECMAUint32(cx, argv[0], &nBytesToRead);

	if(nBytesToRead == 0)
	{
		LARGE_INTEGER fileSize;
		GetFileSizeEx(hFile, &fileSize);
		nBytesToRead = fileSize.LowPart;
	}

	LPSTR localBuffer = (LPSTR)JS_malloc(cx, nBytesToRead);

	if(ReadFile(hFile, localBuffer, nBytesToRead, &actualRead, NULL) == FALSE || actualRead == 0)
	{
		JS_free(cx, localBuffer);
		*rval = JSVAL_FALSE;
		return JS_TRUE;
	}
	
	jsval lastReadVal;
	JSString * retString = NULL;
	if(IsTextUnicode(localBuffer, actualRead, NULL))
	{
		retString = JS_NewUCString(cx, (jschar*)localBuffer, actualRead / sizeof(jschar));
		JS_NewNumberValue(cx, actualRead / sizeof(jschar), &lastReadVal);
	}
	else
	{
		retString = JS_NewString(cx, localBuffer, actualRead);
		JS_NewNumberValue(cx, actualRead, &lastReadVal);
	}
	JS_SetProperty(cx, obj, "lastReadBytes", &lastReadVal);
	*rval = STRING_TO_JSVAL(retString);
	return JS_TRUE;
}
JSBool fs_write_file(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JSString * strToWrite;
	JSBool unicode = JS_TRUE, closeOnWrite = JS_FALSE;
	HANDLE hFile = JS_GetPrivate(cx, obj);
	if(hFile == NULL)
	{
		JS_ReportError(cx, "File not opened, cannot read.");
		return JS_FALSE;
	}

	if(!JS_ConvertArguments(cx, argc, argv, "S /b", &strToWrite, closeOnWrite))
	{
		JS_ReportError(cx, "Unable to parse arguments in fs_write_file.");
		return JS_FALSE;
	}

	jsval unicodeValue;
	JS_GetProperty(cx, obj, "Unicode", &unicodeValue);
	unicode = JS_ValueToBoolean(cx, unicodeValue, &unicode);

	LPBYTE toWrite = (LPBYTE)JS_GetStringChars(strToWrite);
	DWORD toWriteLen = JS_GetStringLength(strToWrite) * sizeof(jschar), actualWrite;
	if(!unicode)
	{
		DWORD ansiLen = WideCharToMultiByte(CP_UTF8, 0, (LPWSTR)toWrite, -1, NULL, 0, NULL, NULL) + 1;
		toWrite = (LPBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ansiLen);
		toWriteLen = WideCharToMultiByte(CP_UTF8, 0, (LPWSTR)JS_GetStringChars(strToWrite), -1, (LPSTR)toWrite, ansiLen, NULL, NULL);
	}
	if(WriteFile(hFile, (LPVOID)toWrite, toWriteLen, &actualWrite, NULL))
		*rval = JSVAL_TRUE;
	else
		*rval = JSVAL_FALSE;

	if(!unicode)
		HeapFree(GetProcessHeap(), 0, toWrite);
	else
		actualWrite /= sizeof(jschar);
	
	jsval lastWriteVal;
	JS_NewNumberValue(cx, actualWrite, &lastWriteVal);
	JS_SetProperty(cx, obj, "lastWriteBytes", &lastWriteVal);

	if(closeOnWrite)
	{
		CloseHandle(hFile);
		JS_SetPrivate(cx, obj, NULL);
	}
	return JS_TRUE;
}
JSBool fs_close_file(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	HANDLE hFile = JS_GetPrivate(cx, obj);
	if(hFile == NULL)
	{
		JS_ReportError(cx, "File not opened, cannot read.");
		return JS_FALSE;
	}

	CloseHandle(hFile);
	JS_SetPrivate(cx, obj, NULL);
	return JS_TRUE;
}

JSBool fs_seteof_file(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	HANDLE hFile = JS_GetPrivate(cx, obj);
	if(hFile == NULL)
	{
		JS_ReportError(cx, "File not opened, cannot read.");
		return JS_FALSE;
	}
	
	if(SetEndOfFile(hFile))
		*rval = JSVAL_TRUE;
	return JS_TRUE;
}

JSBool fs_size_file(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	LARGE_INTEGER fileSize;
	HANDLE hFile = JS_GetPrivate(cx, obj);
	if(hFile == NULL)
	{
		JS_ReportError(cx, "File not opened, cannot read.");
		return JS_FALSE;
	}

	GetFileSizeEx(hFile, &fileSize);

	JS_NewNumberValue(cx, (jsdouble)fileSize.QuadPart, rval);
	return JS_TRUE;	
}
JSBool fs_seek_file(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	HANDLE hFile = JS_GetPrivate(cx, obj);
	if(hFile == NULL)
	{
		JS_ReportError(cx, "File not opened, cannot read.");
		return JS_FALSE;
	}
	DWORD flags = FILE_CURRENT, offset;

	if(!JS_ConvertArguments(cx, argc, argv, "u /u", &offset, &flags))
	{
		JS_ReportError(cx, "Unable to parse arguments in fs_seek_file.");
		return JS_FALSE;
	}

	DWORD retCode = SetFilePointer(hFile, offset, NULL, flags);
	switch(retCode)
	{
	case INVALID_SET_FILE_POINTER:
	case ERROR_NEGATIVE_SEEK:
		*rval = JSVAL_FALSE;
		break;
	default:
		JS_NewNumberValue(cx, retCode, rval);
	}
	return JS_TRUE;
}

JSBool fs_construct_file(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JS_SetPrivate(cx, obj, NULL);
	return JS_TRUE;
}
JSBool fs_create_file(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JSString * fileNameStr = NULL;
	DWORD desiredAccess = GENERIC_READ | GENERIC_WRITE, shareMode = 0, creationDisposition = OPEN_EXISTING, flags = FILE_ATTRIBUTE_NORMAL;

	if(argc == 0)
	{
		JS_SetPrivate(cx, obj, NULL);
		return JS_TRUE;
	}

	if(!JS_ConvertArguments(cx, argc, argv, "S /u u u u", &fileNameStr, &creationDisposition, &desiredAccess, &flags, &shareMode))
	{
		JS_ReportError(cx, "Error parsing arguments in fs_create_file");
		return JS_FALSE;
	}

	HANDLE hFile = CreateFile((LPWSTR)JS_GetStringChars(fileNameStr), desiredAccess, shareMode, NULL, creationDisposition, flags, NULL);
	if(hFile == INVALID_HANDLE_VALUE)
	{
		DWORD errorCode = GetLastError();
		*rval = JSVAL_FALSE;
	}
	else
	{
		JS_SetPrivate(cx, obj, (void*)hFile);
		*rval = JSVAL_TRUE;
	}
	return JS_TRUE;
}

void fs_finalize(JSContext * cx, JSObject * obj)
{
	HANDLE hFile = JS_GetPrivate(cx, obj);
	if(hFile != NULL)
		CloseHandle(hFile);
}

struct JSClass fsClass = {
	"File", JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, fs_finalize,
	JSCLASS_NO_OPTIONAL_MEMBERS
};
JSObject * fsProto = NULL;

JSBool fs_create_file_static(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JSObject * newObject = JS_NewObject(cx, &fsClass, fsProto, obj);
	if(newObject == NULL)
	{
		JS_ReportError(cx, "Error creating new File object.");
		return JS_FALSE;
	}
	return fs_create_file(cx, newObject, argc, argv, rval);
}

JSBool InitFile(JSContext * cx, JSObject * global)
{
	JSFunctionSpec fsStaticMethods[] = {
		{ "CopyFile", fs_copy_file, 2, 0, 0 },
		{ "MoveFile", fs_move_file, 2, 0, 0 },
		{ "DeleteFile", fs_delete_file, 1, 0, 0 },
		{ "CreateFile", fs_create_file_static, 1, 0, 0 },
		{ 0 },
	};

	JSFunctionSpec fsInstanceMethods[] = {
		{ "Read", fs_read_file, 1, 0, 0 },
		{ "Write", fs_write_file, 1, 0, 0 },
		{ "Close", fs_close_file, 0, 0, 0 },
		{ "SetEOF", fs_seteof_file, 0, 0, 0 },
		{ "Size", fs_size_file, 0, 0, 0 },
		{ "Seek", fs_seek_file, 0, 0, 0 },
		{ "Create", fs_create_file, 1, 0, 0 },
		{ 0 }
	};

	JSPropertySpec fsProperties[] = {
		{"lastWriteBytes", 0, JSPROP_ENUMERATE },
		{"lastReadBytes", 1, JSPROP_ENUMERATE },
		NULL,
	};

	JSConstDoubleSpec fsConsts[] = {
		{ MOVEFILE_COPY_ALLOWED, "MOVEFILE_COPY_ALLOWED", 0, 0 },
		{ MOVEFILE_DELAY_UNTIL_REBOOT, "MOVEFILE_DELAY_UNTIL_REBOOT", 0, 0 },
		{ MOVEFILE_FAIL_IF_NOT_TRACKABLE, "MOVEFILE_FAIL_IF_NOT_TRACKABLE", 0, 0 },
		{ MOVEFILE_REPLACE_EXISTING, "MOVEFILE_REPLACE_EXISTING", 0, 0 },
		{ MOVEFILE_WRITE_THROUGH, "MOVEFILE_WRITE_THROUGH", 0, 0 },
		{ FILE_BEGIN, "FILE_BEGIN", 0, 0 },
		{ FILE_CURRENT, "FILE_CURRENT", 0, 0 },
		{ FILE_END, "FILE_END", 0, 0 },
		{ FILE_SHARE_DELETE, "FILE_SHARE_DELETE", 0, 0 },
		{ FILE_SHARE_READ, "FILE_SHARE_READ", 0, 0 },
		{ FILE_SHARE_WRITE, "FILE_SHARE_WRITE", 0, 0 },
		{ CREATE_ALWAYS, "CREATE_ALWAYS", 0, 0 },
		{ CREATE_NEW, "CREATE_NEW", 0, 0 },
		{ OPEN_ALWAYS, "OPEN_ALWAYS", 0, 0 },
		{ OPEN_EXISTING, "OPEN_EXISITNG", 0, 0 },
		{ TRUNCATE_EXISTING, "TRUNCATE_EXISTING", 0, 0 },
		{ GENERIC_READ, "GENERIC_READ", 0, 0 },
		{ GENERIC_WRITE, "GENERIC_WRITE", 0, 0 },
		{ FILE_ALL_ACCESS, "FILE_ALL_ACCESS", 0, 0 },
		{ FILE_ATTRIBUTE_ARCHIVE, "FILE_ATTRIBUTE_ARCHIVE", 0, 0 },
		{ FILE_ATTRIBUTE_ENCRYPTED, "FILE_ATTRIBUTE_ENCRYPTED", 0, 0 },
		{ FILE_ATTRIBUTE_HIDDEN, "FILE_ATTRIBUTE_HIDDEN", 0, 0 },
		{ FILE_ATTRIBUTE_NORMAL, "FILE_ATTRIBUTE_NORMAL", 0, 0 },
		{ FILE_ATTRIBUTE_OFFLINE, "FILE_ATTRIBUTE_OFFLINE", 0, 0 },
		{ FILE_ATTRIBUTE_READONLY, "FILE_ATTRIBUTE_READONLY", 0, 0 },
		{ FILE_ATTRIBUTE_SYSTEM, "FILE_ATTRIBUTE_SYSTEM", 0, 0 },
		{ FILE_ATTRIBUTE_TEMPORARY, "FILE_ATTRIBUTE_TEMPORARY", 0, 0 },
		{ FILE_FLAG_DELETE_ON_CLOSE, "FILE_FLAG_DELETE_ON_CLOSE", 0, 0 },
		{ FILE_FLAG_NO_BUFFERING, "FILE_FLAG_NO_BUFFERING", 0, 0 },
		{ FILE_FLAG_RANDOM_ACCESS, "FILE_FLAG_RANDOM_ACCESS", 0, 0 },
		{ FILE_FLAG_SEQUENTIAL_SCAN, "FILE_FLAG_SEQUENTIAL_SCAN", 0, 0 },
		{ FILE_FLAG_WRITE_THROUGH, "FILE_FLAG_WRITE_THROUGH", 0, 0 },
		{ 0, 0, 0, 0 }
	};

	JS_DefineConstDoubles(cx, global, fsConsts);
	fsProto = JS_InitClass(cx, global, NULL, &fsClass, fs_construct_file, 0, fsProperties, fsInstanceMethods, NULL, NULL);

	if(fsProto == NULL)
		return JS_FALSE;
	return JS_DefineFunctions(cx, global, fsStaticMethods);
}