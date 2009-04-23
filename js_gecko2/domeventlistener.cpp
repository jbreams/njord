#include "stdafx.h"
#include "js_gecko2.h"
#include "domeventlistener.h"
#include "nsIDOMEvent.h"

NS_IMPL_ISUPPORTS1(EventListener,
				   nsIDOMEventListener);

EventListener::EventListener(njEncap *aOwner)
: mOwner(aOwner)
{

}

EventListener::~EventListener()
{

}

NS_IMETHODIMP EventListener::HandleEvent(nsIDOMEvent *domevent)
{
	return NS_OK;
}