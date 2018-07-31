#include <windows.h>
#include "mylogging.h"

#define NO_DEBUG_MYPROG
#include "mylastheader.h"

void dbg_LookupPrivilegeValue(LPCTSTR lpSystemName, LPCTSTR lpName, PLUID   lpLuid)
{
	if (!LookupPrivilegeValue(lpSystemName, lpName, lpLuid)) {
		pWin32Error(ERR, "LookupPrivilegeValue() failed");
		abort();
	}
}
void dbg_SetFilePointerEx(HANDLE hFile, LARGE_INTEGER liDistanceToMove, PLARGE_INTEGER lpNewFilePointer, DWORD dwMoveMethod)
{
	if (!SetFilePointerEx(hFile, liDistanceToMove, lpNewFilePointer, dwMoveMethod)) {
		pWin32Error(ERR, "SetFilePointerEx() failed");
		abort();
	}
}
void dbg_CloseHandle(const char *file, int line, HANDLE hObject) {
	if (!CloseHandle(hObject)) {
		pWin32Error(ERR, "CloseHandle() failed at %s:%d", file, line);
		abort();
	}
}

void dbg_WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped)
{
	if (!WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped)) {
		pWin32Error(ERR, "WriteFile() failed");
		abort();
	}
}
void dbg_ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped)
{
	if (!ReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped)) {
		pWin32Error(ERR, "ReadFile() failed");
		abort();
	}
}

void dbg_CancelIo(HANDLE handle) {
	if (!CancelIo(handle)) {
		pWin32Error(ERR, "CancelIo() failed");
		abort();
	}
}

/*
void dbg_GetOverlappedResult(HANDLE hFile, LPOVERLAPPED lpOverlapped, LPDWORD lpNumberOfBytesTransferred, BOOL bWait) {
	if (!GetOverlappedResult(hFile, lpOverlapped, lpNumberOfBytesTransferred, bWait)) {
		pWin32Error(ERR, "GetOverlappedResult() failed");
		abort();
	}
}
*/

DWORD dbg_WaitForMultipleObjects(DWORD nCount, const HANDLE *lpHandles, BOOL bWaitAll, DWORD dwMilliseconds) {
	DWORD res = WaitForMultipleObjects(nCount, lpHandles, bWaitAll, dwMilliseconds);
	if (res >= nCount) {
		pWin32Error(ERR, "WaitForMultipleObjects() failed");
		abort();
	}
}

HANDLE dbg_CreateEvent(
	LPSECURITY_ATTRIBUTES lpEventAttributes,
	BOOL                  bManualReset,
	BOOL                  bInitialState,
	LPCSTR                lpName
)
{
	HANDLE res = CreateEvent(lpEventAttributes, bManualReset, bInitialState, lpName);
	if (!res) {
		pWin32Error(ERR, "CreateEvent() failed");
		abort();
	}
	return res;
}