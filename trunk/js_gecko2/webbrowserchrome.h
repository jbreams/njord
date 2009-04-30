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

	nsresult CreateBrowser(PRInt32 aX, PRInt32 aY, PRInt32 aCX, PRInt32 aCY);

protected:

    HWND         mNativeWindow;
	HWND		 mContainerWindow;
    PRUint32     mChromeFlags;
    PRBool       mContinueModalLoop;
    PRBool       mSizeSet;
	PRBool		 mDocumentLoaded;
	LPSTR		 mCurrentLocation;

    nsCOMPtr<nsIWebBrowser> mWebBrowser;
};