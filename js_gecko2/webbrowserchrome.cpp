#include "stdafx.h"
#include "WebBrowserChrome.h"
#include "nsStringAPI.h"
#include "nsIDOMWindow.h"
#include "nsIURI.h"
#include "jsapi.h"
#include "jsstr.h"
#include "js_gecko2.h"

WebBrowserChrome::WebBrowserChrome(njEncap *pAEncap) :
	mChromeFlags(0),
	mSizeSet(PR_FALSE),
	pEncap(pAEncap),
	mIsModal(PR_FALSE)
{
}

WebBrowserChrome::~WebBrowserChrome()
{
}

NS_IMETHODIMP WebBrowserChrome::GetInterface(const nsIID &uuid, void **result)
{
	NS_ENSURE_ARG_POINTER(result);
	*result = NULL;
	if(uuid.Equals(NS_GET_IID(nsIDOMWindow)))
	{
		if(!mWebBrowser)
			return NS_ERROR_NOT_INITIALIZED;
		return mWebBrowser->GetContentDOMWindow((nsIDOMWindow**)result);
	}
	return QueryInterface(uuid, result);
}

NS_IMETHODIMP WebBrowserChrome::SetStatus(PRUint32 statusType, const PRUnichar *status)
{
	JSString * statusStr = JS_NewUCStringCopyZ(pEncap->getContext(), (jschar*)status);
	jsval argv[2];
	argv[0] = STRING_TO_JSVAL(statusStr);
	JS_NewNumberValue(pEncap->getContext(), statusType, &argv[1]);
	pEncap->HandleCallback(GECKO_SETSTATUS, 2, argv, NULL);
	return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::GetWebBrowser(nsIWebBrowser **aWebBrowser)
{
	NS_ENSURE_ARG_POINTER(aWebBrowser);
	*aWebBrowser = mWebBrowser;
	NS_IF_ADDREF(*aWebBrowser);
	return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::GetChromeFlags(PRUint32 * aChromeFlags)
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
	if(mIsModal)
		mIsModal = PR_FALSE;
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::SizeBrowserTo(PRInt32 aCX, PRInt32 aCY)
{
	jsval argv[2];
	JS_NewNumberValue(pEncap->getContext(), aCX, &argv[0]);
	JS_NewNumberValue(pEncap->getContext(), aCY, &argv[1]);
	jsval rval;
	pEncap->HandleCallback(GECKO_SIZEBROWSERTO, 2, argv, &rval);

	if(rval == JSVAL_TRUE)
	{
		RECT parentRect, rect;
		::GetWindowRect(pEncap->GetWndHandle(TRUE), &parentRect);
		::GetWindowRect(pEncap->GetWndHandle(FALSE), &rect);
		::SetWindowPos(pEncap->GetWndHandle(FALSE), 0, 0, 0,
			aCX + (parentRect.right - parentRect.left) - (rect.right - rect.left),
			aCY + (parentRect.bottom - parentRect.top) - (rect.bottom - rect.top),
			SWP_NOMOVE | SWP_NOZORDER);
	}
	return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::ShowAsModal()
{
	jsval rval;
	if(!pEncap->HandleCallback(GECKO_SHOWASMODAL, 0, NULL, &rval))
		return NS_ERROR_NOT_IMPLEMENTED;
	if(rval == JSVAL_TRUE)
	{
		mIsModal = PR_TRUE;
		MSG msg;
		while(mIsModal == PR_TRUE && GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::IsWindowModal(PRBool *_retval)
{
	NS_ENSURE_ARG_POINTER(_retval);
	*_retval = mIsModal;
	return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::ExitModalEventLoop(nsresult aStatus)
{
	jsval rval;
	if(!pEncap->HandleCallback(GECKO_EXITMODAL, 0, NULL, &rval))
		return NS_ERROR_NOT_IMPLEMENTED;
	if(rval == JSVAL_TRUE)
		mIsModal = PR_FALSE;
	return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::OnStateChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRUint32 aStateFlags, nsresult aStatus)
{
	if((aStateFlags & STATE_STOP) &&(aStateFlags & STATE_IS_DOCUMENT))
	{
		if(!mSizeSet && (mChromeFlags & nsIWebBrowserChrome::CHROME_OPENAS_CHROME))
		{
			nsCOMPtr<nsIDOMWindow> contentWin;
			mWebBrowser->GetContentDOMWindow(getter_AddRefs(contentWin));
			if(contentWin)
				contentWin->SizeToContent();
			SetVisibility(PR_TRUE);
		}
		pEncap->HandleCallback(GECKO_DOCUMENTLOADED, 0, NULL, NULL);
	}
	return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::OnProgressChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRInt32 aCurSelfProgress, PRInt32 aMaxSelfProgress, PRInt32 aCurTotalProgress, PRInt32 aMaxTotalProgress)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::OnLocationChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, nsIURI *aLocation)
{
	NS_ENSURE_ARG_POINTER(aLocation);
	nsCString spec;
	aLocation->GetSpec(spec);
	jsval argv = STRING_TO_JSVAL(JS_NewUCStringCopyZ(pEncap->getContext(), (jschar*)spec.get()));
	if(!pEncap->HandleCallback(GECKO_LOCATION, 1, &argv, NULL))
		return NS_ERROR_NOT_IMPLEMENTED;
	return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::OnStatusChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, nsresult aStatus, const PRUnichar *aMessage)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::OnSecurityChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRUint32 aState)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::SetDimensions(PRUint32 flags, PRInt32 x, PRInt32 y, PRInt32 cx, PRInt32 cy)
{
	jsval argv[4];
	JS_NewNumberValue(pEncap->getContext(), x, &argv[0]);
	JS_NewNumberValue(pEncap->getContext(), y, &argv[1]);
	JS_NewNumberValue(pEncap->getContext(), cx, &argv[2]);
	JS_NewNumberValue(pEncap->getContext(), cy, &argv[3]);
	jsval rval;
	pEncap->HandleCallback(GECKO_SIZEBROWSERTO, 2, argv, &rval);

	if(rval == JSVAL_TRUE)
	{
		RECT parentRect, rect;
		::GetWindowRect(pEncap->GetWndHandle(TRUE), &parentRect);
		::GetWindowRect(pEncap->GetWndHandle(FALSE), &rect);
		::SetWindowPos(pEncap->GetWndHandle(TRUE), NULL, x, y, 
			cx + (parentRect.right - parentRect.left) - (rect.right - rect.left),
			cy + (parentRect.bottom - parentRect.top) - (rect.bottom - rect.top),
			SWP_NOMOVE | SWP_NOZORDER);
	}
	mSizeSet = PR_TRUE;
	return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::GetDimensions(PRUint32 flags, PRInt32 *x, PRInt32 *y, PRInt32 *cx, PRInt32 *cy)
{
	RECT parentRect;
	::GetWindowRect(pEncap->GetWndHandle(TRUE), &parentRect);
	*x = parentRect.left;
	*y = parentRect.top;
	*cx = parentRect.right - parentRect.left;
	*cy = parentRect.bottom - parentRect.top;
	return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::SetFocus()
{
	::SetFocus(pEncap->GetWndHandle(TRUE));
	return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::GetVisibility(PRBool *aVisibility)
{
	NS_ENSURE_ARG_POINTER(aVisibility);
	*aVisibility = IsWindowVisible(pEncap->GetWndHandle(TRUE));
	return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::SetVisibility(PRBool aVisibility)
{
	jsval argv, rval;
	argv = aVisibility ? JSVAL_TRUE: JSVAL_FALSE;
	if(pEncap->HandleCallback(GECKO_VISIBILITY, 1, &argv, &rval))
	{
		if(rval == JSVAL_TRUE)
			::ShowWindow(pEncap->GetWndHandle(TRUE), aVisibility ? SW_SHOW : SW_HIDE);
	}
	else
		return NS_ERROR_NOT_IMPLEMENTED;
	return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::GetTitle(PRUnichar **aTitle)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::SetTitle(const PRUnichar *aTitle)
{
	jsval rval;
	jsval argv = STRING_TO_JSVAL(JS_NewUCStringCopyZ(pEncap->getContext(), (jschar*)aTitle));
	if(!pEncap->HandleCallback(GECKO_SETTITLE, 1, &argv, &rval))
		return NS_ERROR_NOT_IMPLEMENTED;
	if(rval == JSVAL_TRUE)
		SetWindowText(pEncap->GetWndHandle(TRUE), aTitle);
	return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::GetSiteWindow(void **aSiteWindow)
{
	NS_ENSURE_ARG_POINTER(aSiteWindow);
	*aSiteWindow = pEncap->GetWndHandle(TRUE);
	return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::FocusNextElement()
{
	return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::FocusPrevElement()
{
	return NS_OK;
}