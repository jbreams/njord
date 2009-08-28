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
#include "WebBrowserChrome.h"
#include "nsStringAPI.h"
#include "nsIDOMWindow.h"
#include "nsIURI.h"
#include "nsComponentManagerUtils.h"
#include "nsIBaseWindow.h"
#include "nsEmbedCID.h"
#include "nsIWebNavigation.h"
#include "nsMemory.h"
#include "nsIWidget.h"
#include "privatedata.h"

extern CRITICAL_SECTION domNodeLock;

WebBrowserChrome::WebBrowserChrome()
{
	mNativeWindow = NULL;
	mSizeSet = PR_FALSE;
	mDocumentLoaded = PR_FALSE;
	mContinueModalLoop = PR_FALSE;
	mChromeFlags = 0;
	mDocumentLoaded = CreateEvent(NULL, TRUE, FALSE, NULL);
}
	
WebBrowserChrome::~WebBrowserChrome()
{
	CloseHandle(mDocumentLoaded);
}

NS_IMPL_ISUPPORTS6(WebBrowserChrome,
				   nsIWebBrowserChrome,
				   nsIWebBrowserChromeFocus,
				   nsIInterfaceRequestor,
				   nsIEmbeddingSiteWindow,
				   nsIWebProgressListener,
				   nsSupportsWeakReference)

nsresult WebBrowserChrome::CreateBrowser(HWND nativeWnd)
{
	nsresult rv;
	mWebBrowser = do_CreateInstance(NS_WEBBROWSER_CONTRACTID, &rv);
	if(NS_FAILED(rv))
		return NS_ERROR_FAILURE;

	AddRef();
	if(NS_FAILED(mWebBrowser->SetContainerWindow((nsIWebBrowserChrome*)this)))
		return NS_ERROR_FAILURE;
	
	mNativeWindow = nativeWnd;
	RECT area;
	GetClientRect(mNativeWindow, &area);
	nsCOMPtr<nsIBaseWindow> baseWindow = do_QueryInterface(mWebBrowser);
	if(NS_FAILED(baseWindow->InitWindow(mNativeWindow, NULL, area.left, area.top, area.right - area.left, area.bottom - area.top)))
		return NS_ERROR_FAILURE;
	if(NS_FAILED(baseWindow->Create()))
		return NS_ERROR_FAILURE;
	baseWindow->SetVisibility(PR_TRUE);

	nsWeakPtr weakling(dont_AddRef(NS_GetWeakReference(static_cast<nsIWebProgressListener*>(this))));
	if(NS_FAILED(mWebBrowser->AddWebBrowserListener(weakling, NS_GET_IID(nsIWebProgressListener))))
		return NS_ERROR_FAILURE;

	return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::GetInterface(const nsIID &aIID, void** aInstancePtr)
{
    NS_ENSURE_ARG_POINTER(aInstancePtr);

    *aInstancePtr = 0;
    if (aIID.Equals(NS_GET_IID(nsIDOMWindow)))
    {
        if (!mWebBrowser)
            return NS_ERROR_NOT_INITIALIZED;

        return mWebBrowser->GetContentDOMWindow((nsIDOMWindow **) aInstancePtr);
    }
    return QueryInterface(aIID, aInstancePtr);
}

NS_IMETHODIMP WebBrowserChrome::SetStatus(PRUint32 statusType, const PRUnichar *status)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::GetWebBrowser(nsIWebBrowser * *aWebBrowser)
{
    NS_ENSURE_ARG_POINTER(aWebBrowser);
    *aWebBrowser = mWebBrowser;
    NS_IF_ADDREF(*aWebBrowser);
    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::SetWebBrowser(nsIWebBrowser * aWebBrowser)
{
    mWebBrowser = aWebBrowser;
    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::GetChromeFlags(PRUint32 *aChromeFlags)
{
    *aChromeFlags = mChromeFlags;
    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::SetChromeFlags(PRUint32 aChromeFlags)
{
    mChromeFlags = aChromeFlags;
    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::DestroyBrowserWindow()
{
	if(mContinueModalLoop)
		mContinueModalLoop = PR_FALSE;
	SendMessage(mNativeWindow, WM_DESTROY, 0, 0);
	return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::SizeBrowserTo(PRInt32 aCX, PRInt32 aCY)
{
	nsCOMPtr<nsIBaseWindow> baseWindow = do_QueryInterface(mWebBrowser);
	nsCOMPtr<nsIWidget> gWidget;
	baseWindow->GetMainWidget(getter_AddRefs(gWidget));
	HWND viewWnd = (HWND)gWidget->GetNativeData(NS_NATIVE_WINDOW);
	
	RECT rect, parentRect;
	GetWindowRect(mNativeWindow, &parentRect);
	GetWindowRect(viewWnd, &rect);
	SetWindowPos(mNativeWindow, NULL, 0, 0, 
		aCX + (parentRect.right - parentRect.left) - (rect.right - rect.left),
		aCY + (parentRect.bottom - parentRect.top) - (rect.bottom - rect.top),
		SWP_NOMOVE | SWP_NOZORDER);
	return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::ShowAsModal()
{
	mContinueModalLoop = PR_TRUE;
	while(mContinueModalLoop)
	{
		MSG msg;
		if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
			Sleep(100);
	}
    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::IsWindowModal(PRBool *_retval)
{
    NS_ENSURE_ARG_POINTER(_retval);
    *_retval = mContinueModalLoop;
    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::ExitModalEventLoop(nsresult aStatus)
{
	mContinueModalLoop = PR_FALSE;
    return NS_OK;
}

// ----- Progress Listener -----

NS_IMETHODIMP WebBrowserChrome::OnStateChange(nsIWebProgress * /*aWebProgress*/,
                                              nsIRequest * /*aRequest*/,
                                              PRUint32 aStateFlags,
                                              nsresult /*aStatus*/)
{
    if ((aStateFlags & STATE_STOP) && (aStateFlags & STATE_IS_DOCUMENT)) {
        // if it was a chrome window and no one has already specified a size,
        // size to content
        if (!mSizeSet &&
            (mChromeFlags & nsIWebBrowserChrome::CHROME_OPENAS_CHROME)) {
            nsCOMPtr<nsIDOMWindow> contentWin;
            mWebBrowser->GetContentDOMWindow(getter_AddRefs(contentWin));
            if (contentWin)
                contentWin->SizeToContent();
            SetVisibility(PR_TRUE);
        }

		SetEvent(mDocumentLoaded);
    }

    return NS_OK;
}


NS_IMETHODIMP WebBrowserChrome::OnProgressChange(nsIWebProgress * /*aWebProgress*/,
                                                 nsIRequest * /*aRequest*/,
                                                 PRInt32 /*aCurSelfProgress*/,
                                                 PRInt32 /*aMaxSelfProgress*/,
                                                 PRInt32 /*aCurTotalProgress*/,
                                                 PRInt32 /*aMaxTotalProgress*/)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::OnLocationChange(nsIWebProgress * /*aWebProgress*/,
                                                 nsIRequest * /*aRequest*/,
                                                 nsIURI *aLocation)
{
    NS_ENSURE_ARG_POINTER(aLocation);
    nsCString spec;
    aLocation->GetSpec(spec);
	JS_BeginRequest(cx);
	JSString * newLocation = JS_NewString(cx, (char*)spec.get(), spec.Length());
	jsval newLocationVal = STRING_TO_JSVAL(newLocation);
	JS_SetProperty(cx, mPrivate->obj, "location", &newLocationVal);
	JS_EndRequest(cx);
    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::OnStatusChange(nsIWebProgress * /*aWebProgress*/,
                                               nsIRequest * /*aRequest*/,
                                               nsresult /*aStatus*/,
                                               const PRUnichar * /*aMessage*/)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::OnSecurityChange(nsIWebProgress * /*aWebProgress*/,
                                                 nsIRequest * /*aRequest*/,
                                                 PRUint32 /*aState*/)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::SetDimensions(PRUint32 flags,
                                              PRInt32 aX, PRInt32 aY,
                                              PRInt32 aCx, PRInt32 aCy)
{
	RECT newPos;
	UINT swpFlags = SWP_NOZORDER;

	GetWindowRect(mNativeWindow, &newPos);
	if(flags & nsIEmbeddingSiteWindow::DIM_FLAGS_POSITION)
	{
		newPos.left = aX;
		newPos.top = aY;
	}
	else
		swpFlags |= SWP_NOMOVE;
	if(flags & (nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_INNER 
		| nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_OUTER))
	{
		newPos.bottom = newPos.top + aCy;
		newPos.right = newPos.left + aCx;
		if(flags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_INNER)
			AdjustWindowRect(&newPos, WS_OVERLAPPEDWINDOW, FALSE);
	}
	if(SetWindowPos(mNativeWindow, NULL, newPos.left, newPos.top, newPos.right - newPos.left, newPos.bottom - newPos.top, swpFlags))
	{
		mSizeSet = PR_TRUE;
		return NS_OK;
	}
	return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP WebBrowserChrome::GetDimensions(PRUint32 flags,
                                              PRInt32 * X, PRInt32 * Y,
                                              PRInt32 * cX, PRInt32 * cY)
{
	RECT wndRect;
	if(!GetWindowRect(mNativeWindow, &wndRect))
		return NS_ERROR_UNEXPECTED;
	if(flags & nsIEmbeddingSiteWindow::DIM_FLAGS_POSITION)
	{
		*X = wndRect.left;
		*Y = wndRect.top;
	}
	if(flags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_OUTER)
	{
		*cX = wndRect.right - wndRect.left;
		*cY = wndRect.bottom - wndRect.top;
	}
    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::SetFocus()
{
	::SetForegroundWindow(mNativeWindow);
	::SetFocus(mNativeWindow);
    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::GetVisibility(PRBool * aVisibility)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::SetVisibility(PRBool aVisibility)
{
	ShowWindow(mNativeWindow, aVisibility ? SW_SHOW : SW_HIDE);
    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::GetTitle(PRUnichar ** /*aTitle*/)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::SetTitle(const PRUnichar *aTitle)
{
	SetWindowText(mNativeWindow, aTitle);
    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::GetSiteWindow(void * *aSiteWindow)
{
    NS_ENSURE_ARG_POINTER(aSiteWindow);
    *aSiteWindow = mNativeWindow;
    return NS_OK;
}

// ----- WebBrowser Chrome Focus

NS_IMETHODIMP WebBrowserChrome::FocusNextElement()
{
	return NS_ERROR_NOT_IMPLEMENTED;
//    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::FocusPrevElement()
{
	return NS_ERROR_NOT_IMPLEMENTED;
}
