#ifndef _CLIPTUN_LOGGING_H
#define _CLIPTUN_LOGGING_H

#ifdef __cplusplus
extern "C" {
#endif

#define MYPROG_APPNAME "winzerofree"

#define MYPROG_LOG_TRACE 100
#define MYPROG_LOG_DEBUG 200
#define MYPROG_LOG_MESSAGE 300
#define MYPROG_LOG_WARNING 400
#define MYPROG_LOG_ERROR 500

#ifdef _UNICODE
#define FMT_S "%S"
#else
#define FMT_S "%s"
#endif

extern int myprog_loglevel;

void myprog_pSysError(int lvl, char const *fmt, ...);
void myprog_pWinsockError(int lvl, char const *fmt, ...);
void myprog_pWin32Error(int lvl, char const *fmt, ...);
void myprog_log(int lvl, char const *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
