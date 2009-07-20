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
#include "js_wimg.h"
#include <commctrl.h>
#include <intrin.h>
#pragma intrinsic(_InterlockedAnd)

struct uiInfo * uiHead;
HANDLE headMutex;

DWORD CALLBACK WIMCallback(DWORD messageID, WPARAM wParam, LPARAM lParam, LPVOID lpvUserData)
{
	struct uiInfo * myInfo = (struct uiInfo*)lpvUserData;
	if(myInfo->hDlg == NULL)
		return WIM_MSG_SUCCESS;
	switch(messageID)
	{
	case WIM_MSG_PROGRESS:
		{
			LPTSTR timeRemainingText = (LPWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 512 * sizeof(TCHAR));
			if(lParam == 0)
				_stprintf_s(timeRemainingText, 512, TEXT("%u%% Completed. Estimated time till completion: calculating."), wParam, lParam / 1000);
			else
			{
				DWORD minutes = 0;
				DWORD seconds = lParam / 1000;
				if(seconds > 60)
				{
					minutes = seconds / 60;
					seconds = seconds - (minutes * 60);
				}
				if(minutes > 0)
					_stprintf_s(timeRemainingText, 512, TEXT("%u%% Completed. Estimated time till completion: %u mins %u secs"), wParam, minutes, seconds);
				else
					_stprintf_s(timeRemainingText, 512, TEXT("%u%% Completed. Estimated time till completion: %u secs"), wParam, seconds);
			}
			SetDlgItemText(myInfo->hDlg, IDC_STATIC3, timeRemainingText);
			HeapFree(GetProcessHeap(), 0, timeRemainingText);
			SendDlgItemMessage(myInfo->hDlg, IDC_PROGRESS, PBM_SETPOS, wParam, 0);
		}
		break;
	case WIM_MSG_FILEINFO:
		SetDlgItemText(myInfo->hDlg, IDC_STATIC2, (LPWSTR)wParam);
		break;

	}
	
	return WIM_MSG_SUCCESS;
}

INT_PTR CALLBACK ProgressDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
		SendDlgItemMessage(hWnd, IDC_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
		return TRUE;
	}
	return FALSE;
}

DWORD uiThread(LPVOID param)
{
	BOOL active = FALSE;
	HINSTANCE thisInstance = (HINSTANCE)param;

	INITCOMMONCONTROLSEX icce;
	icce.dwSize = sizeof(icce);
	icce.dwICC = ICC_PROGRESS_CLASS;
	InitCommonControlsEx(&icce);

	struct uiInfo * curUi;
	do
	{
		WaitForSingleObject(headMutex, INFINITE);
		curUi = uiHead;
		while(curUi != NULL)
		{
			switch(curUi->state)
			{
			case UISTATE_ADDED:
				curUi->hDlg = CreateDialogW(thisInstance, MAKEINTRESOURCE(IDD_PROGRESS), NULL, ProgressDlgProc);
				if(curUi->hDlg != NULL)
					WIMRegisterMessageCallback(curUi->wimFile, (FARPROC)WIMCallback, (LPVOID)curUi);
				curUi->state = UISTATE_RUNNING;
				active = TRUE;
				break;
			case UISTATE_RUNNING:
				active = TRUE;
				break;
			case UISTATE_DYING:
				WIMUnregisterMessageCallback(curUi->wimFile, (FARPROC)WIMCallback);
				DestroyWindow(curUi->hDlg);
				curUi->state = UISTATE_DEAD;
				active = (active == TRUE) ? TRUE : FALSE;
				break;
			}
			curUi = curUi->next;
		}
		ReleaseMutex(headMutex);
		MSG msg;
		if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	} while(active);

	WaitForSingleObject(headMutex, INFINITE);
	curUi = uiHead;
	while(curUi != NULL)
	{
		struct uiInfo * nextUi = curUi->next;
		HeapFree(GetProcessHeap(), 0, curUi);
		curUi = nextUi;
	}
	uiHead = NULL;
	ReleaseMutex(headMutex);
	CloseHandle(headMutex);
	return 0;
}