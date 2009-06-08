#include "stdafx.h"
#include "js_wimg.h"

MatchEntry::MatchEntry(LPWSTR copyFrom)
{ 
	next = NULL;
	fileName = FALSE;
	DWORD matchLen = wcslen(copyFrom);
	matchString = (LPWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WCHAR) * (matchLen + 1));
	if(wcsrchr(copyFrom, TEXT('\\')) == NULL)
		fileName = TRUE;
	wcscpy_s(matchString, matchLen + 1, copyFrom);
	_wcslwr_s(matchString, matchLen + 1);
}

MatchEntry::~MatchEntry()
{
	HeapFree(GetProcessHeap(), 0, matchString);
}

BOOL MatchEntry::Match(LPWSTR target)
{
	LPWSTR t = target, m = matchString;
	if(fileName) // If we're matching against a filename, advance the target to the filename portion
		t = wcsrchr(t, L'\\') + 1;
	else // Otherwise advance past the drive name ("c:\temp" -> "\temp")
		t = wcschr(t, L'\\'); 
	while(*t) // While the target has more characters
	{
		if(*m == L'*') // If the match has a wildcard
		{
			while(*m && *m == L'*') m++; // Find the next character that isn't a wild card.
			if(!*m) // If the wildcard was the last character - then it's a match. 
				return TRUE;
			while(*t && *t != *m) t++; // Find the next character in the target that isn't equal to the current match character
			if(!*t) // If the target has no more characters, then the search fails.
				return FALSE;
		}
		else if(*m == L'?') // If the match has a single-character wildcard
		{
			m++; // Advance both pointers
			t++;
			if(*t != *m) // If the next characters aren't equal, then the search equals
				return FALSE;
		}
		else // Otherwise it's just a straight string comparison
		{
			if(*m != *t)
				return FALSE;
			m++;
			t++;
		}
	}
	// If you've made it this far, the strings are equivalent.
	return TRUE; 
}

struct MatchSet sets[254] = { 0 };
BYTE setsInUse = 0;

DWORD CALLBACK ExceptionCallback(DWORD messageID, WPARAM wParam, LPARAM lParam, LPVOID lpvUserData)
{
	if(messageID != WIM_MSG_PROCESS)
		return WIM_MSG_SUCCESS;
	MatchEntry * myHead = (MatchEntry*)lpvUserData;
	LPWSTR currentFile = (LPWSTR)wParam;
	BOOL * processThis = (BOOL*)lParam;

	while(myHead != NULL && !myHead->Match((LPWSTR)wParam))
		myHead = myHead->next;
	*processThis = myHead ? FALSE : TRUE;
	return WIM_MSG_SUCCESS;
}

JSBool wimg_set_exceptions(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JSObject * arrayObj;
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "o", &arrayObj) || !JS_IsArrayObject(cx, arrayObj))
	{
		JS_ReportError(cx, "Error parsing arguments in SetExceptions. Must pass an array of strings.");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	HANDLE hObject = (HANDLE)JS_GetPrivate(cx, obj);
	BYTE i = 0;
	for(i = 0; i < setsInUse; i++)
	{
		if(sets[i].wimFile == hObject)
			break;
	}
	sets[i].wimFile = hObject;
	if(sets[i].head != NULL)
	{
		while(sets[i].head != NULL)
		{
			MatchEntry * nEntry = sets[i].head->next;
			delete sets[i].head;
			sets[i].head = nEntry;
		}
		WIMUnregisterMessageCallback(hObject, (FARPROC)ExceptionCallback);
	}

	jsuint arrayLength;
	JS_GetArrayLength(cx, arrayObj, &arrayLength);
	for(jsuint j = 0; j < arrayLength; j++)
	{
		jsval curStrVal;
		JS_GetElement(cx, arrayObj, j, &curStrVal);
		JSString * curStr = JS_ValueToString(cx, curStrVal);
		MatchEntry * me = new MatchEntry((LPWSTR)JS_GetStringChars(curStr));
		me->next = sets[i].head;
		sets[i].head = me;
	}
	
	*rval = WIMRegisterMessageCallback(hObject, (FARPROC)ExceptionCallback, sets[i].head) ? JSVAL_TRUE : JSVAL_FALSE;
	JS_EndRequest(cx);

	return JS_TRUE;	
}