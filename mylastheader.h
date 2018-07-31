/* local */
#undef DBG
#undef INFO
#undef WARN
#undef ERR
#define DBG MYPROG_LOG_DEBUG
#define INFO MYPROG_LOG_MESSAGE
#define WARN MYPROG_LOG_WARNING
#define ERR MYPROG_LOG_ERROR

#define pSysError     myprog_pSysError
#define pWinsockError myprog_pWinsockError
#define pWin32Error   myprog_pWin32Error
#define log           myprog_log

#define COUNTOF(a) (sizeof(a) / sizeof(a[0]))

#ifdef __cplusplus
extern "C" {
#endif

void dbg_OpenProcessToken(HANDLE ProcessHandle, DWORD DesiredAccess, PHANDLE TokenHandle);
void dbg_LookupPrivilegeValue(LPCTSTR lpSystemName, LPCTSTR lpName, PLUID   lpLuid);
void dbg_AdjustTokenPrivileges(HANDLE TokenHandle, BOOL DisableAllPrivileges, PTOKEN_PRIVILEGES NewState, DWORD BufferLength, PTOKEN_PRIVILEGES PreviousState, PDWORD ReturnLength);
void dbg_SetFilePointerEx(HANDLE hFile, LARGE_INTEGER liDistanceToMove, PLARGE_INTEGER lpNewFilePointer, DWORD dwMoveMethod);
void dbg_CloseHandle(const char *file, int line, HANDLE hObject);
void dbg_WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
void dbg_ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);
void dbg_CancelIo(HANDLE handle);
//void dbg_GetOverlappedResult(HANDLE hFile, LPOVERLAPPED lpOverlapped, LPDWORD lpNumberOfBytesTransferred, BOOL bWait);
HANDLE dbg_CreateEvent(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCSTR lpName);
DWORD dbg_WaitForMultipleObjects(DWORD nCount, const HANDLE *lpHandles, BOOL bWaitAll, DWORD dwMilliseconds);

#ifdef __cplusplus
}
#endif

#ifndef NO_DEBUG_MYPROG

#undef LookupPrivilegeValue
#define LookupPrivilegeValue dbg_LookupPrivilegeValue
#define SetFilePointerEx dbg_SetFilePointerEx
#define CloseHandle(hObject) dbg_CloseHandle(__FILE__, __LINE__, hObject)
#undef CreateEvent
#define CreateEvent dbg_CreateEvent
//#define WriteFile dbg_WriteFile
//#define ReadFile dbg_ReadFile
#define CancelIo dbg_CancelIo
//#define GetOverlappedResult dbg_GetOverlappedResult
#define WaitForMultipleObjects dbg_WaitForMultipleObjects

#endif
