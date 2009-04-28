#include "stdafx.h"
#include "nsXULAppApi.h"
#include "nsXPComGlue.h"
#include "nsCOMPtr.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsComponentManagerUtils.h"
#include "nsWeakReference.h"
#include "nsStringAPI.h"
#include "domeventlistener.h"
#include "nsIDOMEvent.h"
#include "nsIDOMEventTarget.h"

NS_IMPL_ISUPPORTS1(DOMEventListener, nsIDOMEventListener)

DOMEventListener::DOMEventListener(JSContext *aCx)
{
	cx = aCx;
}

DOMEventListener::~DOMEventListener()
{

}

NS_IMETHODIMP DOMEventListener::HandleEvent(nsIDOMEvent *aEvent)
{
	nsIDOMEventTarget * target;
	return NS_OK;
}