/*\
 * utils.hxx
 *
 * Copyright (c) 2014 - Geoff R. McLane
 * Licence: GNU GPL version 2
 *
\*/

#ifndef _UTILS_HXX_
#define _UTILS_HXX_

#ifdef _MSC_VER
#define M_IS_DIR _S_IFDIR
#else // !_MSC_VER
#define M_IS_DIR S_IFDIR
#endif

enum DiskType {
    MDT_NONE,
    MDT_FILE,
    MDT_DIR
};

extern DiskType is_file_or_directory32 ( const char * path );
extern size_t get_last_file_size32();
extern time_t get_last_file_time32(); // { return buf.st_mtime; }

#ifdef _MSC_VER
extern DiskType is_file_or_directory64 ( const char * path );
extern __int64 get_last_file_size64();
extern time_t get_last_file_time64(); // { return buf64.st_mtime; }
#endif

extern char *double_to_stg( double d );
extern void trim_float_buf( char *pb );
extern char *get_I64u_Stg( unsigned long long val );
extern char *get_I64_Stg( long long val );
extern char *get_k_num( long long i64, int type = 0, int dotrim = 1 );
extern char *get_nice_number64( long long num, size_t width = 0 );
extern char *get_nice_number64u( unsigned long long num, size_t width = 0 );

extern void nice_num( char * dst, char * src ); // get nice number, with commas

extern double get_seconds();
char *get_elapsed_stg( double start );

#endif // #ifndef _UTILS_HXX_
// eof - utils.hxx
