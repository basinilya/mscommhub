#include "stdafx.h"

#include <stdio.h>
#include <errno.h>
#include <Windows.h>
#include "mylogging.h"

//#define NO_DEBUG_MYPROG 1

#include "mylastheader.h"

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

	int port_open(const char *path)
	{
		int i;
		DCB dcb = { 0 };
		DWORD dwModemStatus;
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

		if (!GetCommModemStatus(port->hComm, &dwModemStatus)) {
			pWin32Error(ERR, "GetCommModemStatus() failed");
			CloseHandle(port->hComm);
			FaiL;
		}

		
		dcb.fInX = 0;
		dcb.fOutX = 0;

		log(INFO, "%s: %d,%c,%d,%s", path, dcb.BaudRate, parity[dcb.Parity], dcb.ByteSize, stops[dcb.StopBits]);
		log(INFO, "fParity:%d, fOutxCtsFlow:%d, fOutxDsrFlow:%d, fDtrControl:%d, fDsrSensitivity:%d"
			", fTXContinueOnXoff:%d, fOutX:%d, fInX:%d, fErrorChar:%02X, fNull:%d, fRtsControl:%d"
			", fAbortOnError:%d, XonLim:%d, XoffLim:%d"
			", XonChar:%02X, XoffChar:%02X, ErrorChar:%02X, EofChar:%02X, EvtChar:%02X"
			, dcb.fParity, dcb.fOutxCtsFlow, dcb.fOutxDsrFlow, dcb.fDtrControl, dcb.fDsrSensitivity
			, dcb.fTXContinueOnXoff, dcb.fOutX, dcb.fInX, dcb.fErrorChar, dcb.fNull, dcb.fRtsControl
			, dcb.fAbortOnError, dcb.XonLim, dcb.XoffLim
			, dcb.XonChar & 0xFF, dcb.XoffChar & 0xFF, dcb.ErrorChar & 0xFF, dcb.EofChar & 0xFF, dcb.EvtChar & 0xFF);

		if (!SetCommState(port->hComm, &dcb)) {
			pWin32Error(ERR, "SetCommState() failed");
			CloseHandle(port->hComm);
			FaiL;
		}

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
		ResetEvent(port->osWriter.hEvent);
		if (nb == 0) {
			log(DBG, "write timeout on port %s", port->name);
			return;
		}
		if (myprog_loglevel <= TRACE) {
			log(TRACE, "wrote %u bytes on port %s", nb, port->name);
		}
		port->written += nb;
		port->remain -= nb;
	}

	void read_complete(cncport_t *port, DWORD nb) {
		BOOL havePendingWrites;
		DWORD dw;
		DWORD npending;
		if (nb == 0) {
			log(DBG, "read cancel or timeout on port %s", port->name);
			return;
		}
		if (myprog_loglevel <= TRACE) {
			log(TRACE, "got %u bytes on port %s", nb, port->name);
			fprintf(stderr, "< %*s:", namewidth, port->name);
			for (dw = 0; dw < nb; dw++) {
				fprintf(stderr, " %02X", port->buf[dw] & 0xFF);
			}
			fprintf(stderr, "\n");
			fflush(stderr);
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
			npending = 0;
			for (dw = 0; dw < nWaitObjects; dw++) {
				if (ports[dw].writePending) {
					havePendingWrites = TRUE;
					npending++;
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
								npending++;
							}
						}
						else {
							write_complete(&ports[dw], nb);
						}
					}
				}
			}
			if (!havePendingWrites) {
				break;
			}
			log(TRACE, "waiting completion on %u of %u ports...", npending, nWaitObjects);
			dw = WaitForMultipleObjects(nWaitObjects, writeEvents, FALSE, INFINITE);
			if (!GetOverlappedResult(ports[dw].hComm, &ports[dw].osWriter, &nb, FALSE)) {
				pWin32Error(ERR, "overlapped WriteFile() failed");
				exit(1);
			}
			ports[dw].writePending = FALSE;
			write_complete(&ports[dw], nb);
		}
	}


int m(int argc, char *argv[]) {
	BOOL havePendingReads;
	DWORD npending;
	DWORD dw, nb;
	int i;

	namewidth = 0;
	nWaitObjects = 0;
	myprog_loglevel = TRACE;

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
		npending = 0;
		for (dw = 0; dw < nWaitObjects; dw++) {
			if (ports[dw].readPending) {
				havePendingReads = TRUE;
				npending++;
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
						npending++;
					}
				}
				else {
					ResetEvent(ports[dw].osReader.hEvent);
					read_complete(&ports[dw], nb);
				}
			}
		}
		if (havePendingReads) {
			log(TRACE, "waiting data on %u of %u ports...", npending, nWaitObjects);
			dw = dbg_WaitForMultipleObjects(nWaitObjects, readEvents, FALSE, INFINITE);
			nb = -1;
			SetLastError(0);
			if (!GetOverlappedResult(ports[dw].hComm, &ports[dw].osReader, &nb, FALSE)) {
				if (GetLastError() != ERROR_OPERATION_ABORTED) {
					pWin32Error(ERR, "overlapped ReadFile() failed");
					return 1;
				}
				else {
					log(TRACE, "read cancelled earlier on port %s", ports[dw].name);
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

