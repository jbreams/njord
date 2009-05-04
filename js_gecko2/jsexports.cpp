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
#include "nsIURI.h"
#include "nsIWeakReference.h"
#include "nsIWeakReferenceUtils.h"
#include "nsIWebBrowser.h"
#include "nsIWebBrowserFocus.h"
#include "nsIWebBrowserStream.h"
#include "nsIWebNavigation.h"
#include "nsIWidget.h"
#include "privatedata.h"

#include "nsIDOMDocument.h"

extern BOOL keepUIGoing;
extern CRITICAL_SECTION viewsLock;
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

	JS_BeginRequest(cx);
	WORD cX, cY;
	nsresult rv;

	if(!JS_ConvertArguments(cx, argc, argv, "c c", &cX, &cY))
	{
		JS_ReportWarning(cx, "No dimensions specified for creating the view. Defaulting.");
		cX = 800;
		cY = 600;
	}

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

	BOOL inited = FALSE;
	while(!inited)
	{
		EnterCriticalSection(&viewsLock);
		inited = newPrivateData->initialized;
		LeaveCriticalSection(&viewsLock);
		Sleep(50);
	}
	
	ShowWindow(newPrivateData->mNativeWindow, SW_SHOW);
	newPrivateData->nsIPO = do_GetService(NS_XPCOMPROXY_CONTRACTID, &rv);
	nsCOMPtr<nsIWebBrowser> browser;
	newPrivateData->nsIPO->GetProxyForObject(NS_PROXY_TO_MAIN_THREAD, NS_GET_IID(nsIWebBrowser), newPrivateData->mBrowser, NS_PROXY_SYNC, getter_AddRefs(browser));
	newPrivateData->mDOMWindow = do_GetInterface(browser);

	JSObject * retObj = JS_NewObject(cx, &GeckoViewClass, GeckoViewProto, obj);
	*rval = OBJECT_TO_JSVAL(retObj);
	JS_SetPrivate(cx, retObj, newPrivateData);
	return JS_TRUE;
}

JSBool g2_load_data(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	nsresult rv;
	PrivateData * mPrivate = (PrivateData*)JS_GetPrivate(cx, obj);
	LPWSTR baseUrl;
	char * dataToLoad, *contentType = "text/html";

	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "s W/ s", &dataToLoad, &baseUrl, &contentType))
	{
		JS_ReportError(cx, "Error in argument processing in g2_load_data");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	JS_EndRequest(cx);
	nsCOMPtr<nsIWebBrowser> mWebBrowser;
	mPrivate->nsIPO->GetProxyForObject(NS_PROXY_TO_MAIN_THREAD, nsIWebBrowser::GetIID(), mPrivate->mBrowser, NS_PROXY_SYNC, getter_AddRefs(mWebBrowser));

	nsCOMPtr<nsIWebBrowserStream> wbStream = do_QueryInterface(mWebBrowser);
	nsCOMPtr<nsIURI> uri;
	rv = NS_NewURI(getter_AddRefs(uri), nsString(baseUrl));
	wbStream->OpenStream(uri, nsDependentCString(contentType));
	wbStream->AppendToStream((PRUint8*)dataToLoad, strlen(dataToLoad));
	wbStream->CloseStream();

	return JS_TRUE;
}

JSBool g2_load_uri(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	LPSTR uri;
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "s", &uri))
	{
		JS_ReportError(cx, "Error parsing arguments in g2_load_uri");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	JS_EndRequest(cx);

	nsresult rv;
	PrivateData * mPrivate = (PrivateData*)JS_GetPrivate(cx, obj);
	nsCOMPtr<nsIWebBrowser> mBrowser;
	mPrivate->nsIPO->GetProxyForObject(NS_PROXY_TO_MAIN_THREAD, nsIWebBrowser::GetIID(), mPrivate->mBrowser, NS_PROXY_SYNC, getter_AddRefs(mBrowser));

	nsCOMPtr<nsIWebNavigation> mWebNavigation = do_QueryInterface(mBrowser);
	nsresult result = mWebNavigation->LoadURI(NS_ConvertUTF8toUTF16(uri).get(), nsIWebNavigation::LOAD_FLAGS_NONE, NULL, NULL, NULL);
	*rval = result == NS_OK ? JSVAL_TRUE : JSVAL_FALSE;
	return JS_TRUE;
}

JSBool g2_destroy(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	PrivateData * mPrivate = (PrivateData*)JS_GetPrivate(cx, obj);
	mPrivate->mChrome->DestroyBrowserWindow();
	nViews--;
	if(nViews == 0)
		keepUIGoing = FALSE;
	return JS_TRUE;
}

extern JSClass lDOMDocClass;
extern JSObject * lDOMDocProto;

JSBool g2_get_dom(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	PrivateData * mPrivate = (PrivateData*)JS_GetPrivate(cx, obj);

	nsIDOMDocument * mDocument;
	mPrivate->mDOMWindow->GetDocument(&mDocument);
	nsIDOMNode * mNode;
	mDocument->QueryInterface(NS_GET_IID(nsIDOMNode), (void**)&mNode);
	mDocument->Release();

	JS_BeginRequest(cx);
	JSObject * retObj = JS_NewObject(cx, &lDOMDocClass, lDOMDocProto, obj);
	*rval = OBJECT_TO_JSVAL(retObj);
	JS_SetPrivate(cx, retObj, mNode);
	JS_EndRequest(cx);
	return JS_TRUE;
}

#ifdef __cplusplus
extern "C" {
#endif
BOOL __declspec(dllexport) InitExports(JSContext * cx, JSObject * global)
{
	JSFunctionSpec geckoViewFuncs[] = {
		{ "LoadData", g2_load_data, 3, 0 },
		{ "LoadURI", g2_load_uri, 1, 0 },
		{ "Destroy", g2_destroy, 0, 0 },
		{ "GetDOM", g2_get_dom, 0, 0 },
		{ "RegisterEvent", g2_register_event, 3, 0 },
		{ "UnregisterEvent", g2_unregister_event, 1, 0 },
		{ "WaitForStuff", g2_wait_for_stuff, 2, 0 },
		{ "WaitForThings", g2_wait_for_things, 2, 0 },
		{ "GetInput", g2_get_input_value, 1, 0 },
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