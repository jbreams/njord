/****************************************************************************\

    WIMGAPI.H

    Copyright (c) Microsoft Corporation.
    All rights reserved.

\****************************************************************************/

#ifndef _WIMGAPI_H_
#define _WIMGAPI_H_

#ifdef __cplusplus
extern "C" {
#endif

//
// Defined Value(s):
//

// WIMCreateFile:
//
#define WIM_GENERIC_READ            GENERIC_READ
#define WIM_GENERIC_WRITE           GENERIC_WRITE

#define WIM_CREATE_NEW              CREATE_NEW
#define WIM_CREATE_ALWAYS           CREATE_ALWAYS
#define WIM_OPEN_EXISTING           OPEN_EXISTING
#define WIM_OPEN_ALWAYS             OPEN_ALWAYS

typedef enum
{
    WIM_COMPRESS_NONE = 0,
    WIM_COMPRESS_XPRESS,
    WIM_COMPRESS_LZX
};

typedef enum
{
    WIM_CREATED_NEW = 0,
    WIM_OPENED_EXISTING
};

// WIMCreateFile, WIMCaptureImage, WIMApplyImage flags:
//
#define WIM_FLAG_RESERVED           0x00000001
#define WIM_FLAG_VERIFY             0x00000002
#define WIM_FLAG_INDEX              0x00000004
#define WIM_FLAG_NO_APPLY           0x00000008
#define WIM_FLAG_NO_DIRACL          0x00000010
#define WIM_FLAG_NO_FILEACL         0x00000020
#define WIM_FLAG_SHARE_WRITE        0x00000040
#define WIM_FLAG_FILEINFO           0x00000080
#define WIM_FLAG_NO_RP_FIX          0x00000100  // do not fix up reparse point tag
                                                // only used in WIMCaptureImage

// WIMSetReferenceFile
//
#define WIM_REFERENCE_APPEND        0x00010000
#define WIM_REFERENCE_REPLACE       0x00020000

// WIMExportImage
//
#define WIM_EXPORT_ALLOW_DUPLICATES 0x00000001
#define WIM_EXPORT_ONLY_RESOURCES   0x00000002
#define WIM_EXPORT_ONLY_METADATA    0x00000004

// WIMRegisterMessageCallback:
//
#define INVALID_CALLBACK_VALUE      0xFFFFFFFF

// WIMCopyFile
//
#define WIM_COPY_FILE_RETRY         0x01000000

// WIMMessageCallback Notifications:
//
typedef enum
{
    WIM_MSG = WM_APP + 0x1476,
    WIM_MSG_TEXT,
    WIM_MSG_PROGRESS,
    WIM_MSG_PROCESS,
    WIM_MSG_SCANNING,
    WIM_MSG_SETRANGE,
    WIM_MSG_SETPOS,
    WIM_MSG_STEPIT,
    WIM_MSG_COMPRESS,
    WIM_MSG_ERROR,
    WIM_MSG_ALIGNMENT,
    WIM_MSG_RETRY,
    WIM_MSG_SPLIT,
    WIM_MSG_FILEINFO,
    WIM_MSG_INFO,
    WIM_MSG_WARNING,
    WIM_MSG_CHK_PROCESS
};

//
// WIMMessageCallback Return codes:
//
#define WIM_MSG_SUCCESS     ERROR_SUCCESS
#define WIM_MSG_DONE        0xFFFFFFF0
#define WIM_MSG_SKIP_ERROR  0xFFFFFFFE
#define WIM_MSG_ABORT_IMAGE 0xFFFFFFFF

//
// WIM_INFO dwFlags values:
//
#define WIM_ATTRIBUTE_NORMAL        0x00000000
#define WIM_ATTRIBUTE_RESOURCE_ONLY 0x00000001
#define WIM_ATTRIBUTE_METADATA_ONLY 0x00000002
#define WIM_ATTRIBUTE_VERIFY_DATA   0x00000004
#define WIM_ATTRIBUTE_RP_FIX        0x00000008
#define WIM_ATTRIBUTE_SPANNED       0x00000010
#define WIM_ATTRIBUTE_READONLY      0x00000020

//
// The WIM_INFO structure used by WIMGetAttributes:
//
typedef struct _WIM_INFO
{
    WCHAR  WimPath[MAX_PATH];
    GUID   Guid;
    DWORD  ImageCount;
    DWORD  CompressionType;
    USHORT PartNumber;
    USHORT TotalParts;
    DWORD  BootIndex;
    DWORD  WimAttributes;
    DWORD  WimFlagsAndAttr;
}
WIM_INFO, *PWIM_INFO, *LPWIM_INFO;

//
// The WIM_MOUNT_LIST structure used for getting the list of mounted images.
//
typedef struct _WIM_MOUNT_LIST
{
    WCHAR  WimPath[MAX_PATH];
    WCHAR  MountPath[MAX_PATH];
    DWORD  ImageIndex;
    BOOL   MountedForRW;
}
WIM_MOUNT_LIST, *PWIM_MOUNT_LIST, *LPWIM_MOUNT_LIST;

//
// Naming translations for static library use:
//
#ifdef WIM_STATIC_LIB
#define WIMCreateFile                     _WIMCreateFile
#define WIMCloseHandle                    _WIMCloseHandle
#define WIMSetTemporaryPath               _WIMSetTemporaryPath
#define WIMSetReferenceFile               _WIMSetReferenceFile
#define WIMSplitFile                      _WIMSplitFile
#define WIMExportImage                    _WIMExportImage
#define WIMDeleteImage                    _WIMDeleteImage
#define WIMGetImageCount                  _WIMGetImageCount
#define WIMGetAttributes                  _WIMGetAttributes
#define WIMSetBootImage                   _WIMSetBootImage
#define WIMLoadImage                      _WIMLoadImage
#define WIMApplyImage                     _WIMApplyImage
#define WIMCaptureImage                   _WIMCaptureImage
#define WIMGetImageInformation            _WIMGetImageInformation
#define WIMSetImageInformation            _WIMSetImageInformation
#define WIMGetMessageCallbackCount        _WIMGetMessageCallbackCount
#define WIMRegisterMessageCallback        _WIMRegisterMessageCallback
#define WIMUnregisterMessageCallback      _WIMUnregisterMessageCallback
#define WIMMountImage                     _WIMMountImage
#define WIMUnmountImage                   _WIMUnmountImage
#define WIMGetMountedImages               _WIMGetMountedImages
#define WIMCopyFile                       _WIMCopyFile
#define WIMInitFileIOCallbacks            _WIMInitFileIOCallbacks
#define WIMSetFileIOCallbackTemporaryPath _WIMSetFileIOCallbackTemporaryPath
#endif

//
// Exported Function Prototypes:
//
HANDLE
WINAPI
WIMCreateFile(
    __in      LPWSTR  lpszWimPath,
    IN        DWORD   dwDesiredAccess,
    IN        DWORD   dwCreationDisposition,
    IN        DWORD   dwFlagsAndAttributes,
    IN        DWORD   dwCompressionType,
    __out_opt LPDWORD lpdwCreationResult
    );

BOOL
WINAPI
WIMCloseHandle(
    __in HANDLE hObject
    );

BOOL
WINAPI
WIMSetTemporaryPath(
    __in HANDLE hWim,
    __in LPWSTR lpszPath
    );

BOOL
WINAPI
WIMSetReferenceFile(
    __in HANDLE hWim,
    __in LPWSTR lpszPath,
    IN   DWORD  dwFlags
    );

BOOL
WINAPI
WIMSplitFile(
    __in    HANDLE         hWim,
    __in    LPWSTR         lpszPartPath,
    __inout PLARGE_INTEGER pliPartSize,
    IN      DWORD          dwFlags
    );

BOOL
WINAPI
WIMExportImage(
    __in HANDLE hImage,
    __in HANDLE hWim,
    IN   DWORD  dwFlags
    );

BOOL
WINAPI
WIMDeleteImage(
    __in HANDLE hWim,
    IN   DWORD  dwImageIndex
    );

DWORD
WINAPI
WIMGetImageCount(
    __in HANDLE hWim
    );

BOOL
WINAPI
WIMGetAttributes(
    __in                    HANDLE     hWim,
    __out_bcount(cbWimInfo) LPWIM_INFO lpWimInfo,
    IN                      DWORD      cbWimInfo
    );

BOOL
WINAPI
WIMSetBootImage(
    __in HANDLE hWim,
    __in DWORD  dwImageIndex
    );

HANDLE
WINAPI
WIMCaptureImage(
    __in HANDLE hWim,
    __in LPWSTR lpszPath,
    IN   DWORD  dwCaptureFlags
    );

HANDLE
WINAPI
WIMLoadImage(
    __in HANDLE hWim,
    IN   DWORD  dwImageIndex
    );

BOOL
WINAPI
WIMApplyImage(
    __in HANDLE hImage,
    __in LPWSTR lpszPath,
    IN   DWORD  dwApplyFlags
    );

BOOL
WINAPI
WIMGetImageInformation(
    __in  HANDLE  hImage,
    __out LPVOID  *lplpvImageInfo,
    __out LPDWORD lpcbImageInfo
    );

BOOL
WINAPI
WIMSetImageInformation(
    __in                     HANDLE hImage,
    __in_bcount(cbImageInfo) LPVOID lpvImageInfo,
    IN                       DWORD  cbImageInfo
    );

DWORD
WINAPI
WIMGetMessageCallbackCount(
    __in_opt HANDLE hWim
    );

DWORD
WINAPI
WIMRegisterMessageCallback(
    __in_opt HANDLE  hWim,
    __in     FARPROC fpMessageProc,
    __in_opt LPVOID  lpvUserData
    );

BOOL
WINAPI
WIMUnregisterMessageCallback(
    __in_opt HANDLE  hWim,
    __in_opt FARPROC fpMessageProc
    );

DWORD
WINAPI
WIMMessageCallback(
    IN DWORD  dwMessageId,
    IN WPARAM wParam,
    IN LPARAM lParam,
    IN LPVOID lpvUserData
    );

BOOL
WINAPI
WIMCopyFile(
    __in     LPWSTR             lpszExistingFileName,
    __in     LPWSTR             lpszNewFileName,
    __in_opt LPPROGRESS_ROUTINE lpProgressRoutine,
    __in_opt LPVOID             lpvData,
    __in_opt LPBOOL             pbCancel,
    IN       DWORD              dwCopyFlags
    );

BOOL
WINAPI
WIMMountImage(
    __in     LPWSTR lpszMountPath,
    __in     LPWSTR lpszWimFileName,
    __in     DWORD  dwImageIndex,
    __in_opt LPWSTR lpszTempPath
    );

BOOL
WINAPI
WIMUnmountImage(
    __in     LPWSTR lpszMountPath,
    __in_opt LPWSTR lpszWimFileName,
    __in     DWORD  dwImageIndex,
    __in     BOOL   bCommitChanges
    );

BOOL
WINAPI
WIMGetMountedImages(
    __inout_bcount(*lpcbBufferSize) LPWIM_MOUNT_LIST lpMountList,
    __inout                         LPDWORD          lpcbBufferSize
    );

BOOL
WINAPI
WIMInitFileIOCallbacks(
    __in_opt LPVOID lpCallbacks
    );

BOOL
WINAPI
WIMSetFileIOCallbackTemporaryPath(
    __in_opt LPTSTR lpszPath
    );

//
// File I/O callback prototypes
//
typedef VOID * PFILEIOCALLBACK_SESSION;

typedef
PFILEIOCALLBACK_SESSION
(CALLBACK * FileIOCallbackOpenFile)(
    LPCWSTR lpFileName
    );

typedef
BOOL
(CALLBACK * FileIOCallbackCloseFile)(
    PFILEIOCALLBACK_SESSION hFile
    );

typedef
BOOL
(CALLBACK * FileIOCallbackReadFile)(
    PFILEIOCALLBACK_SESSION hFile,
    LPVOID lpBuffer,
    DWORD nNumberOfBytesToRead,
    LPDWORD lpNumberOfBytesRead,
    LPOVERLAPPED lpOverlapped
    );

typedef
BOOL
(CALLBACK * FileIOCallbackSetFilePointer)(
    PFILEIOCALLBACK_SESSION hFile,
    LARGE_INTEGER liDistanceToMove,
    PLARGE_INTEGER lpNewFilePointer,
    DWORD dwMoveMethod
    );

typedef
BOOL
(CALLBACK * FileIOCallbackGetFileSize)(
    HANDLE hFile,
    PLARGE_INTEGER lpFileSize
    );

typedef struct _SFileIOCallbackInfo
{
    FileIOCallbackOpenFile       pfnOpenFile;
    FileIOCallbackCloseFile      pfnCloseFile;
    FileIOCallbackReadFile       pfnReadFile;
    FileIOCallbackSetFilePointer pfnSetFilePointer;
    FileIOCallbackGetFileSize    pfnGetFileSize;
} SFileIOCallbackInfo;

#ifdef __cplusplus
}
#endif

#endif // _WIMGAPI_H_
