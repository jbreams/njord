#include "stdafx.h"
#include "contentlistener.h"
#include "js_gecko2.h"

#include "nsXULAppAPI.h"
#include "nsXPCOMGlue.h"
#include "nsStringAPI.h"
#include "nsCOMPtr.h"
#include "nsServiceManagerUtils.h"
#include "nsDocShellCID.h"
#include "nsILocalFile.h"
#include "nsIURI.h"
#include "nsIWebNavigationInfo.h"

ContentListener::ContentListener(njEncap *aOwner, nsIWebNavigation *aNavigation)
{
	pEncap = aOwner;
	mNavigation = aNavigation;
}

ContentListener::~ContentListener()
{

}

NS_IMPL_ISUPPORTS2(ContentListener, nsIURIContentListener, nsISupportsWeakReference)

NS_IMETHODIMP ContentListener::OnStartURIOpen(nsIURI *aURI, PRBool *aAbortOpen)
{
	nsresult rv;
	nsCAutoString specString;
	rv = aURI->GetSpec(specString);
	NS_ENSURE_SUCCESS(rv, rv);

	jsval rval, argv = STRING_TO_JSVAL(JS_NewUCStringCopyZ(pEncap->getContext(), (jschar*)specString.get()));
	if(pEncap->HandleCallback(GECKO_OPENURI, 1, &argv, &rval))
		*aAbortOpen = rval == JSVAL_TRUE ? true : false;
	else
		*aAbortOpen = false;
	return NS_OK;
}

NS_IMETHODIMP ContentListener::DoContent(const char *aContentType, PRBool aIsContentPreferred, nsIRequest *aRequest, nsIStreamListener **aContentHandler, PRBool *_retval)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP ContentListener::IsPreferred(const char *aContentType, char **aDesiredContentType, PRBool *_retval)
{
	return CanHandleContent(aContentType, PR_TRUE, aDesiredContentType, _retval);
}

NS_IMETHODIMP ContentListener::CanHandleContent(const char *aContentType, PRBool aIsContentPreferred, char **aDesiredContentType, PRBool *_retval)
{
	PRUint32 canHandle;
	*_retval = PR_FALSE;
	*aDesiredContentType = nsnull;

	if(!aContentType)
		return NS_ERROR_FAILURE;

	nsCOMPtr<nsIWebNavigationInfo> webNavInfo(do_GetService(NS_WEBNAVIGATION_INFO_CONTRACTID));
	nsresult rv = webNavInfo->IsTypeSupported(nsDependentCString(aContentType), mNavigation ? mNavigation.get() : nsnull, &canHandle);
	NS_ENSURE_SUCCESS(rv, rv);
	*_retval = (canHandle != nsIWebNavigationInfo::UNSUPPORTED);
	return NS_OK;
}

NS_IMETHODIMP
ContentListener::GetLoadCookie(nsISupports ** /*aLoadCookie*/)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
ContentListener::SetLoadCookie(nsISupports * /*aLoadCookie*/)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
ContentListener::GetParentContentListener(nsIURIContentListener ** /*aParent*/)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
ContentListener::SetParentContentListener(nsIURIContentListener * /*aParent*/)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}