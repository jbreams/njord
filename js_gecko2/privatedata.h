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

class WebBrowserChrome;
class DOMEventListener;
#include "nsCOMPtr.h"

class PrivateData 
{
public:
	PrivateData()
	{
		next = NULL;
		initialized = FALSE;
		mChrome = NULL;
		mNativeWindow = NULL;
		allowClose = FALSE;
		mDOMListener = NULL;
		nDOMListeners = 0;
	}
	~PrivateData()
	{
	}

	WebBrowserChrome * mChrome;
	DOMEventListener * mDOMListener;
	DWORD nDOMListeners;
	RECT requestedRect;
	BOOL initialized;
	BOOL allowClose;
	BOOL destroying;
	HWND mNativeWindow;
	nsCOMPtr<nsIWebBrowser> mBrowser;
	nsCOMPtr<nsIDOMWindow> mDOMWindow;
	nsCOMPtr<nsIProxyObjectManager> nsIPO;
	PrivateData * next;

	JSContext * mContext;
};