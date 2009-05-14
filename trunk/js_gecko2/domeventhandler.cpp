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
#include "nsIDOM3Node.h"

#include "nsXPCOMCIDInternal.h"
#include "nsIProxyObjectManager.h"


EventRegistration::EventRegistration()
{
	callback = NULL;
	target = NULL;
	windowsEvent = INVALID_HANDLE_VALUE;
	next = NULL;
}

EventRegistration::~EventRegistration()
{
	if(callback)
		free(callback);
	if(windowsEvent != INVALID_HANDLE_VALUE)
		CloseHandle(windowsEvent);
	if(target != NULL)
		target->Release();
}

NS_IMPL_ISUPPORTS1(DOMEventListener,
				   nsIDOMEventListener)

DOMEventListener::DOMEventListener(PrivateData *aOwner)
{
	mOwner = aOwner;
	head = NULL;
	regCount = 0;
	InitializeCriticalSection(&headLock);
	addRemoveMutex = CreateMutex(NULL, FALSE, NULL);
}

DOMEventListener::~DOMEventListener()
{
	UnregisterAll();
	DeleteCriticalSection(&headLock);
}

NS_IMETHODIMP DOMEventListener::HandleEvent(nsIDOMEvent *aEvent)
{
	EnterCriticalSection(&headLock);
	if(head == NULL)
	{
		LeaveCriticalSection(&headLock);
		return NS_OK;
	}
	
	nsString eventType;
	aEvent->GetType(eventType);
	nsCOMPtr<nsIDOMEventTarget> eventTarget;
	aEvent->GetTarget(getter_AddRefs(eventTarget));
	nsCOMPtr<nsIDOM3Node> targetNode = do_QueryInterface(eventTarget);

	EventRegistration * curReg = head;
	while(curReg != NULL)
	{
		PRBool isSame = PR_FALSE;
		targetNode->IsSameNode(curReg->target, &isSame);
		if(eventType.Compare(curReg->domEvent) == 0 && isSame)
		{
			SetEvent(curReg->windowsEvent);
			if(curReg->callback)
			{
				JS_BeginRequest(mOwner->mContext);
				JS_CallFunctionName(mOwner->mContext, JS_GetGlobalObject(mOwner->mContext), curReg->callback, 0, NULL, NULL);
				JS_EndRequest(mOwner->mContext);
			}
			break;
		}
		curReg = curReg->next;
	}
	LeaveCriticalSection(&headLock);
	return NS_OK;
}

BOOL DOMEventListener::RegisterEvent(nsIDOMNode *target, LPWSTR type, LPSTR callback)
{
	WaitForSingleObject(addRemoveMutex, INFINITE);
	nsDependentString typeString(type);
	nsIDOMEventTarget * realTarget;
	target->QueryInterface(NS_GET_IID(nsIDOMEventTarget), (void**)&realTarget);
	nsresult rv = realTarget->AddEventListener(typeString, (nsIDOMEventListener*)this, PR_FALSE);
	realTarget->Release();

	if(NS_SUCCEEDED(rv))
	{
		EventRegistration * nReg = new EventRegistration();
		nReg->domEvent = typeString;
		if(callback != NULL)
			nReg->callback = _strdup(callback);
		nReg->windowsEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		nReg->target = target;
		target->AddRef();

		EnterCriticalSection(&headLock);
		nReg->next = head;
		head = nReg;
		regCount++;
		LeaveCriticalSection(&headLock);
		ReleaseMutex(addRemoveMutex);
		return TRUE;
	}
	ReleaseMutex(addRemoveMutex);
	return FALSE;
}

void DOMEventListener::UnregisterEvent(nsIDOMNode * targetNode, LPWSTR type)
{
	nsIDOM3Node * realTarget;
	targetNode->QueryInterface(NS_GET_IID(nsIDOM3Node), (void**)&realTarget);

	EventRegistration * curEr, *lastEr = NULL;
	EnterCriticalSection(&headLock);
	curEr = head;
	WaitForSingleObject(addRemoveMutex, INFINITE);
	while(curEr != NULL)
	{
		PRBool nodesAreSame = PR_FALSE;
		realTarget->IsSameNode(curEr->target, &nodesAreSame);
		if(!(curEr->domEvent.Compare(type) == 0 && nodesAreSame))
		{
			lastEr = curEr;
			curEr = curEr->next;
			continue;
		}
		if(lastEr != NULL)
			lastEr->next = curEr->next;
		else
			head = curEr->next;
		break;
	}
	if(curEr != NULL)
		regCount--;
	LeaveCriticalSection(&headLock);
	
	if(curEr != NULL)
	{
		nsCOMPtr<nsIDOMEventTarget> eventTarget = do_QueryInterface(curEr->target);
		eventTarget->RemoveEventListener(curEr->domEvent, (nsIDOMEventListener*)this, PR_FALSE);
		delete curEr;
	}
	ReleaseMutex(addRemoveMutex);
	realTarget->Release();
}

void DOMEventListener::UnregisterAll()
{
	EventRegistration * curEr;
	EnterCriticalSection(&headLock);
	curEr = head;
	WaitForSingleObject(addRemoveMutex, INFINITE);
	while(curEr != NULL)
	{
		nsCOMPtr<nsIDOMEventTarget>  eventTarget = do_QueryInterface(curEr->target);
		eventTarget->RemoveEventListener(curEr->domEvent, (nsIDOMEventListener*)this, PR_FALSE);
		EventRegistration * nextEr = curEr->next;
		delete curEr;
		curEr = nextEr;
	}
	head = NULL;
	regCount = 0;
	ReleaseMutex(addRemoveMutex);
	LeaveCriticalSection(&headLock);
}

BOOL DOMEventListener::WaitForSingleEvent(nsIDOMNode *target, LPWSTR type, DWORD timeout)
{
	EventRegistration * curEr;
	nsIDOM3Node * realTarget;
	target->QueryInterface(NS_GET_IID(nsIDOM3Node), (void**)&realTarget);
	HANDLE waitForMe = NULL;

	EnterCriticalSection(&headLock);
	curEr = head;
	while(curEr != NULL)
	{
		PRBool isEqual = PR_FALSE;
		if(curEr->domEvent.Compare(type) == 0)
			realTarget->IsSameNode(curEr->target, &isEqual);
		if(isEqual)
			break;
		curEr = curEr->next;
	}
	if(curEr != NULL)
		DuplicateHandle(GetCurrentProcess(), curEr->windowsEvent, GetCurrentProcess(), &waitForMe, SYNCHRONIZE, FALSE, 0);
	LeaveCriticalSection(&headLock);
	realTarget->Release();

	if(waitForMe == NULL)
		return FALSE;

	BOOL retval = FALSE;
	if(WaitForSingleObject(waitForMe, timeout) == WAIT_OBJECT_0)
		retval = TRUE;
	ResetEvent(waitForMe);
	CloseHandle(waitForMe);
	return retval;
}

BOOL DOMEventListener::WaitForAllEvents(DWORD timeout, BOOL waitAll, JSObject ** out)
{
	EnterCriticalSection(&headLock);
	if(regCount == 0)
		return FALSE;
	HANDLE * handles = new HANDLE[regCount];
	DWORD i = 0;
	EventRegistration * curReg = head;
	while(curReg != NULL)
	{
		if(DuplicateHandle(GetCurrentProcess(), curReg->windowsEvent, GetCurrentProcess(), &handles[i], SYNCHRONIZE, FALSE, 0))
			i++;
		curReg = curReg->next;
	}
	WaitForSingleObject(addRemoveMutex, INFINITE);
	LeaveCriticalSection(&headLock);

	if(i == 0)
	{
		delete [] handles;
		return 0;
	}

	DWORD result = WaitForMultipleObjects(i, handles, waitAll, timeout);
	for(DWORD c= 0; c < i; c++)
	{
		ResetEvent(handles[c]);
		CloseHandle(handles[c]);
	}
	delete [] handles;
	if(result == WAIT_TIMEOUT)
		return FALSE;
	return TRUE;

/*	if(waitAll == TRUE || out == NULL)
		return TRUE;

	EnterCriticalSection(&headLock);
	curReg = head;
	for(i = WAIT_OBJECT_0; (i < result) && curReg != NULL; i++, curReg = curReg->next) ;
	if(curReg == NULL)
		return TRUE;*/
}

extern JSClass lDOMNodeClass;
extern JSObject * lDOMNodeProto;

JSBool g2_register_event(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	LPSTR callback = NULL;
	LPTSTR domEvent = NULL;
	JSObject * target = NULL;

	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "o W/ s", &target, &domEvent, &callback))
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
	nsIDOMNode * mNode = (nsIDOMNode*)JS_GetPrivate(cx, target);
	if(mPrivate->mDOMListener->RegisterEvent(mNode, domEvent, callback))
		*rval = JSVAL_TRUE;
	else
		*rval = JSVAL_FALSE;
	return JS_TRUE;
}

JSBool g2_wait_for_stuff(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	LPWSTR domEvent = NULL;
	DWORD timeOut = INFINITE;
	JSObject * nodeObj = NULL;
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "o W/ u", &nodeObj, &domEvent, &timeOut))
	{
		JS_ReportError(cx, "Error parsing arguments in waitforstuff.");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	JS_EndRequest(cx);

	PrivateData * mPrivate = (PrivateData*)JS_GetPrivate(cx, obj);
	nsIDOMNode * node = (nsIDOMNode*)JS_GetPrivate(cx, nodeObj);
	*rval = mPrivate->mDOMListener->WaitForSingleEvent(node, domEvent, timeOut) ? JSVAL_TRUE : JSVAL_FALSE;
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
	JS_EndRequest(cx);

	PrivateData * mPrivate = (PrivateData*)JS_GetPrivate(cx, obj);
	*rval = mPrivate->mDOMListener->WaitForAllEvents(timeOut, waitForAll, NULL) ? JSVAL_TRUE : JSVAL_FALSE;
	return JS_TRUE;
}

JSBool g2_unregister_event(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	LPTSTR domEvent = NULL;
	JSObject * nodeObj = NULL;

	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "/o W", &nodeObj, &domEvent))
	{
		JS_ReportError(cx, "Error parsing arguments in registerevent.");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	JS_EndRequest(cx);

	PrivateData * mPrivate = (PrivateData*)JS_GetPrivate(cx, obj);
	if(nodeObj && domEvent == NULL)
		mPrivate->mDOMListener->UnregisterAll();
	else
	{
		nsIDOMNode * node = (nsIDOMNode*)JS_GetPrivate(cx, nodeObj);
		mPrivate->mDOMListener->UnregisterEvent(node, domEvent);

	}

	return JS_TRUE;
}