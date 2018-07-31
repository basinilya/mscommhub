// cncreader.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdio.h>
#include <errno.h>
#include <Windows.h>
#include "mylogging.h"

//#define NO_DEBUG_MYPROG 1

#include "mylastheader.h"




static DWORD WINAPI reader_thread(LPVOID lpThreadParameter)
{
	HANDLE hComm = NULL;
	char buf[100];
	DWORD dw, nb;
	while (ReadFile(hComm, buf, 2, &nb, NULL)) {
		for (dw = 0; dw < nb; dw++) {
			printf("< %02X\n", buf[dw] & 0xFF);
			fflush(stdout);
		}
	}
	pWin32Error(ERR, "ReadFile() failed");
	return 0;
}

#undef FaiL
#define FaiL return -1

typedef struct cncport_t {
	HANDLE hComm;
	OVERLAPPED osReader;
	OVERLAPPED osWriter;
	BOOL readPending;
	BOOL writePending;
	DWORD remain;
	DWORD written;
	int index;
	const char *name;
	char buf[100];
} cncport_t;

static const char parity[] = "NOEMS";
static const char stops[][4] = { "1","1.5","2" };

struct ctx_t {
	cncport_t ports[100];
	HANDLE readEvents[100];
	HANDLE writeEvents[100];
	int portindex;
	int namewidth;
	DWORD nWaitObjects;
	BOOL trace;

	int port_open(const char *path)
	{
		int i;
		DCB dcb = { 0 };
		COMMTIMEOUTS timeouts = { 0 };

		if (sizeof(ports) / sizeof(ports[0]) <= nWaitObjects) {
			log(ERR, "too many ports");
			FaiL;
		}

		cncport_t *port = &ports[nWaitObjects];

		timeouts.ReadIntervalTimeout = MAXDWORD;
		timeouts.ReadTotalTimeoutMultiplier = MAXDWORD;
		timeouts.ReadTotalTimeoutConstant = MAXDWORD - 1;

		ZeroMemory(port, sizeof(*port));

		port->hComm = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
		if (port->hComm == INVALID_HANDLE_VALUE) {
			pWin32Error(ERR, "CreateFile(\"%s\") failed", path);
			FaiL;
		}

		if (!GetCommState(port->hComm, &dcb)) {
			pWin32Error(ERR, "GetCommState() failed");
			CloseHandle(port->hComm);
			FaiL;
		}

		log(INFO, "%s: %d,%c,%d,%s", path, dcb.BaudRate, parity[dcb.Parity], dcb.ByteSize, stops[dcb.StopBits]);

		timeouts.ReadIntervalTimeout = 1000 / dcb.BaudRate + 1;

		if (!SetCommTimeouts(port->hComm, &timeouts)) {
			pWin32Error(ERR, "SetCommTimeouts() failed");
			CloseHandle(port->hComm);
			FaiL;
		}

		port->osReader.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		readEvents[nWaitObjects] = port->osReader.hEvent;

		port->osWriter.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		writeEvents[nWaitObjects] = port->osWriter.hEvent;

		nWaitObjects++;

		port->index = portindex++;
		// snprintf(port->path, sizeof(port->path), "%s", path);
		port->name = strdup(path);
		i = strlen(port->name);
		namewidth = i > namewidth ? i : namewidth;
		
		return 0;
	}

	void write_complete(cncport_t *port, DWORD nb) {
		port->written += nb;
		port->remain -= nb;
	}

	void read_complete(cncport_t *port, DWORD nb) {
		BOOL havePendingWrites;
		DWORD dw;
		if (nb == 0) {
			return;
		}
		if (trace) {
			printf("< %*s:", namewidth, port->name);
			for (dw = 0; dw < nb; dw++) {
				printf(" %02X", port->buf[dw] & 0xFF);
			}
			printf("\n");
			fflush(stdout);
		}
		for (dw = 0; dw < nWaitObjects; dw++) {
			if (ports[dw].readPending) {
				CancelIo(ports[dw].hComm);
			}
			if (&ports[dw] != port) {
				ports[dw].remain = nb;
				ports[dw].written = 0;
			}
		}

		for (;;) {
			havePendingWrites = FALSE;
			for (dw = 0; dw < nWaitObjects; dw++) {
				if (ports[dw].writePending) {
					havePendingWrites = TRUE;
				}
				else {
					if (ports[dw].remain != 0) {
						if (!WriteFile(ports[dw].hComm, port->buf + ports[dw].written, ports[dw].remain, &nb, &ports[dw].osWriter)) {
							if (GetLastError() != ERROR_IO_PENDING) {
								pWin32Error(ERR, "WriteFile() failed");
								exit(1);
							}
							else {
								ports[dw].writePending = TRUE;
								havePendingWrites = TRUE;
							}
						}
						else {
							write_complete(&ports[dw], nb);
							ResetEvent(ports[dw].osWriter.hEvent);
						}
					}
				}
			}
			if (!havePendingWrites) {
				break;
			}
			dw = WaitForMultipleObjects(nWaitObjects, writeEvents, FALSE, INFINITE);
			if (!GetOverlappedResult(ports[dw].hComm, &ports[dw].osWriter, &nb, FALSE)) {
				pWin32Error(ERR, "WriteFile() failed");
				exit(1);
			}
			ports[dw].writePending = FALSE;
			if (nb != 0) {
				write_complete(&ports[dw], nb);
			}
		}
	}


int m(int argc, char *argv[]) {
	BOOL havePendingReads;
	DWORD dw, nb;
	int i;

	namewidth = 0;
	nWaitObjects = 0;
	trace = 0;

	for (i = 1; i < argc; i++) {
		if (0 != port_open(argv[i])) {
			return 1;
		}
	}

	if (nWaitObjects == 0) {
		log(ERR, "no ports opened");
		return 1;
	}

	for (;;) {
		havePendingReads = FALSE;
		for (dw = 0; dw < nWaitObjects; dw++) {
			if (ports[dw].readPending) {
				havePendingReads = TRUE;
			}
			else {
				nb = -1;
				SetLastError(0);
				if (!ReadFile(ports[dw].hComm, ports[dw].buf, sizeof(ports[dw].buf), &nb, &ports[dw].osReader)) {
					if (GetLastError() != ERROR_IO_PENDING) {
						pWin32Error(ERR, "ReadFile() failed");
						return 1;
					}
					else {
						ports[dw].readPending = TRUE;
						havePendingReads = TRUE;
					}
				}
				else {
					ResetEvent(ports[dw].osReader.hEvent);
					read_complete(&ports[dw], nb);
				}
			}
		}
		if (havePendingReads) {
			dw = WaitForMultipleObjects(nWaitObjects, readEvents, FALSE, INFINITE);
			nb = -1;
			SetLastError(0);
			if (!GetOverlappedResult(ports[dw].hComm, &ports[dw].osReader, &nb, FALSE)) {
				if (GetLastError() != ERROR_OPERATION_ABORTED) {
					pWin32Error(ERR, "ReadFile() failed");
					return 1;
				}
			}
			ports[dw].readPending = FALSE;
			read_complete(&ports[dw], nb);
		}
	}

	return 0;
}

}; // ctx_t

int main(int argc, char *argv[])
{
	ctx_t ctx;
	return ctx.m(argc, argv);
}
