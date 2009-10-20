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

DWORD nWindowsOpen = 0;
extern DWORD threadID;

DWORD CALLBACK WIMCallback(DWORD messageID, WPARAM wParam, LPARAM lParam, LPVOID lpvUserData)
{
	HWND hDlg = (HWND)lpvUserData;
	if(hDlg == NULL)
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
			SetDlgItemText(hDlg, IDC_STATIC3, timeRemainingText);
			HeapFree(GetProcessHeap(), 0, timeRemainingText);
			SendDlgItemMessage(hDlg, IDC_PROGRESS, PBM_SETPOS, wParam, 0);
		}
		break;
	case WIM_MSG_FILEINFO:
		SetDlgItemText(hDlg, IDC_STATIC2, (LPWSTR)wParam);
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

BOOL CALLBACK KillProgressWindowProc(HWND hWnd, LPARAM lParam)
{
	HANDLE wimFile = (HANDLE)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
	if(wimFile == (HANDLE)lParam)
	{
		WIMUnregisterMessageCallback(wimFile, (FARPROC)WIMCallback);
		DestroyWindow(hWnd);
		if(--nWindowsOpen == 0)
			return FALSE;
	}
	return TRUE;
}

DWORD uiThread(LPVOID param)
{
	HINSTANCE thisInstance = (HINSTANCE)param;

	INITCOMMONCONTROLSEX icce;
	icce.dwSize = sizeof(icce);
	icce.dwICC = ICC_PROGRESS_CLASS;
	InitCommonControlsEx(&icce);

	MSG msg;
	while(GetMessage(&msg, NULL, 0, 0))
	{
		switch(msg.message)
		{
		case (WM_APP + 1):
			{
				HWND hDlg = CreateDialogW(thisInstance, MAKEINTRESOURCE(IDD_PROGRESS), NULL, ProgressDlgProc);
				SetWindowLongPtrW(hDlg, GWLP_USERDATA, (LONG_PTR)msg.lParam);
				WIMRegisterMessageCallback((HANDLE)msg.lParam, (FARPROC)WIMCallback, (LPVOID)hDlg);
				nWindowsOpen++;
			}
			break;
		case (WM_APP + 2):
			if(!EnumThreadWindows(GetCurrentThreadId(), KillProgressWindowProc, msg.lParam))
			{
				PostQuitMessage(0);
				threadID = 0;
			}
			break;
		default:
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			break;
		}
	}

	return 0;
}