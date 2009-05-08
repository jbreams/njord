#include "stdafx.h"
#include "js_wimg.h"

MatchEntry::MatchEntry(LPWSTR copyFrom)
{ 
	next = NULL;
	matchLen = wcslen(copyFrom);
	matchString = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR) * (matchLen + 1));
	wcscpy_s(matchString, matchLen + 1, copyFrom);
}

MatchEntry::~MatchEntry()
{
	HeapFree(GetProcessHeap(), 0, matchString);
}

BOOL MatchEntry::Match(LPWSTR target, LPWSTR patternStr = NULL)
{
	enum State { Exact, Any, AnyRepeat };

	LPWSTR s = target, p = matchString, q = NULL;
	if(patternStr != NULL)
		p = patternStr;
	BOOL match = TRUE;
	int state = 0;
	while(match && *p)
	{
		if(*p == '*')
		{
			state = AnyRepeat;
			q = p + 1;
		}
		else if(*p = '?')
			state = Any;
		else
			state = Exact;

		if(*s == 0)
			break;

		switch(state)
		{
		case Exact:
			match = *s == *p;
			s++;
			p++;
			break;
		case Any:
			match = TRUE;
			s++;
			p++;
			break;
		case AnyRepeat:
			match = TRUE;
			s++;
			if(*s == *q)
			{
				if(Match(s, q) == TRUE)
					p++;
			}
			break;
		}
	}
	if(state == AnyRepeat)
		return *s == *q;
	else if(state == Any)
		return *s == *p;
	else
		return match && (*s == *p);
}

MatchSet::MatchSet()
{
	InitializeCriticalSection(&setLock);
	next = NULL;
}
MatchSet::~MatchSet()
{
	ClearSet();
	DeleteCriticalSection(&setLock);
}

BOOL MatchSet::MatchMe(LPWSTR fullPath)
{
	EnterCriticalSection(&setLock);
	MatchEntry * curEntry = entriesHead;
	while(curEntry != NULL && !curEntry->Match(fullPath))
		curEntry = curEntry->next;
	LeaveCriticalSection(&setLock);
	if(curEntry == NULL)
		return FALSE;
	return TRUE;
}

void MatchSet::AddMatch(LPWSTR pattern)
{
	MatchEntry * newMatch = new MatchEntry(pattern);
	EnterCriticalSection(&setLock);
	newMatch->next = entriesHead;
	entriesHead = newMatch;
	LeaveCriticalSection(&setLock);
}

void MatchSet::ClearSet()
{
	EnterCriticalSection(&setLock);
	MatchEntry * curEntry = entriesHead;
	while(curEntry != NULL)
	{
		MatchEntry * nextEntry = curEntry->next;
		delete curEntry;
		curEntry = nextEntry;
	}
	entriesHead = NULL;
	LeaveCriticalSection(&setLock);
}

MatchSet * setHead = NULL;

DWORD ExceptionCallback(DWORD messageID, WPARAM wParam, LPARAM lParam, LPVOID lpvUserData)
{
	if(messageID != WIM_MSG_PROCESS)
		return WIM_MSG_SUCCESS;
	MatchSet * mySet = (MatchSet*)lpvUserData;
	BOOL * pfB = (LPBOOL)lParam;
	*pfB = !(mySet->MatchMe((LPWSTR)wParam));
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
	
	MatchSet * newMatchSet = new MatchSet();
	jsuint arrayLength;
	JS_GetArrayLength(cx, arrayObj, &arrayLength);
	for(jsuint i = 0; i < arrayLength; i++)
	{
		jsval curStrVal;
		JS_GetElement(cx, arrayObj, i, &curStrVal);
		JSString * curStr = JS_ValueToString(cx, curStrVal);
		newMatchSet->AddMatch((LPWSTR)JS_GetStringChars(curStr));
	}
	newMatchSet->next = setHead;
	setHead = newMatchSet;

	HANDLE hObject = (HANDLE)JS_GetPrivate(cx, obj);
	WIMUnregisterMessageCallback(hObject, (FARPROC)ExceptionCallback);
	*rval = WIMRegisterMessageCallback(hObject, (FARPROC)ExceptionCallback, newMatchSet) ? JSVAL_TRUE : JSVAL_FALSE;
	JS_EndRequest(cx);

	return JS_TRUE;	
}