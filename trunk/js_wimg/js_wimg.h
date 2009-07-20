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

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the JS_WIMG_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// JS_WIMG_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef JS_WIMG_EXPORTS
#define JS_WIMG_API __declspec(dllexport)
#else
#define JS_WIMG_API __declspec(dllimport)
#endif
#include "resource.h"

struct uiInfo {
	HWND hDlg;
	HANDLE wimFile;
	LONG state;
	struct uiInfo * next;
};
DWORD uiThread(LPVOID param);

#define UISTATE_ADDED 1
#define UISTATE_RUNNING 2
#define UISTATE_DYING 4
#define UISTATE_DEAD 8

class MatchEntry
{
public:
	MatchEntry(LPWSTR copyFrom);
	~MatchEntry();

	BOOL Match(LPWSTR target);
	MatchEntry * next;
private:
	LPWSTR matchString;
	BOOL fileName;
};

struct MatchSet {
	MatchEntry * head;
	HANDLE wimFile;
};

JSBool wimg_set_exceptions(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval);