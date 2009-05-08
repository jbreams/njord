#include "nsIWebBrowserChrome.h"
#include "nsIWebBrowserChromeFocus.h"

#include "nsIWebBrowser.h"
#include "nsIWebProgressListener.h"
#include "nsIEmbeddingSiteWindow.h"
#include "nsIInterfaceRequestor.h"

#include "nsCOMPtr.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsWeakReference.h"

class WebBrowserChrome   : public nsIWebBrowserChrome,
						   public nsIWebBrowserChromeFocus,
						   public nsIInterfaceRequestor,
						   public nsIEmbeddingSiteWindow,
						   public nsIWebProgressListener,
						   public nsSupportsWeakReference
{
public:
    WebBrowserChrome();
    virtual ~WebBrowserChrome();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBBROWSERCHROME
    NS_DECL_NSIWEBPROGRESSLISTENER
    NS_DECL_NSIWEBBROWSERCHROMEFOCUS
    NS_DECL_NSIEMBEDDINGSITEWINDOW
    NS_DECL_NSIINTERFACEREQUESTOR

	nsresult CreateBrowser(HWND nativeWnd);
	PRBool		 mDocumentLoaded;
	PRBool		 mAllowClose;

protected:

    HWND         mNativeWindow;
	HWND		 mContainerWindow;
    PRUint32     mChromeFlags;
    PRBool       mContinueModalLoop;
    PRBool       mSizeSet;
	LPSTR		 mCurrentLocation;

    nsCOMPtr<nsIWebBrowser> mWebBrowser;
};