/* sprtf.cxx
 * SPRTF - Log output utility - part of the project
 *
 * Copyright (c) 2017 - Geoff R. McLane
 *
 * Licence: GNU GPL version 2
 *
 */

#ifdef _MSC_VER
#  pragma warning( disable : 4995 )
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>  /* fopen()... */
#include <string.h> /* strcpy */
#include <stdarg.h> /* va_start, va_end, ... */

#ifdef _MSC_VER
#  include <WinSock2.h>
#  include <sys/timeb.h>
#  if (defined(UNICODE) || defined(_UNICODE))
#    include <Strsafe.h>
#  endif
#include <direct.h> // for _mkdir, ...
#else /* !_MSC_VER */
#  include <sys/time.h> /* gettimeoday(), struct timeval,... */
#endif /* _MSC_VER y/n */

#include <time.h>
#include <stdlib.h> /* for exit() in unix */
#ifdef _WIN32
#include <fcntl.h>
#include <io.h> // for setmode, ...
#endif // _WIN32
#include "sprtf.hxx"

#ifdef _MSC_VER
#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#endif /* #ifndef _CRT_SECURE_NO_DEPRECATE */
#pragma warning( disable:4996 )
#else
#define strcmpi strcasecmp
#endif 

#ifndef MX_ONE_BUF
#  define MX_ONE_BUF 1024
#endif
#ifndef MX_BUFFERS
#  define MX_BUFFERS 1024
#endif
#ifndef PATH_SEP
#ifdef _WIN32
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif
#endif

static char gszAppData[1024] = "\0";
static char _s_strbufs[MX_ONE_BUF * MX_BUFFERS];
static int iNextBuf = 0;

char *GetNxtBuf()
{
   iNextBuf++;
   if(iNextBuf >= MX_BUFFERS)
      iNextBuf = 0;
   return &_s_strbufs[MX_ONE_BUF * iNextBuf];
}

#define  MXIO     512
/* use local log */
static char def_log[] = "tempped.txt";
static char logfile[264] = "\0";
static FILE * outfile = NULL;
static int addsystime = 0;
static int addsysdate = 0;
static int addstdout = 1;
static int addflush = 1;
static int add2screen = 0;
static int add2listview = 0;
static int append_to_log = 0;

#ifndef VFP
#  define VFP(a) ( a && ( a != (FILE *)-1 ) )
#endif

int   add_list_out( int val )
{
   int i = add2listview;
   add2listview = val;
   return i;
}

int   add_std_out( int val )
{
   int i = addstdout;
   addstdout = val;
   return i;
}

int   add_screen_out( int val )
{
   int i = add2screen;
   add2screen = val;
   return i;
}


int   add_sys_time( int val )
{
   int   i = addsystime;
   addsystime = val;
   return i;
}

int   add_sys_date( int val )
{
   int   i = addsysdate;
   addsysdate = val;
   return i;
}


int   add_append_log( int val )
{
   int   i = append_to_log;
   append_to_log = val;
   return i;
}


#ifdef _MSC_VER
static const char *mode = "wb"; /* in window sprtf looks after the line endings */
#else
static const char *mode = "w";
#endif

#ifdef _WIN32
#define M_IS_DIR _S_IFDIR
#define mkdir _mkdir

#else // !_MSC_VER
#define M_IS_DIR S_IFDIR
#endif

#define MDT_NONE 0
#define	MDT_FILE 1
#define	MDT_DIR 2

static struct stat buf;

int is_file_or_directory(const char* path)
{
    if (!path)
        return MDT_NONE;
    if (stat(path, &buf) == 0)
    {
        if (buf.st_mode & M_IS_DIR)
            return MDT_DIR;
        else
            return MDT_FILE;
    }
    return MDT_NONE;
}

size_t get_last_file_size() { return buf.st_size; }


int create_dir(const char* pd)
{
    int iret = 1;
    int res;
    if (is_file_or_directory(pd) != MDT_DIR) {
        size_t i, j, len = strlen(pd);
        char ps, ch, pc = 0;
        char tmp[260];
        j = 0;
        iret = 0;
        tmp[0] = 0;
#ifdef _WIN32
        ps = '\\';
#else
        ps = '/'
#endif

            for (i = 0; i < len; i++) {
                ch = pd[i];
                if ((ch == '\\') || (ch == '/')) {
                    ch = ps;
                    if ((pc == 0) || (pc == ':')) {
                        tmp[j++] = ch;
                        tmp[j] = 0;
                    }
                    else {
                        tmp[j] = 0;
                        if (is_file_or_directory(tmp) != MDT_DIR) {
                            res = mkdir(tmp);
                            if (res != 0) {
                                return 0; // FAILED
                            }
                            if (is_file_or_directory(tmp) != MDT_DIR) {
                                return 0; // FAILED
                            }
                        }
                        tmp[j++] = ch;
                        tmp[j] = 0;
                    }
                }
                else {
                    tmp[j++] = ch;
                    tmp[j] = 0;
                }
                pc = ch;
            } // for lenght of path
        if ((pc == '\\') || (pc == '/')) {
            iret = 1; // success
        }
        else {
            if (j && pc) {
                tmp[j] = 0;
                if (is_file_or_directory(tmp) == MDT_DIR) {
                    iret = 1; // success
                }
                else {
                    res = mkdir(tmp);
                    if (res != 0) {
                        return 0; // FAILED
                    }
                    if (is_file_or_directory(tmp) != MDT_DIR) {
                        return 0; // FAILED
                    }
                    iret = 1; // success
                }
            }
        }
    }
    return iret;
}

#ifdef _WIN32
VOID  GetModulePath(LPTSTR lpb)
{
    LPTSTR   p;
    GetModuleFileName(NULL, lpb, 256);
    p = strrchr(lpb, '\\');
    if (p)
        p++;
    else
        p = lpb;
    *p = 0;
}
#endif

void GetAppData(PTSTR lpini)
{
    char* pd;
    if (!gszAppData[0]) {
        //pd = getenv("PROGRAMDATA"); // UGH - do not have permissions -  how to GET permissions
        //if (!pd) {
        //	pd = getenv("ALLUSERSPROFILE");
        //}
        pd = getenv("APPDATA");
        if (!pd) {
            pd = getenv("LOCALAPPDATA");
        }
        if (pd) {
            strcpy(gszAppData, pd);
            strcat(gszAppData, PATH_SEP);
            strcat(gszAppData, "pedump2");
            if (!create_dir(gszAppData)) {
                gszAppData[0] = 0;
            }
            else {
                strcat(gszAppData, PATH_SEP);
            }
        }
    }

    if (gszAppData[0]) {
        strcpy(lpini, gszAppData);
    }
    else {
#ifdef _WIN32
        GetModulePath(lpini);    // does GetModuleFileName( NULL, lpini, 256 );
#else
        strcpy(lpini, "./");
#endif
    }
}


int   open_log_file( void )
{
    if (logfile[0] == 0) 
    {
        GetAppData(logfile);
        strcat(logfile, def_log);
    }
   if (append_to_log) {
#ifdef _MSC_VER
        mode = "ab"; /* in window sprtf looks after the line endings */
#else
        mode = "a";
#endif
   }
   outfile = fopen(logfile, mode);
   if( outfile == 0 ) {
      outfile = (FILE *)-1;
      sprtf("ERROR: Failed to open log file [%s] ...\n", logfile);
      /* exit(1); failed */
      return 0;   /* failed */
   }
   else {
#ifdef _WIN32
       setmode(fileno(stdout), O_BINARY);
#endif // _WIN32
   }
   return 1; /* success */
}

void close_log_file( void )
{
   if( VFP(outfile) ) {
      fclose(outfile);
   }
   outfile = NULL;
}

char * get_log_file( void )
{
   if (logfile[0] == 0)
      strcpy(logfile,def_log);
   if (outfile == (FILE *)-1) /* disable the log file */
       return (char *)"none";
   return logfile;
}

void   set_log_file( char * nf, int open )
{
   if (logfile[0] == 0)
      strcpy(logfile,def_log);
   if ( nf && *nf && strcmpi(nf,logfile) ) {
      close_log_file(); /* remove any previous */
      strcpy(logfile,nf); /* set new name */
      if (strcmp(logfile,"none") == 0) { /* if equal 'none' */
          outfile = (FILE *)-1; /* disable the log file */
      } else if (open) {
          open_log_file();  /* and open it ... anything previous written is 'lost' */
      } else
          outfile = 0; /* else set 0 to open on first write */
   }
}

#ifdef _MSC_VER
int gettimeofday(struct timeval *tp, void *tzp)
{
#ifdef WIN32
    struct _timeb timebuffer;
    _ftime(&timebuffer);
    tp->tv_sec = (long)timebuffer.time;
    tp->tv_usec = timebuffer.millitm * 1000;
#else
    tp->tv_sec = time(NULL);
    tp->tv_usec = 0;
#endif
    return 0;
}

#endif /* _MSC_VER */

void add_date_stg( char *ps, struct timeval *ptv )
{
    time_t curtime;
    struct tm * ptm;
    curtime = (ptv->tv_sec & 0xffffffff);
    ptm = localtime(&curtime);
    if (ptm) {
        strftime(EndBuf(ps),128,"%Y/%m/%d",ptm);
    }
}

void add_time_stg( char *ps, struct timeval *ptv )
{
    time_t curtime;
    struct tm * ptm;
    curtime = (ptv->tv_sec & 0xffffffff);
    ptm = localtime(&curtime);
    if (ptm) {
        strftime(EndBuf(ps),128,"%H:%M:%S",ptm);
    }
}

char *get_date_stg()
{
    char *ps;
    struct timeval tv;
    gettimeofday( (struct timeval *)&tv, (struct timezone *)0 );
    ps = GetNxtBuf();
    *ps = 0;
    add_date_stg( ps, &tv );
    return ps;
}

char *get_time_stg()
{
    char *ps;
    struct timeval tv;
    gettimeofday( (struct timeval *)&tv, (struct timezone *)0 );
    ps = GetNxtBuf();
    *ps = 0;
    add_time_stg( ps, &tv );
    return ps;
}

char *get_date_time_stg()
{
    char *ps;
    struct timeval tv;
    gettimeofday( (struct timeval *)&tv, (struct timezone *)0 );
    ps = GetNxtBuf();
    *ps = 0;
    add_date_stg( ps, &tv );
    strcat(ps," ");
    add_time_stg( ps, &tv );
    return ps;
}

static void oi( char * psin )
{
    int len, w;
    char * ps = psin;
    if (!ps)
        return;

   len = (int)strlen(ps);
   if (len) {

      if( outfile == 0 ) {
         open_log_file();
      }
      if( VFP(outfile) ) {
          char *tb;
          if (addsysdate) {
              tb = GetNxtBuf();
              len = sprintf( tb, "%s - %s", get_date_time_stg(), ps );
              ps = tb;
          } else if( addsystime ) {
              tb = GetNxtBuf();
              len = sprintf( tb, "%s - %s", get_time_stg(), ps );
              ps = tb;
          }

         w = (int)fwrite( ps, 1, len, outfile );
         if( w != len ) {
            fclose(outfile);
            outfile = (FILE *)-1;
            sprtf("WARNING: Failed write to log file [%s] ...\n", logfile);
            exit(1);
         } else if (addflush) {
            fflush( outfile );
         }
      }

      if( addstdout ) {
         // fwrite( ps, 1, len, stderr );  /* 20170917 - Switch to using 'stderr' in place of 'stdout' */
         fwrite(ps, 1, len, stdout);  /* 20180117 - Switch back to using 'stdout' in place of 'stderr' */
      }
#ifdef ADD_LISTVIEW
       if (add2listview) {
           LVInsertItem(ps);
       } 
#endif /* ADD_LISTVIEW */
#ifdef ADD_SCREENOUT
       if (add2screen) {
          Add_String(ps);    /* add string to screen list */
       }
#endif /* #ifdef ADD_SCREENOUT */
   }
}

#ifdef _MSC_VER
/* service to ensure line endings in windows only */
static void prt( char * ps )
{
    static char _s_buf[1024];
    char * pb = _s_buf;
    size_t i, j, k;
    char   c, d;
    i = strlen(ps);
    k = 0;
    d = 0;
    if(i) {
        k = 0;
        d = 0;
        for( j = 0; j < i; j++ ) {
            c = ps[j];
            if( c == 0x0d ) {
                if( (j+1) < i ) {
                    if( ps[j+1] != 0x0a ) {
                        pb[k++] = c;
                        c = 0x0a;
                    }
            } else {
                    pb[k++] = c;
                    c = 0x0a;
                }
            } else if( c == 0x0a ) {
                if( d != 0x0d ) {
                    pb[k++] = 0x0d;
                }
            }
            pb[k++] = c;
            d = c;
            if( k >= MXIO ) {
                pb[k] = 0;
                oi(pb);
                k = 0;
            }
        }   /* for length of string */
        if( k ) {
            pb[k] = 0;
            oi( pb );
        }
    }
}
#endif /* #ifdef _MSC_VER */

int direct_out_it( char *cp )
{
#ifdef _MSC_VER
    prt(cp);
#else
    oi(cp);
#endif
    return (int)strlen(cp);
}

/* STDAPI StringCchVPrintf( OUT LPTSTR  pszDest,
 *   IN  size_t  cchDest, IN  LPCTSTR pszFormat, IN  va_list argList ); */
int MCDECL sprtf( const char *pf, ... )
{
   static char _s_sprtfbuf[M_MAX_SPRTF+4];
   char * pb = _s_sprtfbuf;
   int   i;
   va_list arglist;
   va_start(arglist, pf);
   i = vsnprintf( pb, M_MAX_SPRTF, pf, arglist );
   va_end(arglist);
#ifdef _MSC_VER
   prt(pb); /* ensure CR/LF */
#else
   oi(pb);
#endif
   return i;
}

#ifdef UNICODE
/* WIDE VARIETY */
static void wprt( PTSTR ps )
{
   static char _s_woibuf[1024];
   char * cp = _s_woibuf;
   int len = (int)lstrlen(ps);
   if(len) {
      int ret = WideCharToMultiByte( CP_ACP, /* UINT CodePage, // code page */
         0, /* DWORD dwFlags,            // performance and mapping flags */
         ps,   /* LPCWSTR lpWideCharStr,    // wide-character string */
         len,     /* int cchWideChar,          // number of chars in string. */
         cp,      /* LPSTR lpMultiByteStr,     // buffer for new string */
         1024,    /* int cbMultiByte,          // size of buffer */
         NULL,    /* LPCSTR lpDefaultChar,     // default for unmappable chars */
         NULL );  /* LPBOOL lpUsedDefaultChar  // set when default char used */
      /* oi(cp); */
      prt(cp);
   }
}

int MCDECL wsprtf( PTSTR pf, ... )
{
   static WCHAR _s_sprtfwbuf[1024];
   PWSTR pb = _s_sprtfwbuf;
   int   i = 1;
   va_list arglist;
   va_start(arglist, pf);
   *pb = 0;
   StringCchVPrintf(pb,1024,pf,arglist);
   va_end(arglist);
   wprt(pb);
   return i;
}

#endif /* #ifdef UNICODE */

char *get_ctime_stg(time_t *pt)
{
    char *pts = ctime(pt);
    char *nb = GetNxtBuf();
    if (pts) {
        strcpy(nb, pts);
        size_t len = strlen(nb);
        while (len) {
            len--;
            if (nb[len] > 0x20)
                break;
            nb[len] = 0;
        }
    }
    else {
        sprintf(nb, "Invalid time %llu", *pt);
    }
    return nb;
}

/* eof - sprtf.c */
