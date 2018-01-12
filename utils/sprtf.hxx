#ifndef _SPRTF_HXX_
#define _SPRTF_HXX_

/*\
* sprtf.hxx
*
* Copyright (c) 2017 - Geoff R. McLane
* Licence: GNU GPL version 2
*
\*/

#ifdef   __cplusplus
extern "C" {
#endif

#ifndef EXPORT_FUNC
#define EXPORT_FUNC extern
#endif

#ifdef _MSC_VER
#  define MCDECL _cdecl
#else
#  define MCDECL
#endif

EXPORT_FUNC int add_std_out( int val );
EXPORT_FUNC int add_sys_time( int val );
EXPORT_FUNC int add_sys_date( int val );

EXPORT_FUNC int add_screen_out( int val );
EXPORT_FUNC int add_list_out( int val );
EXPORT_FUNC int add_append_log( int val );

EXPORT_FUNC int open_log_file( void );
EXPORT_FUNC void close_log_file( void );
EXPORT_FUNC void set_log_file( char * nf, int open );
EXPORT_FUNC char * get_log_file( void );

EXPORT_FUNC int MCDECL sprtf( const char *pf, ... );
#define M_MAX_SPRTF 2048
EXPORT_FUNC int direct_out_it( char *cp );

EXPORT_FUNC char *GetNxtBuf(void);

#define EndBuf(a)   ( a + strlen(a) )

EXPORT_FUNC char *get_date_stg(void);
EXPORT_FUNC char *get_time_stg(void);
EXPORT_FUNC char *get_date_time_stg(void);

#ifdef _MSC_VER
EXPORT_FUNC int gettimeofday(struct timeval *tp, void *tzp);
#endif

EXPORT_FUNC char *get_ctime_stg(time_t *pt);

#ifndef SPRTF
#define SPRTF sprtf
#endif

#ifdef   __cplusplus
}
#endif

#endif /* #ifndef _SPRTF_HXX_*/
/* eof - sprtf.hxx */
