#include "stdafx.h"
#include "js_gecko2.h"
#include "webbrowserchrome.h"
#include "domeventlistener.h"
#include "contentlistener.h"

#include "nsXULAppAPI.h"
#include "nsXPCOMGlue.h"
#include "nsCOMPtr.h"
#include "nsStringAPI.h"
#include "nsComponentManagerUtils.h"
#include "nsNetUtil.h"
#include "nsEmbedCID.h"

#include "nsIBaseWindow.h"
#include "nsIConsoleService.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMWindow2.h"
#include "nsIDOMWindowInternal.h"
#include "nsIPref.h"
#include "nsIScriptContext.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIURI.h"
#include "nsIWeakReference.h"
#include "nsIWeakReferenceUtils.h"
#include "nsIWebBrowser.h"
#include "nsIWebBrowserFocus.h"
#include "nsIWebBrowserStream.h"
#include "nsIWebNavigation.h"
#include "nsIWidget.h"
#include "nsIWindowCreator2.h"
#include "nsIWindowWatcher.h"

class njEncap::Private
{
	Private() : mParentWindow(0), mParentView(0) { }
	~Private()
	{

	}

	HWND mParentWindow;
	njEncap * mParentView;

	nsCOMPtr<nsIWebBrowser> mWebBrowser;
	nsCOMPtr<nsIDOMWindow2> mDOMWindow;
	nsCOMPtr<nsIWebNavigation> mWebNavigation;
	nsCOMPtr<nsIWebBrowserChrome> mChrome;
	nsCOMPtr<nsIURIContentListener> mContentListener;
	nsCOMPtr<nsIDOMEventListener> mDOMEventListener;
	nsCOMPtr<nsIConsoleListener> mConsoleListener;
};