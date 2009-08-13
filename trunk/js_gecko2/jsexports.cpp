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
#include "js_gecko2.h"
#include "webbrowserchrome.h"
#include "domeventhandler.h"
#include "nsXULAppApi.h"
#include "nsXPComGlue.h"
#include "nsStringAPI.h"
#include "nsComponentManagerUtils.h"
#include "nsNetUtil.h"
#include "nsEmbedCID.h"
#include "nsXPCOMCIDInternal.h"
#include "nsIProxyObjectManager.h"
#include "nsProxiedService.h"

#include "nsIBaseWindow.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMWindow2.h"
#include "nsIDOMNode.h"
#include "nsIDOMElement.h"
#include "nsIDOMDocument.h"
#include "nsIURI.h"
#include "nsIWeakReference.h"
#include "nsIWeakReferenceUtils.h"
#include "nsIWebBrowser.h"
#include "nsIWebBrowserFocus.h"
#include "nsIWebBrowserStream.h"
#include "nsIWebNavigation.h"
#include "nsIWidget.h"
#include "privatedata.h"
#include "nsIDOMParser.h"
#include "nsIDOMDocument.h"

extern BOOL keepUIGoing;
extern CRITICAL_SECTION viewsLock;
extern CRITICAL_SECTION domStateLock;
extern PrivateData * viewsHead;
DWORD nViews = 0;

JSClass GeckoViewClass = {
	"GeckoView",  /* name */
    JSCLASS_HAS_PRIVATE,  /* flags */
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
JSObject * GeckoViewProto;

JSBool g2_create_view(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	PrivateData * newPrivateData = new PrivateData();
	newPrivateData->windowStyle = WS_OVERLAPPEDWINDOW;

	JS_BeginRequest(cx);
	WORD cX = 800, cY = 600;
	nsresult rv;

	JS_ConvertArguments(cx, argc, argv, "c c /b u", &cX, &cY, &newPrivateData->allowClose, &newPrivateData->windowStyle);

	EnterCriticalSection(&viewsLock);
	newPrivateData->next = viewsHead;
	viewsHead = newPrivateData;
	LeaveCriticalSection(&viewsLock);

	WORD x = (GetSystemMetrics(SM_CXSCREEN) - cX) / 2;
	WORD y = (GetSystemMetrics(SM_CYSCREEN) - cY) / 2;
	newPrivateData->requestedRect.top = y;
	newPrivateData->requestedRect.left = x;
	newPrivateData->requestedRect.bottom = y + cY;
	newPrivateData->requestedRect.right = x + cX;
	newPrivateData->mContext = cx;

	BOOL inited = FALSE;
	while(!inited)
	{
		EnterCriticalSection(&viewsLock);
		inited = newPrivateData->initialized;
		LeaveCriticalSection(&viewsLock);
		Sleep(50);
	}
	
	newPrivateData->mDOMListener = NULL;
	newPrivateData->nDOMListeners = 0;
	ShowWindow(newPrivateData->mNativeWindow, SW_SHOW);
	newPrivateData->nsIPO = do_GetService(NS_XPCOMPROXY_CONTRACTID, &rv);
	nsCOMPtr<nsIWebBrowser> browser;
	newPrivateData->nsIPO->GetProxyForObject(NS_PROXY_TO_MAIN_THREAD, NS_GET_IID(nsIWebBrowser), newPrivateData->mBrowser, NS_PROXY_SYNC, getter_AddRefs(browser));
	newPrivateData->mDOMWindow = do_GetInterface(browser);

	JSObject * retObj = JS_NewObject(cx, &GeckoViewClass, GeckoViewProto, obj);
	*rval = OBJECT_TO_JSVAL(retObj);
	JS_SetPrivate(cx, retObj, newPrivateData);
	JS_EndRequest(cx);
	return JS_TRUE;
}

extern JSObject * lDOMNodeProto;
extern JSClass lDOMNodeClass;

JSBool g2_get_element_by_id(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JS_BeginRequest(cx);
	PrivateData * mPrivate = (PrivateData*)JS_GetPrivate(cx, obj);
	LPWSTR idStr;
	if(!JS_ConvertArguments(cx, argc, argv, "W", &idStr))
	{
		JS_ReportError(cx, "Error in argument processing in g2_get_element_by_id");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	nsCOMPtr<nsIDOMDocument> mDocument;
	mPrivate->mDOMWindow->GetDocument(getter_AddRefs(mDocument));
	nsCOMPtr<nsIDOMElement> mElement;
	if(mDocument->GetElementById(nsDependentString(idStr), getter_AddRefs(mElement)) == NS_OK)
	{
		nsIDOMNode * mNode;
		mElement->QueryInterface(NS_GET_IID(nsIDOMNode), (void**)&mNode);
		JSObject * retObj = JS_NewObject(cx, &lDOMNodeClass, lDOMNodeProto, obj);
		*rval = OBJECT_TO_JSVAL(retObj);
		JS_SetPrivate(cx, retObj, mNode);
	}
	else
		*rval = JSVAL_FALSE;
	JS_EndRequest(cx);
	return JS_TRUE;
}

JSBool g2_load_data(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	nsresult rv;
	JS_BeginRequest(cx);
	PrivateData * mPrivate = (PrivateData*)JS_GetPrivate(cx, obj);
	char * dataToLoad, *contentType = "text/html";
	LPWSTR target = NULL, action = NULL, baseUrl;

	if(!JS_ConvertArguments(cx, argc, argv, "s W/ W W s", &dataToLoad, &baseUrl, &target, &action, &contentType))
	{
		JS_ReportError(cx, "Error in argument processing in g2_load_data");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	JS_EndRequest(cx);
	DOMEventListener * curl = mPrivate->mDOMListener;
	while(curl != NULL)
	{
		curl->active = FALSE;
		curl = curl->next;
	}

	nsCOMPtr<nsIWebBrowser> mWebBrowser;
	mPrivate->nsIPO->GetProxyForObject(NS_PROXY_TO_MAIN_THREAD, nsIWebBrowser::GetIID(), mPrivate->mBrowser, NS_PROXY_SYNC, getter_AddRefs(mWebBrowser));
	nsCOMPtr<nsIWebBrowserStream> wbStream = do_QueryInterface(mWebBrowser);
	nsCOMPtr<nsIURI> uri;
	rv = NS_NewURI(getter_AddRefs(uri), nsDependentString(baseUrl));

	wbStream->OpenStream(uri, nsDependentCString(contentType));
	wbStream->AppendToStream((PRUint8*)dataToLoad, strlen(dataToLoad));
	wbStream->CloseStream();
	mPrivate->mDOMWindow = do_GetInterface(mWebBrowser);
	if(target != NULL && !JSVAL_IS_NULL(argv[2]))
	{
		nsCOMPtr<nsIDOMDocument> document;
		nsCOMPtr<nsIDOMElement> mElement;
		if(NS_FAILED(mPrivate->mDOMWindow->GetDocument(getter_AddRefs(document))))
		{
			*rval = JSVAL_FALSE;
			return JS_TRUE;
		}
		if(NS_FAILED(document->GetElementById(nsDependentString(target), getter_AddRefs(mElement))))
		{
			*rval = JSVAL_FALSE;
			return JS_TRUE;
		}
		nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(mElement, &rv);
		if(NS_FAILED(rv))
		{
			*rval = JSVAL_FALSE;
			return JS_TRUE;
		}
		DOMEventListener * tempListener = new DOMEventListener(mPrivate, NULL);
		tempListener->AddRef();
		nsString actionString;
		if(action == NULL)
			actionString.AssignLiteral("click");
		else
			actionString.Assign(action);

		EnterCriticalSection(&domStateLock);
		target->AddEventListener(actionString, tempListener, PR_FALSE);
		LeaveCriticalSection(&domStateLock);
		HANDLE waitHandle = tempListener->GetHandle();
		*rval = WaitForSingleObject(waitHandle, INFINITE) == WAIT_OBJECT_0 ? JSVAL_TRUE : JSVAL_FALSE;
		CloseHandle(waitHandle);
		target->RemoveEventListener(actionString, tempListener, PR_FALSE);
		tempListener->Release();
	}
	return JS_TRUE;
}

JSBool g2_load_uri(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	LPSTR uri;
	LPWSTR target = NULL, eventName = NULL;
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "s /W W", &uri, &target, &eventName))
	{
		JS_ReportError(cx, "Error parsing arguments in g2_load_uri");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	PrivateData * mPrivate = (PrivateData*)JS_GetPrivate(cx, obj);
	JS_EndRequest(cx);
	DOMEventListener * curl = mPrivate->mDOMListener;
	while(curl != NULL)
	{
		curl->active = FALSE;
		curl = curl->next;
	}
	nsCOMPtr<nsIWebBrowser> mBrowser;
	mPrivate->nsIPO->GetProxyForObject(NS_PROXY_TO_MAIN_THREAD, nsIWebBrowser::GetIID(), mPrivate->mBrowser, NS_PROXY_SYNC, getter_AddRefs(mBrowser));
	nsCOMPtr<nsIWebNavigation> mWebNavigation = do_QueryInterface(mBrowser);
	nsresult result = mWebNavigation->LoadURI(NS_ConvertUTF8toUTF16(uri).get(), nsIWebNavigation::LOAD_FLAGS_NONE, NULL, NULL, NULL);
	*rval = result == NS_OK ? JSVAL_TRUE : JSVAL_FALSE;
	if(target != NULL)
	{
		nsresult rv;
		nsCOMPtr<nsIDOMDocument> document;
		nsCOMPtr<nsIDOMElement> mElement;
		if(NS_FAILED(mPrivate->mDOMWindow->GetDocument(getter_AddRefs(document))))
		{
			*rval = JSVAL_FALSE;
			return JS_TRUE;
		}
		if(NS_FAILED(document->GetElementById(nsDependentString(target), getter_AddRefs(mElement))))
		{
			*rval = JSVAL_FALSE;
			return JS_TRUE;
		}
		nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(mElement, &rv);
		if(NS_FAILED(rv))
		{
			*rval = JSVAL_FALSE;
			return JS_TRUE;
		}
		DOMEventListener * tempListener = new DOMEventListener(mPrivate, NULL);
		tempListener->AddRef();
		nsString actionString;
		if(eventName == NULL)
			actionString.AssignLiteral("click");
		else
			actionString.Assign(eventName);

		target->AddEventListener(actionString, tempListener, PR_FALSE);
		HANDLE waitHandle = tempListener->GetHandle();
		*rval = WaitForSingleObject(waitHandle, INFINITE) == WAIT_OBJECT_0 ? JSVAL_TRUE : JSVAL_FALSE;
		CloseHandle(waitHandle);
		target->RemoveEventListener(actionString, tempListener, PR_FALSE);
		tempListener->Release();
	}
	return JS_TRUE;
}

JSBool g2_import_dom_string(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	PrivateData * mPrivate = (PrivateData*)JS_GetPrivate(cx, obj);
	LPWSTR dataToLoad;
	char *contentType = "text/html";

	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "W/ s", &dataToLoad, &contentType))
	{
		JS_ReportError(cx, "Error in argument processing in g2_load_data");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	JS_YieldRequest(cx);
	nsresult rv;
	NS_WITH_PROXIED_SERVICE(nsIDOMParser, parser, NS_DOMPARSER_CONTRACTID, NS_PROXY_TO_MAIN_THREAD, &rv);
	nsCOMPtr<nsIDOMDocument> newDocument, document;
	mPrivate->mDOMWindow->GetDocument(getter_AddRefs(document));
	if(NS_SUCCEEDED(parser->ParseFromString(dataToLoad, contentType, getter_AddRefs(newDocument))))
	{
		nsCOMPtr<nsIDOMNode> domNodeIn = do_QueryInterface(newDocument);
		nsIDOMNode * domNodeOut = NULL;
		document->ImportNode(domNodeIn, PR_TRUE, &domNodeOut);
		JSObject * retObj = JS_NewObject(cx, &lDOMNodeClass, lDOMNodeProto, obj);
		*rval = OBJECT_TO_JSVAL(retObj);
		JS_SetPrivate(cx, retObj, domNodeOut);
	}
	else
		*rval = JSVAL_FALSE;
	JS_EndRequest(cx);
	return JS_TRUE;
}

JSBool g2_destroy(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	PrivateData * mPrivate = (PrivateData*)JS_GetPrivate(cx, obj);
	EnterCriticalSection(&viewsLock);
	mPrivate->destroying = TRUE;
	LeaveCriticalSection(&viewsLock);
	return JS_TRUE;
}

extern JSClass lDOMDocClass;
extern JSObject * lDOMDocProto;

JSBool g2_get_dom(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JS_BeginRequest(cx);
	PrivateData * mPrivate = (PrivateData*)JS_GetPrivate(cx, obj);

	nsCOMPtr<nsIDOMDocument> mDocument;
	mPrivate->mDOMWindow->GetDocument(getter_AddRefs(mDocument));
	nsCOMPtr<nsIDOMNode> mNode = do_QueryInterface(mDocument);

	JSObject * retObj = JS_NewObject(cx, &lDOMDocClass, lDOMDocProto, obj);
	*rval = OBJECT_TO_JSVAL(retObj);
	JS_SetPrivate(cx, retObj, mNode);
	JS_EndRequest(cx);
	return JS_TRUE;
}

JSBool g2_repaint(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JS_BeginRequest(cx);
	PrivateData * mPrivate = (PrivateData*)JS_GetPrivate(cx, obj);
	JS_EndRequest(cx);
	nsCOMPtr<nsIWebBrowser> mBrowser;
	mPrivate->nsIPO->GetProxyForObject(NS_PROXY_TO_MAIN_THREAD, nsIWebBrowser::GetIID(), mPrivate->mBrowser, NS_PROXY_SYNC, getter_AddRefs(mBrowser));
	nsCOMPtr<nsIBaseWindow> mBaseWindow = do_QueryInterface(mBrowser);
	EnterCriticalSection(&domStateLock);
	mBaseWindow->Repaint(PR_TRUE);
	LeaveCriticalSection(&domStateLock);
	return JS_TRUE;
}

#ifdef __cplusplus
extern "C" {
#endif
BOOL __declspec(dllexport) InitExports(JSContext * cx, JSObject * global)
{
	JSFunctionSpec geckoViewFuncs[] = {
		{ "LoadData", g2_load_data, 5, 0 },
		{ "LoadURI", g2_load_uri, 1, 0 },
		{ "Destroy", g2_destroy, 0, 0 },
		{ "GetDOM", g2_get_dom, 0, 0 },
		{ "RegisterEvent", g2_register_event, 3, 0 },
		{ "UnregisterEvents", g2_unregister_event, 1, 0 },
		{ "WaitForStuff", g2_wait_for_stuff, 2, 0 },
		{ "WaitForThings", g2_wait_for_things, 2, 0 },
		{ "GetInput", g2_get_input_value, 1, 0 },
		{ "GetElementByID", g2_get_element_by_id, 1, 0 },
		{ "Repaint", g2_repaint, 0, 0 },
		{ "ImportDOM", g2_import_dom_string, 1, 0 },
		{ 0 }
	};

	JSFunctionSpec geckoFuncs[] = {
		{ "GeckoCreateView", g2_create_view, 4, 0 },
		{ "GeckoInit", g2_init, 2, 0 },
		{ "GeckoTerm", g2_term, 0, 0 },
		{ 0 }
	};

	JS_BeginRequest(cx);
	GeckoViewProto = JS_InitClass(cx, global, NULL, &GeckoViewClass, NULL, 0, NULL, geckoViewFuncs, NULL, NULL);
	JS_DefineFunctions(cx, global, geckoFuncs);
	initDOMNode(cx, global);
	JS_EndRequest(cx);

	return TRUE;
}

BOOL __declspec(dllexport) CleanupExports(JSContext * cx, JSObject * global)
{
	return TRUE;
}
#ifdef __cplusplus
}
#endif