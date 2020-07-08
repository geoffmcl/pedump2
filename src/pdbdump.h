//==================================
// PEDUMP2 - Geoff R. McLane 2020
// FILE: pdbdump.h
//==================================
#ifndef _PDBDUMP_H_
#define _PDBDUMP_H_

extern int IsPdbFile( char *cp, size_t len );
extern int DumpPdbFile( char *cp, size_t len );

#endif // #ifndef _PDBDUMP_H_
/* eof */