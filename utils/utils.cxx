/*\
 * utils.cxx
 *
 * Copyright (c) 2014 - Geoff R. McLane
 * Licence: GNU GPL version 2
 *
\*/

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h> // for strlen(), ...
#ifdef _MSC_VER
#include <WinSock2.h>
#include <sys/timeb.h>
#else
#include <sys/time.h> // gettimeofday(), ...
#endif

#include "sprtf.hxx"
#include "utils.hxx"

static const char *module = "utils";

static struct stat buf;
DiskType is_file_or_directory32 ( const char * path )
{
    if (!path)
        return MDT_NONE;
	if (stat(path,&buf) == 0)
	{
		if (buf.st_mode & M_IS_DIR)
			return MDT_DIR;
		else
			return MDT_FILE;
	}
	return MDT_NONE;
}

size_t get_last_file_size32() { return buf.st_size; }
time_t get_last_file_time32() { return buf.st_mtime; }
#ifdef _MSC_VER
static struct _stat64 buf64;
DiskType is_file_or_directory64 ( const char * path )
{
    if (!path)
        return MDT_NONE;
	if (_stat64(path,&buf64) == 0)
	{
		if (buf64.st_mode & M_IS_DIR)
			return MDT_DIR;
		else
			return MDT_FILE;
	}
	return MDT_NONE;
}

__int64 get_last_file_size64() { return buf64.st_size; }
time_t get_last_file_time64() { return buf64.st_mtime; }
#endif
#ifndef STRLEN
#define STRLEN strlen
#endif

void trim_float_buf( char *pb )
{
   size_t len = STRLEN(pb);
   size_t i, dot, zcnt;
   for( i = 0; i < len; i++ )
   {
      if( pb[i] == '.' )
         break;
   }
   dot = i + 1; // keep position of decimal point (a DOT)
   zcnt = 0;
   for( i = dot; i < len; i++ )
   {
      if( pb[i] != '0' )
      {
         i++;  // skip past first
         if( i < len )  // if there are more chars
         {
            i++;  // skip past second char
            if( i < len )
            {
               size_t i2 = i + 1;
               if( i2 < len )
               {
                  if ( pb[i2] >= '5' )
                  {
                     if( pb[i-1] < '9' )
                     {
                        pb[i-1]++;
                     }
                  }
               }
            }
         }
         pb[i] = 0;
         break;
      }
      zcnt++;     // count ZEROS after DOT
   }
   if( zcnt == (len - dot) )
   {
      // it was ALL zeros
      pb[dot - 1] = 0;
   }
}

char *double_to_stg( double d )
{
    char *cp = GetNxtBuf();
    sprintf(cp,"%lf",d);
    trim_float_buf(cp);
    return cp;
}

char *get_I64_Stg( long long val )
{
    char *cp = GetNxtBuf();
#ifdef _MSC_VER
    sprintf(cp,"%I64d",val);
#else
    sprintf(cp,"%lld",val);
#endif
    return cp;
}
char *get_I64u_Stg( unsigned long long val )
{
    char *cp = GetNxtBuf();
#ifdef _MSC_VER
    sprintf(cp,"%I64u",val);
#else
    sprintf(cp,"%llu",val);
#endif
    return cp;
}

#ifndef SPRINTF
#define SPRINTF sprintf
#endif
#ifndef STRLEN
#define STRLEN strlen
#endif
#ifndef STRCAT
#define STRCAT strcat
#endif

char *get_nice_number64( long long num, size_t width )
{
   size_t len, i;
   char *pb1 = get_I64_Stg(num);
   char *pb2 = GetNxtBuf();
   nice_num( pb2, pb1 );
   len = STRLEN(pb2);
   if(width && ( len < width ))
   {
      *pb1 = 0;
      i = width - len;  // get pad
      while(i--)
         STRCAT(pb1, " ");
      STRCAT(pb1,pb2);
      pb2 = pb1;
   }
   return pb2;
}

char *get_nice_number64u( unsigned long long num, size_t width )
{
   size_t len, i;
   char *pb1 = get_I64_Stg(num);
   char *pb2 = GetNxtBuf();
   nice_num( pb2, pb1 );
   len = STRLEN(pb2);
   if( width && ( len < width ))
   {
      *pb1 = 0;
      i = width - len;  // get pad
      while(i--)
         STRCAT(pb1, " ");
      STRCAT(pb1,pb2);
      pb2 = pb1;
   }
   return pb2;
}


/* ======================================================================
   nice_num = get nice number, with commas
   given a destination buffer,
   and a source buffer of ascii
   NO CHECK OF LENGTH DONE!!! assumed destination is large enough
   and assumed it is a NUMBER ONLY IN THE SOURCE
   ====================================================================== */
void nice_num( char * dst, char * src ) // get nice number, with commas
{
   size_t i;
   size_t len = strlen(src);
   size_t rem = len % 3;
   size_t cnt = 0;
   for( i = 0; i < len; i++ )
   {
      if( rem ) {
         *dst++ = src[i];
         rem--;
         if( ( rem == 0 ) && ( (i + 1) < len ) )
            *dst++ = ',';
      } else {
         *dst++ = src[i];
         cnt++;
         if( ( cnt == 3 ) && ( (i + 1) < len ) ) {
            *dst++ = ',';
            cnt = 0;
         }
      }
   }
   *dst = 0;
}



char *get_k_num( long long i64, int type, int dotrim )
{
   char *pb = GetNxtBuf();
   const char *form = " bytes";
   unsigned long long byts = i64;
   double res;
   const char*ffm = "%0.20f";  // get 20 digits
   if( byts < 1024 ) {
      strcpy(pb, get_I64_Stg(byts));
      dotrim = 0;
   } else if( byts < 1024*1024 ) {
      res = ((double)byts / 1024.0);
      form = (type ? " KiloBypes" : " KB");
      SPRINTF(pb, ffm, res);
   } else if( byts < 1024*1024*1024 ) {
      res = ((double)byts / (1024.0*1024.0));
      form = (type ? " MegaBypes" : " MB");
      SPRINTF(pb, ffm, res);
   } else { // if( byts <  (1024*1024*1024*1024)){
      double val = (double)byts;
      double db = (1024.0*1024.0*1024.0);   // x3
      double db2 = (1024.0*1024.0*1024.0*1024.0);   // x4
      if( val < db2 )
      {
         res = val / db;
         form = (type ? " GigaBypes" : " GB");
         SPRINTF(pb, ffm, res);
      }
      else
      {
         db *= 1024.0;  // x4
         db2 *= 1024.0; // x5
         if( val < db2 )
         {
            res = val / db;
            form = (type ? " TeraBypes" : " TB");
            SPRINTF(pb, ffm, res);
         }
         else
         {
            db *= 1024.0;  // x5
            db2 *= 1024.0; // x6
            if( val < db2 )
            {
               res = val / db;
               form = (type ? " PetaBypes" : " PB");
               SPRINTF(pb, ffm, res);
            }
            else
            {
               db *= 1024.0;  // x6
               db2 *= 1024.0; // x7
               if( val < db2 )
               {
                  res = val / db;
                  form = (type ? " ExaBypes" : " EB");
                  SPRINTF(pb, ffm, res);
               }
               else
               {
                  db *= 1024.0;  // x7
                  res = val / db;
                  form = (type ? " ZettaBypes" : " ZB");
                  SPRINTF(pb, ffm, res);
               }
            }
         }
      }
   }
   if( dotrim > 0 )
      trim_float_buf(pb);

   STRCAT(pb, form);

   //if( ascii > 0 )
   //   Convert_2_ASCII(pb);

   return pb;
}

#if 0 // 0000000000 already in sprtf 000000000
#ifdef _MSC_VER
int gettimeofday(struct timeval *tp, void *tzp) // from crossfeed source
{
    struct _timeb timebuffer;
    _ftime(&timebuffer);
    tp->tv_sec = (long)timebuffer.time;
    tp->tv_usec = timebuffer.millitm * 1000;
    return 0;
}
#endif
#endif // 0000000000000000000000000000000000

double get_seconds()
{
    struct timeval tv;
    gettimeofday(&tv,0);
    double t1 = (double)(tv.tv_sec+((double)tv.tv_usec/1000000.0));
    return t1;
}
char *get_elapsed_stg( double start )
{
    double now = get_seconds();
    return double_to_stg( now - start );
}



// eof = utils.cxx
