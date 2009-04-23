#include "nsIURIContentListener.h"
#include "nsWeakReference.h"
#include "nsIWebNavigation.h"

class njEncap;
class ContentListener : public nsIURIContentListener,
	public nsSupportsWeakReference
{
public:
	ContentListener(njEncap * aOwner, nsIWebNavigation * aNavigation);
	virtual ~ContentListener();

	NS_DECL_ISUPPORTS
	NS_DECL_NSIURICONTENTLISTENER

private:
	njEncap * pEncap;
	nsCOMPtr<nsIWebNavigation> mNavigation;
};