#include "stdafx.h"
#include <commdlg.h>

JSBool openfiledlg(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JSObject * filterArray = NULL;
	LPWSTR initialDirectory = NULL;
	LPWSTR dlgTitle = NULL;
	LPWSTR defaultExtension = NULL;
	JSBool save = JS_FALSE;
	
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "W W W o/ b", &initialDirectory, &defaultExtension, &dlgTitle, &filterArray, &save))
	{
		JS_ReportError(cx, "Error parsing arguments in OpenFileDialog");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	jsuint nFilters;
	JS_GetArrayLength(cx, filterArray, &nFilters);
	LPWSTR filterBuffer = (LPWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (50 * nFilters) * sizeof(WCHAR));
	long filterBufferUsed = 0, filterBufferSize = (50 * nFilters);
	for(jsuint i = 0; i < nFilters; i++)
	{
		jsval curFilter;
		JS_GetElement(cx, filterArray, i, &curFilter);
		JSString * curFilterString = JS_ValueToString(cx, curFilter);
		LPWSTR curFilterRaw = (LPWSTR)JS_GetStringChars(curFilterString);
		int delta = wcslen(curFilterRaw);
		if(filterBufferSize - ( 2 + JS_GetStringLength(curFilterString) + filterBufferUsed) <= 0)
		{
			filterBufferSize += 50;
			HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, filterBuffer, filterBufferSize * sizeof(WCHAR));
		}
		wcscpy_s(filterBuffer + filterBufferUsed, filterBufferSize - filterBufferUsed, (LPWSTR)JS_GetStringChars(curFilterString));
		filterBufferUsed += JS_GetStringLength(curFilterString) + 1;
	}
	filterBuffer[filterBufferUsed] = TEXT('\0');


	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.lpstrFilter = filterBuffer;
	ofn.nFilterIndex = 0;
	ofn.lpstrFile = (LPWSTR)JS_malloc(cx, 260 * sizeof(WCHAR));
	memset(ofn.lpstrFile, 0, sizeof(WCHAR) * 260);
	ofn.nMaxFile = 260;
	ofn.lpstrFileTitle = (LPWSTR)JS_malloc(cx, 260 * sizeof(WCHAR));
	memset(ofn.lpstrFileTitle, 0, sizeof(WCHAR) * 260);
	ofn.nMaxFileTitle = 260;
	ofn.lpstrInitialDir = initialDirectory;
	ofn.lpstrTitle = dlgTitle;
	ofn.lpstrDefExt = defaultExtension;
	jsrefcount rCount = JS_SuspendRequest(cx);
	BOOL ok;
	if(save)
		ok = GetSaveFileName(&ofn);
	else
		ok = GetOpenFileName(&ofn);
	DWORD errorCode = CommDlgExtendedError();
	HeapFree(GetProcessHeap(), 0, filterBuffer);
	JS_ResumeRequest(cx, rCount);
	if(!ok)
	{
		JS_free(cx, ofn.lpstrFile);
		JS_free(cx, ofn.lpstrFileTitle);
		*rval = JSVAL_FALSE;
		return JS_TRUE;
	}

	JSObject * retObj = JS_NewObject(cx, NULL, NULL, obj);
	*rval = OBJECT_TO_JSVAL(retObj);

	jsval filePathVal = STRING_TO_JSVAL(JS_NewUCString(cx, (jschar*)ofn.lpstrFile, wcslen(ofn.lpstrFile)));
	JS_DefineProperty(cx, retObj, "filePath", filePathVal, NULL, NULL, JSPROP_PERMANENT | JSPROP_ENUMERATE);
	jsval fileTitleVal = STRING_TO_JSVAL(JS_NewUCString(cx, (jschar*)ofn.lpstrFileTitle, wcslen(ofn.lpstrFileTitle)));
	JS_DefineProperty(cx, retObj, "fileTitle", fileTitleVal, NULL, NULL, JSPROP_PERMANENT | JSPROP_ENUMERATE);
	JS_EndRequest(cx);
	return JS_TRUE;
}