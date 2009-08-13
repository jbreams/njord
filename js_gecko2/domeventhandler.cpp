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

extern JSClass lDOMNodeClass;
extern JSObject * lDOMNodeProto;
extern CRITICAL_SECTION domStateLock;

DOMEventListener::DOMEventListener(PrivateData * owner, LPSTR callback)
{
	this->callBack = NULL;
	if(callback != NULL)
		this->callBack = _strdup(callback);
	this->myHandle = CreateEvent(NULL, TRUE, FALSE, NULL);
	this->aOwner = owner;
	this->active = TRUE;
	this->next = NULL;
}

DOMEventListener::~DOMEventListener()
{
	if(callBack != NULL)
		free(callBack);
	CloseHandle(myHandle);
}

NS_IMPL_ISUPPORTS1(DOMEventListener,
                   nsIDOMEventListener)


HANDLE DOMEventListener::GetHandle()
{
	HANDLE retHandle = INVALID_HANDLE_VALUE;
	DuplicateHandle(GetCurrentProcess(), myHandle, GetCurrentProcess(), &retHandle, SYNCHRONIZE | EVENT_MODIFY_STATE, FALSE, 0);
	return retHandle;
}

NS_IMETHODIMP DOMEventListener::HandleEvent(nsIDOMEvent *aEvent)
{
	if(!active)
		return NS_OK;
	SetEvent(myHandle);
	if(callBack != NULL)
	{
		JS_BeginRequest(aOwner->mContext);
		JS_CallFunctionName(aOwner->mContext, JS_GetGlobalObject(aOwner->mContext), callBack, 0, NULL, NULL);
		JS_EndRequest(aOwner->mContext);
	}
	return NS_OK;
}

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
	DOMEventListener * newListener = new DOMEventListener(mPrivate, callback);
	newListener->AddRef();
	
	nsCOMPtr<nsIDOMEventTarget> realTarget = do_QueryInterface(mNode);
	nsDependentString typeString(domEvent);
	EnterCriticalSection(&domStateLock);
	nsresult rv = realTarget->AddEventListener(typeString, newListener, PR_FALSE);
	LeaveCriticalSection(&domStateLock);
	if(NS_SUCCEEDED(rv))
	{
		newListener->next = mPrivate->mDOMListener;
		mPrivate->mDOMListener = newListener;
		mPrivate->nDOMListeners++;
		*rval = JSVAL_TRUE;
	}
	else
	{
		newListener->Release();
		*rval = JSVAL_FALSE;
	}
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
	DOMEventListener * newListener = new DOMEventListener(mPrivate, NULL);
	newListener->AddRef();
	
	nsCOMPtr<nsIDOMEventTarget> realTarget = do_QueryInterface(node);
	nsDependentString typeString(domEvent);
	EnterCriticalSection(&domStateLock);
	if(NS_FAILED(realTarget->AddEventListener(typeString, newListener, PR_FALSE)))
	{
		LeaveCriticalSection(&domStateLock);
		*rval = JSVAL_FALSE;
		return JS_TRUE;
	}
	LeaveCriticalSection(&domStateLock);
	
	HANDLE waitHandle = newListener->GetHandle();
	DWORD waitResult = WaitForSingleObject(waitHandle, timeOut);
	EnterCriticalSection(&domStateLock);
	realTarget->RemoveEventListener(typeString, newListener, PR_FALSE);
	LeaveCriticalSection(&domStateLock);
	CloseHandle(waitHandle);
	newListener->Release();
	*rval = JSVAL_FALSE;
	if(waitResult == WAIT_OBJECT_0)
		*rval = JSVAL_TRUE;
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
	HANDLE * thingsToWaitFor = (HANDLE*)malloc(sizeof(HANDLE) * mPrivate->nDOMListeners);
	DOMEventListener * curListener = mPrivate->mDOMListener;
	for(DWORD i = 0; i < mPrivate->nDOMListeners && curListener != NULL; i++, curListener = curListener->next)
		thingsToWaitFor[i] = curListener->GetHandle();
	DWORD result = WaitForMultipleObjects(mPrivate->nDOMListeners, thingsToWaitFor, waitForAll, timeOut);
	for(DWORD i = 0; i < mPrivate->nDOMListeners; i++)
	{
		ResetEvent(thingsToWaitFor[i]);
		CloseHandle(thingsToWaitFor[i]);
	}
	free(thingsToWaitFor);

	*rval = JSVAL_TRUE;
	if((result >= WAIT_ABANDONED_0 && result < WAIT_ABANDONED_0 + mPrivate->nDOMListeners) || result == WAIT_TIMEOUT || result == WAIT_FAILED)
		*rval = JSVAL_FALSE;
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
	
	DOMEventListener * curl = mPrivate->mDOMListener;
	mPrivate->mDOMListener = NULL;
	while(curl != NULL)
	{
		DOMEventListener * next = curl->next;
		curl->Release();
		curl = next;
	}
	return JS_TRUE;
}
