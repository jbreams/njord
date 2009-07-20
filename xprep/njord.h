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

#pragma once

#include "resource.h"

LPWSTR LoadFile(LPTSTR fileName);
BOOL ShutdownSigning();
BOOL StartupSigning();

JSBool InitNativeLoad(JSContext * cx, JSObject * global);
JSBool InitExec(JSContext * cx, JSObject * global);
JSBool InitFile(JSContext * cx, JSObject * global);
void InitWin32s(JSContext * cx, JSObject * global);
JSBool InitFindFile(JSContext * cx, JSObject * global);
JSBool InitRegistry(JSContext * cx, JSObject * global);
void Cleanup();