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

#include "nsIDOMEventListener.h"
#include "nsIDOMEventTarget.h"
#include "nsStringApi.h"

class PrivateData;

class EventRegistration
{
public:
	EventRegistration();
	~EventRegistration();

	nsIDOMNode * target;
	nsString domEvent;
	LPSTR callback;
	HANDLE windowsEvent;

	EventRegistration * next;
};

class DOMEventListener : public nsIDOMEventListener
{
public:
	DOMEventListener(PrivateData * aOwner);
	virtual ~DOMEventListener();

	NS_DECL_ISUPPORTS
	NS_DECL_NSIDOMEVENTLISTENER

	BOOL RegisterEvent(nsIDOMNode *target, LPWSTR type, LPSTR callback);
	void UnregisterEvent(nsIDOMNode * targetNode, LPWSTR type);
	void UnregisterAll();
	BOOL WaitForSingleEvent(nsIDOMNode *target, LPWSTR type, DWORD timeout);
	BOOL WaitForAllEvents(DWORD timeout, BOOL waitAll, JSObject ** out);

private:
	PrivateData * mOwner;
	EventRegistration * head;
	DWORD regCount;
	CRITICAL_SECTION headLock;
	HANDLE addRemoveMutex;

};