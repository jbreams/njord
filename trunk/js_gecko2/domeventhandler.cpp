#include "stdafx.h"
#include "webbrowserchrome.h"
#include "domeventhandler.h"
#include "nsXULAppApi.h"
#include "nsXPComGlue.h"
#include "nsCOMPtr.h"
#include "nsNetUtil.h"
#include "nsIProxyObjectManager.h"
#include "nsProxiedService.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsComponentManagerUtils.h"
#include "nsWeakReference.h"
#include "nsStringAPI.h"
#include "privatedata.h"
#include "nsIDOMEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMWindow.h"
#include "nsIDOMNode.h"

#include "nsXPCOMCIDInternal.h"
#include "nsIProxyObjectManager.h"


EventRegistration::EventRegistration()
{
	domEvent = NULL;
	callback = NULL;
	stopBubbles = FALSE;
	windowsEvent = INVALID_HANDLE_VALUE;
}

EventRegistration::~EventRegistration()
{
	if(domEvent)
		HeapFree(GetProcessHeap(), HEAP_ZERO_MEMORY, domEvent);
	if(callback)
		HeapFree(GetProcessHeap(), HEAP_ZERO_MEMORY, callback);
	if(windowsEvent != INVALID_HANDLE_VALUE)
		CloseHandle(windowsEvent);
}

NS_IMPL_ISUPPORTS1(DOMEventListener,
				   nsIDOMEventListener)

DOMEventListener::DOMEventListener(PrivateData *aOwner)
{
	mOwner = aOwner;
}

DOMEventListener::~DOMEventListener()
{

}

NS_IMETHODIMP DOMEventListener::HandleEvent(nsIDOMEvent *aEvent)
{
	EnterCriticalSection(&mOwner->eventHeadLock);
	if(mOwner->eventHead == NULL)
	{
		LeaveCriticalSection(&mOwner->eventHeadLock);
		return NS_OK;
	}
	
	nsString eventType;
	aEvent->GetType(eventType);

	EventRegistration * curReg = mOwner->eventHead;
	while(curReg != NULL)
	{
		if(eventType.Compare(curReg->domEvent) == 0)
		{
			SetEvent(curReg->windowsEvent);
			if(curReg->stopBubbles)
				aEvent->StopPropagation();
			if(curReg->callback)
			{
				JS_BeginRequest(mOwner->mContext);
				JS_CallFunctionName(mOwner->mContext, JS_GetGlobalObject(mOwner->mContext), curReg->callback, 0, NULL, NULL);
				JS_EndRequest(mOwner->mContext);
			}
		}
		curReg = curReg->next;
	}
	LeaveCriticalSection(&mOwner->eventHeadLock);
	return NS_OK;
}

extern JSClass lDOMNodeClass;
extern JSObject * lDOMNodeProto;

JSBool g2_register_event(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	EventRegistration * newReg;
	LPSTR callback = NULL;
	LPTSTR domEvent = NULL;
	BOOL stopPropogation = FALSE;
	JSObject * target = NULL;

	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "o W/ s b", &target, &domEvent, &callback, &stopPropogation))
	{
		JS_ReportError(cx, "Error parsing arguments in registerevent.");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	if(!JS_InstanceOf(cx, target, &lDOMNodeClass, NULL))
	{
		JS_ReportWarning(cx, "Cannot register an event for anything besides a DOM node");
		JS_EndRequest(cx);
		return JS_TRUE;
	}

	PrivateData * mPrivate = (PrivateData*)JS_GetPrivate(cx, obj);
	EnterCriticalSection(&mPrivate->eventHeadLock);
	EventRegistration * curReg = mPrivate->eventHead;
	*rval = JSVAL_NULL;
	while(curReg != NULL)
	{
		if(_wcsicmp(curReg->domEvent, domEvent) == 0)
		{
			*rval = JSVAL_TRUE;
			curReg->stopBubbles = stopPropogation;
			if(callback != NULL)
			{
				if(curReg->callback != NULL)
					HeapFree(GetProcessHeap(), 0, curReg->callback);
				curReg->callback = (LPSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, strlen(callback) + 1);
				strcpy_s(curReg->callback, strlen(callback) + 1, callback);
			}
			else
			{
				if(curReg->callback != NULL)
				{	
					HeapFree(GetProcessHeap(), 0, curReg->callback);
					curReg->callback = NULL;
				}	
			}
		}
		curReg = curReg->next;
	}
	LeaveCriticalSection(&mPrivate->eventHeadLock);
	if(*rval == JSVAL_TRUE)
	{
		JS_EndRequest(cx);
		return JS_TRUE;
	}

	nsIDOMNode * mNode = (nsIDOMNode*)JS_GetPrivate(cx, target);
	nsIDOMEventTarget * eventTarget;
	mNode->QueryInterface(NS_GET_IID(nsIDOMEventTarget), (void**)&eventTarget);
	nsresult rv = eventTarget->AddEventListener(nsString(domEvent), (nsIDOMEventListener*)mPrivate->mDOMListener, PR_FALSE);
	if(NS_SUCCEEDED(rv))
	{
		newReg = new EventRegistration();
		newReg->stopBubbles = stopPropogation;
		newReg->domEvent = (LPTSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (wcslen(domEvent) + 1) * sizeof(TCHAR));
		wcscpy_s(newReg->domEvent, (wcslen(domEvent) + 1), domEvent);
		if(callback != NULL)
		{
			newReg->callback = (LPSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (strlen(callback) + 1));
			strcpy_s(newReg->callback, strlen(callback) + 1, callback);
		}
		newReg->windowsEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

		EnterCriticalSection(&mPrivate->eventHeadLock);
		newReg->next = mPrivate->eventHead;
		mPrivate->eventHead = newReg;
		mPrivate->eventRegCount++;
		LeaveCriticalSection(&mPrivate->eventHeadLock);
		*rval = JSVAL_TRUE;
	}
	else
		*rval = JSVAL_FALSE;
	eventTarget->Release();

	return JS_TRUE;
}

JSBool g2_wait_for_stuff(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	LPWSTR domEvent = NULL;
	DWORD timeOut = INFINITE;
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "W/ u", &domEvent, &timeOut))
	{
		JS_ReportError(cx, "Error parsing arguments in waitforstuff.");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	JS_EndRequest(cx);

	HANDLE eventToWaitFor = NULL;
	PrivateData * mPrivate = (PrivateData*)JS_GetPrivate(cx, obj);
	EnterCriticalSection(&mPrivate->eventHeadLock);
	EventRegistration * curReg = mPrivate->eventHead;
	*rval = JSVAL_NULL;
	while(curReg != NULL)
	{
		if(_wcsicmp(curReg->domEvent, domEvent) == 0)
		{
			if(!DuplicateHandle(GetCurrentProcess(), curReg->windowsEvent, GetCurrentProcess(), &eventToWaitFor, 0, FALSE, DUPLICATE_SAME_ACCESS))
				*rval = JSVAL_FALSE;
			break;
		}
		curReg = curReg->next;
	}
	LeaveCriticalSection(&mPrivate->eventHeadLock);

	if(*rval == JSVAL_FALSE || *rval == JSVAL_NULL)
		return JS_TRUE;

	*rval = WaitForSingleObject(eventToWaitFor, timeOut) == WAIT_OBJECT_0 ? JSVAL_TRUE : JSVAL_FALSE;
	ResetEvent(eventToWaitFor);
	CloseHandle(eventToWaitFor);
	return JS_TRUE;
}

JSBool g2_wait_for_things(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	DWORD timeOut = INFINITE;
	BOOL waitForAll = FALSE;

	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "/b u", &waitForAll, &timeOut))
	{
		JS_ReportError(cx, "Error parsing arguments in waitforstuff.");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	PrivateData * mPrivate = (PrivateData*)JS_GetPrivate(cx, obj);
	EnterCriticalSection(&mPrivate->eventHeadLock);
	DWORD eventIter = 0;
	HANDLE * eventsToWaitFor = (HANDLE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(HANDLE) * mPrivate->eventRegCount);
	EventRegistration * curReg = mPrivate->eventHead;
	while(curReg != NULL)
	{
		DuplicateHandle(GetCurrentProcess(), curReg->windowsEvent, GetCurrentProcess(), &eventsToWaitFor[eventIter++], 0, FALSE, DUPLICATE_SAME_ACCESS);
		curReg = curReg->next;
	}
	LeaveCriticalSection(&mPrivate->eventHeadLock);

	JS_EndRequest(cx);
	DWORD waitResult = WaitForMultipleObjects(eventIter, eventsToWaitFor, waitForAll, timeOut);
	for(DWORD j = 0; j < eventIter; j++)
	{
		ResetEvent(eventsToWaitFor[j]);
		CloseHandle(eventsToWaitFor[j]);
	}

	if(!waitForAll)
	{
		JS_BeginRequest(cx);
		EnterCriticalSection(&mPrivate->eventHeadLock);
		curReg = mPrivate->eventHead;
		DWORD j = WAIT_OBJECT_0;
		while(curReg != NULL && j <= waitResult)
		{
			curReg = curReg->next;
			j++;
		}

		if(curReg == NULL)
			*rval = JSVAL_NULL;
		else
		{
			JSString * retStr = JS_NewUCStringCopyZ(cx, curReg->domEvent);
			*rval = STRING_TO_JSVAL(retStr);
		}
		LeaveCriticalSection(&mPrivate->eventHeadLock);
		JS_EndRequest(cx);
	}
	else
		*rval = JSVAL_TRUE;
	return JS_TRUE;
}

JSBool g2_unregister_event(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	LPTSTR domEvent = NULL;
	JSObject * targetObject;

	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "/o W", &domEvent))
	{
		JS_ReportError(cx, "Error parsing arguments in registerevent.");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	JS_EndRequest(cx);

	PrivateData * mPrivate = (PrivateData*)JS_GetPrivate(cx, obj);
	EnterCriticalSection(&mPrivate->eventHeadLock);
	if(domEvent == NULL)
	{
		EventRegistration * back = mPrivate->eventHead;
		mPrivate->eventHead = NULL;
		mPrivate->eventRegCount = 0;
		LeaveCriticalSection(&mPrivate->eventHeadLock);
		while(back != NULL)
		{
			EventRegistration * nBack = back->next;
			delete back;
			back = nBack;
		}
		return JS_TRUE;
	}

	EventRegistration * curReg = mPrivate->eventHead, *prevReg = NULL;
	while(curReg != NULL && _wcsicmp(curReg->domEvent, domEvent) != 0)
	{
		prevReg = curReg;
		curReg = curReg->next;
	}
	if(curReg == NULL)
		*rval = JSVAL_FALSE;
	else
	{
		prevReg->next = curReg->next;
		delete curReg;
		*rval = JSVAL_TRUE;
	}
	LeaveCriticalSection(&mPrivate->eventHeadLock);
	return JS_TRUE;
}