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
#include "nsXULAppApi.h"
#include "nsXPComGlue.h"
#include "nsStringAPI.h"
#include "nsCOMPtr.h"
#include "nsComponentManagerUtils.h"
#include "nsNetUtil.h"
#include "nsEmbedCID.h"
#include "nsIProxyObjectManager.h"
#include "nsProxiedService.h"

#include "nsIBaseWindow.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDOMWindow2.h"
#include "nsIURI.h"
#include "nsIWeakReference.h"
#include "nsIWeakReferenceUtils.h"
#include "nsIWebBrowser.h"
#include "nsIWebBrowserFocus.h"
#include "nsIWebBrowserStream.h"
#include "nsIWebBrowserChrome.h"
#include "nsIWebNavigation.h"
#include "nsIWidget.h"
#include "privatedata.h"
#include "nsIWindowCreator2.h"
#include "nsIWindowWatcher.h"
#include "js_gecko2.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsProfileDirServiceProvider.h"

BOOL keepUIGoing = FALSE;
CRITICAL_SECTION viewsLock;
PrivateData * viewsHead = NULL;
LONG ThreadInitialized = 0;

XRE_InitEmbeddingType XRE_InitEmbedding = 0;
XRE_TermEmbeddingType XRE_TermEmbedding = 0;
XRE_NotifyProfileType XRE_NotifyProfile = 0;
XRE_LockProfileDirectoryType XRE_LockProfileDirectory = 0;

nsIDirectoryServiceProvider *sAppFileLocProvider = 0;
nsCOMPtr<nsILocalFile> sProfileDir = 0;
nsISupports * sProfileLock = 0;

class MozEmbedDirectoryProvider : public nsIDirectoryServiceProvider2
{
public:
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIDIRECTORYSERVICEPROVIDER
    NS_DECL_NSIDIRECTORYSERVICEPROVIDER2
};

static const MozEmbedDirectoryProvider kDirectoryProvider;

NS_IMPL_QUERY_INTERFACE2(MozEmbedDirectoryProvider,
                         nsIDirectoryServiceProvider,
                         nsIDirectoryServiceProvider2)

NS_IMETHODIMP_(nsrefcnt)
MozEmbedDirectoryProvider::AddRef()
{
    return 1;
}

NS_IMETHODIMP_(nsrefcnt)
MozEmbedDirectoryProvider::Release()
{
    return 1;
}

NS_IMETHODIMP
MozEmbedDirectoryProvider::GetFile(const char *aKey, PRBool *aPersist,
                                   nsIFile* *aResult)
{
    if (sAppFileLocProvider) {
        nsresult rv = sAppFileLocProvider->GetFile(aKey, aPersist, aResult);
        if (NS_SUCCEEDED(rv))
            return rv;
    }

    if (sProfileDir && !strcmp(aKey, NS_APP_USER_PROFILE_50_DIR)) {
        *aPersist = PR_TRUE;
        return sProfileDir->Clone(aResult);
    }

    if (sProfileDir && !strcmp(aKey, NS_APP_PROFILE_DIR_STARTUP)) {
        *aPersist = PR_TRUE;
        return sProfileDir->Clone(aResult);
    }

    if (sProfileDir && !strcmp(aKey, NS_APP_CACHE_PARENT_DIR)) {
        *aPersist = PR_TRUE;
        return sProfileDir->Clone(aResult);
    }

    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
MozEmbedDirectoryProvider::GetFiles(const char *aKey,
                                    nsISimpleEnumerator* *aResult)
{
    nsCOMPtr<nsIDirectoryServiceProvider2>
        dp2(do_QueryInterface(sAppFileLocProvider));

    if (!dp2)
        return NS_ERROR_FAILURE;

    return dp2->GetFiles(aKey, aResult);
}

int InitGRE(LPVOID lpParam)
{
	nsresult rv;
	LPSTR pathToGRE = (LPSTR)lpParam;
	if(lpParam == NULL)
	{
		GREVersionRange vr = {
			"1.9a1", PR_TRUE,
			"2.0", PR_FALSE };
		
		pathToGRE = (LPSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MAX_PATH);
		rv = GRE_GetGREPathWithProperties(&vr, 1, nsnull, 0, pathToGRE, MAX_PATH);
		if(NS_FAILED(rv))
		{
			MessageBox(NULL, TEXT("Could not find GRE_HOME."), TEXT("Gecko"), MB_OK);
			HeapFree(GetProcessHeap(), 0, pathToGRE);
			return 1;
		}
	}
	rv = XPCOMGlueStartup(pathToGRE);
	nsCString nsPathToGRE(pathToGRE);
	if(pathToGRE != lpParam)
		HeapFree(GetProcessHeap(), 0, pathToGRE);
	if(NS_FAILED(rv))
	{
		MessageBox(NULL, TEXT("Error in XPCOMGlueStartup. Cannot continue."), TEXT("Gecko"), MB_OK);
		return 2;
	}

    NS_LogInit();

    nsDynamicFunctionLoad nsFuncs[] = {
            {"XRE_InitEmbedding", (NSFuncPtr*)&XRE_InitEmbedding},
            {"XRE_TermEmbedding", (NSFuncPtr*)&XRE_TermEmbedding},
            {"XRE_NotifyProfile", (NSFuncPtr*)&XRE_NotifyProfile},
            {"XRE_LockProfileDirectory", (NSFuncPtr*)&XRE_LockProfileDirectory},
            {0, 0}
    };

	if(NS_FAILED(XPCOMGlueLoadXULFunctions(nsFuncs)))
	{
		MessageBox(NULL, TEXT("Error loading XUL functions. Cannot continue."), TEXT("Gecko"), MB_OK);
		return 3;
	}

	nsPathToGRE.SetLength(nsPathToGRE.RFindChar('\\'));
	nsCOMPtr<nsILocalFile> xulDir;
	if(NS_FAILED(NS_NewNativeLocalFile(nsPathToGRE, PR_FALSE, getter_AddRefs(xulDir))))
	{
		MessageBox(NULL, TEXT("Error creating XUL directory handle. Cannot continue."), TEXT("Gecko"), MB_OK);
		return 4;
	}

	LPSTR self = (LPSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MAX_PATH);
	GetModuleFileNameA(GetModuleHandle(NULL), self, MAX_PATH);
	nsCString nsPathToSelf(self);
	HeapFree(GetProcessHeap(), 0, self);
	nsPathToSelf.SetLength(nsPathToSelf.RFindChar('\\'));
	nsCOMPtr<nsILocalFile> appDir;
	if(NS_FAILED(NS_NewNativeLocalFile(nsPathToSelf, PR_FALSE, getter_AddRefs(appDir))))
	{
		MessageBox(NULL, TEXT("Error creating application directory handle. Cannot continue."), TEXT("Gecko"), MB_OK);
		return 5;
	}

	nsCOMPtr<nsIFile> profFile;
	rv = appDir->Clone(getter_AddRefs(profFile));
	sProfileDir = do_QueryInterface(profFile);
	sProfileDir->AppendNative(NS_LITERAL_CSTRING("njordgecko"));

	PRBool dirExists;
	sProfileDir->Exists(&dirExists);
	if(!dirExists)
		sProfileDir->Create(nsIFile::DIRECTORY_TYPE, 0700);

	if(sProfileDir && ! sProfileLock)
	{
		if(NS_FAILED(XRE_LockProfileDirectory(sProfileDir, &sProfileLock)))
		{
			MessageBox(NULL, TEXT("Error locking profile. Cannot continue."), TEXT("Gecko"), MB_OK);
			return 6;
		}
	}

	if(NS_FAILED(XRE_InitEmbedding(xulDir, appDir, const_cast<MozEmbedDirectoryProvider*>(&kDirectoryProvider), 0, NULL)))
	{
		MessageBox(NULL, TEXT("Error starting embedding. Cannot continue."), TEXT("Gecko"), MB_OK);
		return 7;
	}
	XRE_NotifyProfile();
	NS_LogTerm();
	return 0;
}

LRESULT CALLBACK MozViewProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	WebBrowserChrome * mChrome = (WebBrowserChrome*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	nsCOMPtr<nsIWebBrowser> mBrowser;
	if(mChrome)
		mChrome->GetWebBrowser(getter_AddRefs(mBrowser));
	switch(message)
	{
	case WM_SIZE:
		if(mChrome)
		{
			RECT rect;
			GetClientRect(hWnd, &rect);
			nsCOMPtr<nsIBaseWindow> baseWindow = do_QueryInterface(mBrowser);
			baseWindow->SetPositionAndSize(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, PR_TRUE);
		}

    case WM_ACTIVATE:
		if (mChrome) {
			nsCOMPtr<nsIWebBrowserFocus> mFocus = do_QueryInterface(mBrowser);
			switch (wParam) {
			case WA_CLICKACTIVE:
			case WA_ACTIVE:
				mFocus->Activate();
				break;
			case WA_INACTIVE:
				mFocus->Deactivate();
				break;
			default:
				break;
			}
		}
		break;
	case WM_SHOWWINDOW:
		if(mChrome)
		{
			nsCOMPtr<nsIWebBrowserFocus> mFocus = do_QueryInterface(mBrowser);
			if((BOOL)wParam)
				mFocus->Activate();
			else
				mFocus->Deactivate();
		}
		break;
	case WM_CLOSE:
		if(mChrome) {
			if(mChrome->mAllowClose)
				DestroyWindow(hWnd);
		}
		break;
	case WM_DESTROY:
		if(mChrome) {
			nsCOMPtr<nsIBaseWindow> basewindow = do_QueryInterface(mBrowser);
			basewindow->Destroy();
		}
		break;
	case WM_ERASEBKGND:
		if(mChrome && mChrome->mDocumentLoaded)
			return 1;
	default:
		return ::DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

DWORD UiThread(LPVOID lpParam)
{
	WNDCLASS wc;
	memset(&wc, 0, sizeof(WNDCLASS));
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = (WNDPROC)MozViewProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = GetModuleHandle(NULL);
//	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GECKO));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = TEXT("NJORD_GECKOEMBED_2");
	RegisterClass(&wc);
	nsresult rv;

	int initCode = InitGRE(lpParam);
	if(initCode != 0)
		return initCode;

	InitializeCriticalSection(&viewsLock);

	InterlockedIncrement(&ThreadInitialized);
	while(keepUIGoing)
	{
		EnterCriticalSection(&viewsLock);
		PrivateData * curView = viewsHead;
		while(curView != NULL)
		{
			if(curView->initialized == FALSE)
			{
				HWND nWnd = CreateWindowW(TEXT("NJORD_GECKOEMBED_2"), TEXT("Gecko"), WS_OVERLAPPEDWINDOW, curView->requestedRect.left, curView->requestedRect.top,
					curView->requestedRect.right - curView->requestedRect.left, curView->requestedRect.bottom - curView->requestedRect.top, NULL, NULL,
					GetModuleHandle(NULL), NULL);
				curView->mNativeWindow = nWnd;
				curView->mChrome = new WebBrowserChrome();
				curView->mChrome->CreateBrowser(nWnd);
				curView->mChrome->GetWebBrowser(getter_AddRefs(curView->mBrowser));
				SetWindowLongPtrW(nWnd, GWLP_USERDATA, (LONG_PTR)curView->mChrome);
				curView->mDOMWindow = do_GetInterface(curView->mBrowser);

				SetFocus(nWnd);
				nsCOMPtr<nsIWebBrowserFocus> mFocus = do_QueryInterface(curView->mBrowser);
				mFocus->Activate();
		
				curView->mChrome->mAllowClose = curView->allowClose;
				curView->initialized = TRUE;
			}
			if(curView->destroying == TRUE)
			{
				DestroyWindow(curView->mNativeWindow);
				curView->destroying = FALSE;
			}
			curView = curView->next;
		}
		LeaveCriticalSection(&viewsLock);

		MSG msg;
		if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
			Sleep(10);
	}

	NS_LogInit();
	XRE_TermEmbedding();
	NS_IF_RELEASE(sProfileLock);
	sProfileDir = NULL;

	InterlockedDecrement(&ThreadInitialized);
	DeleteCriticalSection(&viewsLock);
	UnregisterClassW(TEXT("NJORD_GECKOEMBED_2"), GetModuleHandle(NULL));
	return 0;
}

JSBool g2_init(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	keepUIGoing = TRUE;
	LPSTR pathToGRE = NULL;
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "/s", &pathToGRE))
	{
		JS_ReportError(cx, "Error parsing arguments in g2_init");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	HANDLE threadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)UiThread, pathToGRE, 0, NULL);
	LONG stillWaiting = 1;
	DWORD exitCode;
	while(stillWaiting)
	{
		Sleep(1000);
		InterlockedCompareExchange(&stillWaiting, 0, ThreadInitialized);
		GetExitCodeThread(threadHandle, &exitCode);
		if(exitCode != STILL_ACTIVE)
			break;
	}
	if(exitCode != STILL_ACTIVE)
		*rval = JSVAL_FALSE;
	else
		*rval = JSVAL_TRUE;
	return JS_TRUE;
}

JSBool g2_term(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	keepUIGoing = FALSE;
	LONG stopped = 0;
	while(!stopped)
	{
		Sleep(250);
		InterlockedCompareExchange(&stopped, 1, ThreadInitialized);
	}

    nsresult rv;
    return JS_TRUE;
}