#include "nsCOMPtr.h"
#include "nsIWebBrowser.h"
#include "nsIWebBrowserChrome.h"
#include "nsIWebBrowserChromeFocus.h"
#include "nsIWebProgressListener.h"
#include "nsIEmbeddingSiteWindow.h"
#include "nsIInterfaceRequestor.h"
#include "nsWeakReference.h"

class njEncap;

class WebBrowserChrome : public nsIWebBrowserChrome,
	public nsIWebProgressListener,
	public nsIWebBrowserChromeFocus,
	public nsIEmbeddingSiteWindow,
	public nsIInterfaceRequestor,
	public nsSupportsWeakReference
{
public:
	NS_DECL_ISUPPORTS
	NS_DECL_NSIWEBBROWSERCHROME
	NS_DECL_NSIWEBPROGRESSLISTENER
	NS_DECL_NSIWEBBROWSERCHROMEFOCUS
	NS_DECL_NSIEMBEDDINGSITEWINDOW
	NS_DECL_NSIINTERFACEREQUESTOR

	WebBrowserChrome(njEncap * pAEncap);

	virtual ~WebBrowserChrome();
	njEncap * GetView() { return pEncap; }

protected:
	nsCOMPtr<nsIWebBrowser> mWebBrowser;
	PRUint32 mChromeFlags;
	PRBool mSizeSet;
	njEncap * pEncap;
	PRBool mIsModal;
};