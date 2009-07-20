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

#include "stdafx.h"
// CRT headers
#include <iostream>
#include <string>
using namespace std;

#include "nsXULAppAPI.h"
#include "nsXPCOMGlue.h"
#include "nsCOMPtr.h"
#include "nsStringAPI.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsProfileDirServiceProvider.h"

#include "nsILocalFile.h"
extern CRITICAL_SECTION viewsLock;
extern BOOL keepUIGoing;

static int gInitCount = 0;

#ifdef __cplusplus
extern "C" {
#endif

BOOL __declspec(dllexport) CleanupExports(JSContext * cx, JSObject * global)
{
	if(NS_FAILED(XPCOMGlueShutdown()))
	{
		JS_ReportError(cx, "XRE_Termembedding failed!");
		return FALSE;
	}
	return TRUE;
}
#ifdef __cplusplus
}
#endif