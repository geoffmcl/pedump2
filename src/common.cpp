//==================================
// PEDUMP - Matt Pietrek 1997
// FILE: COMMON.C
//==================================
#ifdef _WIN32
#include <windows.h>
#else // !_WIN32
// not WIN32 == unix
#endif // _WIN32 y/n
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "common.h"
#include "symboltablesupport.h"
#include "extrnvar.h"

PIMAGE_DEBUG_MISC g_pMiscDebugInfo = 0;
PDWORD g_pCVHeader = 0;
PIMAGE_COFF_SYMBOLS_HEADER g_pCOFFHeader = 0;
COFFSymbolTable * g_pCOFFSymbolTable = 0;   // 0 == does not exit
vSTR vDllList;  // FIX20171017
char cMachineType[256] = { 0 };

void str_to_upper(std::string &data)
{
    std::transform(data.begin(), data.end(), data.begin(), ::toupper);
}

bool in_dll_list(std::string sDll)  // FIX20171017
{
    size_t max = vDllList.size();
    size_t ii;
    std::string su(sDll);
    str_to_upper(su);
    std::string s;
    for (ii = 0; ii < max; ii++) {
        s = vDllList[ii];
        str_to_upper(s);
        if (s.compare(su) == 0)
            return true;
    }
    return false;
}

void add_2_dll_list(char * pDLL)  // FIX20171017
{
    std::string s(pDLL);
    if (!in_dll_list(s))
        vDllList.push_back(s);
}

void kill_dll_list()  // FIX20171017
{
    vDllList.clear();
}

bool stringCompare(const std::string &left, const std::string &right)
{
    for (std::string::const_iterator lit = left.begin(), rit = right.begin();
        lit != left.end() && rit != right.end(); ++lit, ++rit) {
        if (tolower(*lit) < tolower(*rit))
            return true;
        else if (tolower(*lit) > tolower(*rit))
            return false;
    }
    if (left.size() < right.size())
        return true;
    return false;
}

void show_dll_list()  // FIX20171017
{
    size_t max = vDllList.size();
    size_t mwid = 100;
    size_t ii, len, sz;
    std::string s;
    if (max) {
        SPRTF("Total imports: %d, in case insensitive alphabetic order...\n", (int)max);
        std::sort(vDllList.begin(), vDllList.end(), stringCompare);
    }
    len = 0;

    for (ii = 0; ii < max; ii++) {
        s = vDllList[ii];
        sz = s.size();
        if (sz > mwid) {
            if (len)
                SPRTF("\n"); // close and previous
            SPRTF("%s\n", s.c_str()); // and this
            len = 0;
        }
        else if ((len + sz) > mwid) {
            if (len)
                SPRTF("\n");  // close and previous
            SPRTF("%s ", s.c_str()); // out this
            len = sz;   // and start size counter
        }
        else {
            SPRTF("%s ", s.c_str()); // add this
            len += sz;  // and add to width
        }
    }
    if (max) {
        if (len)
            SPRTF("\n\n"); // close current
        else
            SPRTF("\n");
    }
    kill_dll_list();
}


/*----------------------------------------------------------------------------*/
//
// Header related stuff
//
/*----------------------------------------------------------------------------*/

typedef struct
{
    WORD    flag;
    PSTR    name;
} WORD_FLAG_DESCRIPTIONS;

typedef struct
{
    DWORD   flag;
    PSTR    name;
} DWORD_FLAG_DESCRIPTIONS;

// Bitfield values and names for the IMAGE_FILE_HEADER flags
#if 0 // 0000000000000000000000000000000000000000000000000000000000
WORD_FLAG_DESCRIPTIONS ImageFileHeaderCharacteristics[] = 
{
{ IMAGE_FILE_RELOCS_STRIPPED, "RELOCS_STRIPPED" },
{ IMAGE_FILE_EXECUTABLE_IMAGE, "EXECUTABLE_IMAGE" },
{ IMAGE_FILE_LINE_NUMS_STRIPPED, "LINE_NUMS_STRIPPED" },
{ IMAGE_FILE_LOCAL_SYMS_STRIPPED, "LOCAL_SYMS_STRIPPED" },
{ IMAGE_FILE_AGGRESIVE_WS_TRIM, "AGGRESIVE_WS_TRIM" },
// { IMAGE_FILE_MINIMAL_OBJECT, "MINIMAL_OBJECT" }, // Removed in NT 3.5
// { IMAGE_FILE_UPDATE_OBJECT, "UPDATE_OBJECT" },   // Removed in NT 3.5
// { IMAGE_FILE_16BIT_MACHINE, "16BIT_MACHINE" },   // Removed in NT 3.5
{ IMAGE_FILE_BYTES_REVERSED_LO, "BYTES_REVERSED_LO" },
{ IMAGE_FILE_32BIT_MACHINE, "32BIT_MACHINE" },
{ IMAGE_FILE_DEBUG_STRIPPED, "DEBUG_STRIPPED" },
// { IMAGE_FILE_PATCH, "PATCH" },
{ IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP, "REMOVABLE_RUN_FROM_SWAP" },
{ IMAGE_FILE_NET_RUN_FROM_SWAP, "NET_RUN_FROM_SWAP" },
{ IMAGE_FILE_SYSTEM, "SYSTEM" },
{ IMAGE_FILE_DLL, "DLL" },
{ IMAGE_FILE_UP_SYSTEM_ONLY, "UP_SYSTEM_ONLY" },
{ IMAGE_FILE_BYTES_REVERSED_HI, "BYTES_REVERSED_HI" }
};
#endif // 00000000000000000000000000000000000000000000000000000000000
// 20171019 - 20140719 UPDATE
// Bitfield values and names for the IMAGE_FILE_HEADER flags
WORD_FLAG_DESCRIPTIONS ImageFileHeaderCharacteristics[] = {
    { IMAGE_FILE_RELOCS_STRIPPED,"RELOCS_STRIPPED" }, // 0x0001  // Relocation info stripped from file.
    { IMAGE_FILE_EXECUTABLE_IMAGE,"EXECUTABLE_IMAGE" }, // 0x0002  // File is executable  (i.e. no unresolved externel references).
    { IMAGE_FILE_LINE_NUMS_STRIPPED,"LINE_NUMS_STRIPPED" }, // 0x0004  // Line nunbers stripped from file.
    { IMAGE_FILE_LOCAL_SYMS_STRIPPED,"LOCAL_SYMS_STRIPPED" }, // 0x0008  // Local symbols stripped from file.
    { IMAGE_FILE_AGGRESIVE_WS_TRIM,"AGGRESIVE_WS_TRIM" }, // 0x0010  // Agressively trim working set
    { IMAGE_FILE_LARGE_ADDRESS_AWARE,"LARGE_ADDRESS_AWARE" }, // 0x0020  // App can handle >2gb addresses
    { IMAGE_FILE_BYTES_REVERSED_LO,"BYTES_REVERSED_LO" }, //  0x0080  // Bytes of machine word are reversed.
    { IMAGE_FILE_32BIT_MACHINE,"32BIT_MACHINE" }, // 0x0100  // 32 bit word machine.
    { IMAGE_FILE_DEBUG_STRIPPED,"DEBUG_STRIPPED" }, // 0x0200  // Debugging info stripped from file in .DBG file
    { IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP,"REMOVABLE_RUN_FROM_SWAP" }, // 0x0400  // If Image is on removable media, copy and run from the swap file.
    { IMAGE_FILE_NET_RUN_FROM_SWAP,"NET_RUN_FROM_SWAP" }, // 0x0800  // If Image is on Net, copy and run from the swap file.
    { IMAGE_FILE_SYSTEM,"SYSTEM" }, //                    0x1000  // System File.
    { IMAGE_FILE_DLL,"DLL" }, //                       0x2000  // File is a DLL.
    { IMAGE_FILE_UP_SYSTEM_ONLY,"UP_SYSTEM_ONLY" }, //            0x4000  // File should only be run on a UP machine
    { IMAGE_FILE_BYTES_REVERSED_HI,"BYTES_REVERSED_HI" } //       0x8000  // Bytes of machine word are reversed.
};


#define NUMBER_IMAGE_HEADER_FLAGS \
    (sizeof(ImageFileHeaderCharacteristics) / sizeof(WORD_FLAG_DESCRIPTIONS))

//
// Dump the IMAGE_FILE_HEADER for a PE file or an OBJ or LIB?
//

/*
typedef struct _IMAGE_FILE_HEADER {
WORD  Machine;
WORD  NumberOfSections;
DWORD TimeDateStamp;
DWORD PointerToSymbolTable;
DWORD NumberOfSymbols;
WORD  SizeOfOptionalHeader;
WORD  Characteristics;
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;

*/
void DumpHeader(PIMAGE_FILE_HEADER pImageFileHeader)
{
    UINT headerFieldWidth = 30;
    UINT i;
    
    if (!fShowMachineType)
        SPRTF("File Header:\n");
    if (!IsAddressInRange((BYTE *)pImageFileHeader, (int)sizeof(IMAGE_FILE_HEADER))) {
        SPRTF("TODO: PIMAGE_FILE_HEADER out of range - %p\n", pImageFileHeader);
        return;
    }

    // musing - TODO: seems if Machine is 0, date bad, and number of section = 0xFFFF, should be considered INVALID
    // from: https://msdn.microsoft.com/en-us/library/windows/desktop/ms680313(v=vs.85).aspx
    // NumberOfSections
    // The number of sections.This indicates the size of the section table, which immediately follows the headers.
    // Note that the Windows loader limits the number of sections to 96. !!!!!!!!!!!!!!!!!!!!
    // *****************************************************************

    WORD nSects = pImageFileHeader->NumberOfSections;
    PSTR mt = GetMachineTypeName(pImageFileHeader->Machine);

    if (pImageFileHeader->Machine && (cMachineType[0] == 0))   // FIX20171021 - Only collect/show the FIRST instance of this, and ONLY if NOT 0
        sprintf(cMachineType, "PE/OBJ: Machine Type: %04x (%s)", pImageFileHeader->Machine, mt);
    if (fShowMachineType)
        return; // all done

    SPRTF("  %-*s%04X (%s)\n", headerFieldWidth, "Machine:", 
                pImageFileHeader->Machine,
                mt );
    SPRTF("  %-*s%04X\n", headerFieldWidth, "Number of Sections:", nSects); // pImageFileHeader->NumberOfSections);
    SPRTF("  %-*s%08X -> %s\n", headerFieldWidth, "TimeDateStamp:",
                pImageFileHeader->TimeDateStamp,
                get_ctime_stg((time_t *)&pImageFileHeader->TimeDateStamp));
    SPRTF("  %-*s%08X\n", headerFieldWidth, "PointerToSymbolTable:",
                pImageFileHeader->PointerToSymbolTable);
    SPRTF("  %-*s%08X\n", headerFieldWidth, "NumberOfSymbols:",
                pImageFileHeader->NumberOfSymbols);
    SPRTF("  %-*s%04X\n", headerFieldWidth, "SizeOfOptionalHeader:",
                pImageFileHeader->SizeOfOptionalHeader);
    WORD Chars = pImageFileHeader->Characteristics;
    SPRTF("  %-*s%04X\n", headerFieldWidth, "Characteristics:",
                Chars);
    for ( i=0; i < NUMBER_IMAGE_HEADER_FLAGS; i++ )
    {
        WORD flag = ImageFileHeaderCharacteristics[i].flag;
        //if ( pImageFileHeader->Characteristics & 
        //     ImageFileHeaderCharacteristics[i].flag )
        if (Chars & flag) {
            SPRTF("    %s\n", ImageFileHeaderCharacteristics[i].name);
            Chars &= ~flag;
            if (Chars == 0)
                break;
        }
    }
    if (Chars) {
        SPRTF("    Unknown bits %u\n", Chars);
    }
}

#ifndef	IMAGE_DLLCHARACTERISTICS_WDM_DRIVER
#define IMAGE_DLLCHARACTERISTICS_WDM_DRIVER  0x2000     // Driver uses WDM model
#endif

// Marked as obsolete in MSDN CD 9
// Bitfield values and names for the DllCharacteritics flags
WORD_FLAG_DESCRIPTIONS DllCharacteristics[] = 
{
{ IMAGE_DLLCHARACTERISTICS_WDM_DRIVER, "WDM_DRIVER" },
};
#define NUMBER_DLL_CHARACTERISTICS \
    (sizeof(DllCharacteristics) / sizeof(WORD_FLAG_DESCRIPTIONS))

#if 0
// Marked as obsolete in MSDN CD 9
// Bitfield values and names for the LoaderFlags flags
DWORD_FLAG_DESCRIPTIONS LoaderFlags[] = 
{
{ IMAGE_LOADER_FLAGS_BREAK_ON_LOAD, "BREAK_ON_LOAD" },
{ IMAGE_LOADER_FLAGS_DEBUG_ON_LOAD, "DEBUG_ON_LOAD" }
};
#define NUMBER_LOADER_FLAGS \
    (sizeof(LoaderFlags) / sizeof(DWORD_FLAG_DESCRIPTIONS))
#endif

typedef struct tagIDNAMES {
    const char *name;
    DWORD   dwVirtualAddess;
    DWORD   dwSize;
    const char *var;
}IDNAMES, *PIDNAME;

// Names of the data directory elements that are defined
/*
char *ImageDirectoryNames[] = {
    "EXPORT", "IMPORT", "RESOURCE", "EXCEPTION", "SECURITY", "BASERELOC",
    "DEBUG", "COPYRIGHT", "GLOBALPTR", "TLS", "LOAD_CONFIG",
    "BOUND_IMPORT", "IAT",  // These two entries added for NT 3.51
	"DELAY_IMPORT" };		// This entry added in NT 5

#define NUMBER_IMAGE_DIRECTORY_ENTRYS \
    (sizeof(ImageDirectoryNames)/sizeof(char *))
    */
/*
    Extract from winnt.h
    // Directory Entries

    #define IMAGE_DIRECTORY_ENTRY_EXPORT          0   // Export Directory
    #define IMAGE_DIRECTORY_ENTRY_IMPORT          1   // Import Directory
    #define IMAGE_DIRECTORY_ENTRY_RESOURCE        2   // Resource Directory
    #define IMAGE_DIRECTORY_ENTRY_EXCEPTION       3   // Exception Directory
    #define IMAGE_DIRECTORY_ENTRY_SECURITY        4   // Security Directory
    #define IMAGE_DIRECTORY_ENTRY_BASERELOC       5   // Base Relocation Table
    #define IMAGE_DIRECTORY_ENTRY_DEBUG           6   // Debug Directory
    //      IMAGE_DIRECTORY_ENTRY_COPYRIGHT       7   // (X86 usage)
    #define IMAGE_DIRECTORY_ENTRY_ARCHITECTURE    7   // Architecture Specific Data
    #define IMAGE_DIRECTORY_ENTRY_GLOBALPTR       8   // RVA of GP
    #define IMAGE_DIRECTORY_ENTRY_TLS             9   // TLS Directory
    #define IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG    10   // Load Configuration Directory
    #define IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT   11   // Bound Import Directory in headers
    #define IMAGE_DIRECTORY_ENTRY_IAT            12   // Import Address Table
    #define IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT   13   // Delay Load Import Descriptors
    #define IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR 14   // COM Runtime descriptor

*/
IDNAMES ImageDirectoryNames[] = {
    { "EXPORT", 0, 0, "IMAGE_DIRECTORY_ENTRY_EXPORT(0)" },         //  0 Export Directory
    { "IMPORT", 0, 0, "IMAGE_DIRECTORY_ENTRY_IMPORT(1)" },         //  1 Import Directory
    { "RESOURCE", 0, 0, "IMAGE_DIRECTORY_ENTRY_RESOURCE(2)" },     //  2 Resource Directory
    { "EXCEPTION", 0, 0, "IMAGE_DIRECTORY_ENTRY_EXCEPTION(3)" },      //  3 Exception Directory
    { "SECURITY", 0, 0, "IMAGE_DIRECTORY_ENTRY_SECURITY(4)" },       //  4 Security Directory
    { "BASERELOC", 0, 0, "IMAGE_DIRECTORY_ENTRY_BASERELOC(5)" },      //  5 Base Relocation Table
    { "DEBUG",  0, 0, "IMAGE_DIRECTORY_ENTRY_DEBUG (6)" },         //  6 Debug Directory
    { "COPYRIGHT",  0, 0, "IMAGE_DIRECTORY_ENTRY_ARCHITECTURE(7)" },     //  7 Architecture Specific Data
    { "GLOBALPTR",  0, 0, "IMAGE_DIRECTORY_ENTRY_GLOBALPTR(8)" },     //  8 RVA of GP
    { "TLS",  0, 0, "IMAGE_DIRECTORY_ENTRY_TLS(9)" },           //  9 TLS Directory
    { "LOAD_CONFIG",  0, 0, "IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG(10)"  },   // 10 Load Configuration Directory
    { "BOUND_IMPORT",  0, 0, "IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT(11)" },  // 11 Bound Import Directory in headers
    { "IAT",   0, 0, "IMAGE_DIRECTORY_ENTRY_IAT(12)" },          // 12 Import Address Table // These two entries added for NT 3.51
    { "DELAY_IMPORT", 0, 0, "IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT(13)" },   // 13 Delay Load Import Descriptors // This entry added in NT 5
    { "COM_RUNTIME", 0, 0, "IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR(14)" }     // 14 COM Runtime descriptor
};

#define NUMBER_IMAGE_DIRECTORY_ENTRYS \
    (sizeof(ImageDirectoryNames)/sizeof(IDNAMES))

BOOL DoesSectionExist(const char *sect) 
{
    int i;
    for (i = 0; i < NUMBER_IMAGE_DIRECTORY_ENTRYS; i++) {
        if (strcmp(ImageDirectoryNames[i].name, sect) == 0) {
            if (ImageDirectoryNames[i].dwVirtualAddess && ImageDirectoryNames[i].dwSize)
                return TRUE;
            else
                return FALSE;

        }
    }
    return FALSE;

}

//
// Dump the IMAGE_OPTIONAL_HEADER from a PE file
//
//void DumpOptionalHeader(PIMAGE_OPTIONAL_HEADER optionalHeader)
/*
typedef struct _IMAGE_OPTIONAL_HEADER {
//
// Standard fields.
//

WORD    Magic;
BYTE    MajorLinkerVersion;
BYTE    MinorLinkerVersion;
DWORD   SizeOfCode;
DWORD   SizeOfInitializedData;
DWORD   SizeOfUninitializedData;
DWORD   AddressOfEntryPoint;
DWORD   BaseOfCode;
DWORD   BaseOfData;

//
// NT additional fields.
//

DWORD   ImageBase;
DWORD   SectionAlignment;
DWORD   FileAlignment;
WORD    MajorOperatingSystemVersion;
WORD    MinorOperatingSystemVersion;
WORD    MajorImageVersion;
WORD    MinorImageVersion;
WORD    MajorSubsystemVersion;
WORD    MinorSubsystemVersion;
DWORD   Win32VersionValue;
DWORD   SizeOfImage;
DWORD   SizeOfHeaders;
DWORD   CheckSum;
WORD    Subsystem;
WORD    DllCharacteristics;
DWORD   SizeOfStackReserve;
DWORD   SizeOfStackCommit;
DWORD   SizeOfHeapReserve;
DWORD   SizeOfHeapCommit;
DWORD   LoaderFlags;
DWORD   NumberOfRvaAndSizes;
IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER32, *PIMAGE_OPTIONAL_HEADER32;

*/
void DumpOptionalHeader32(PIMAGE_OPTIONAL_HEADER32 optionalHeader)
{
    UINT width = 30;
    char *s;
    UINT i;
    
    SPRTF("Optional Header - 32-bit\n");
    
    SPRTF("  %-*s%04X\n", width, "Magic", optionalHeader->Magic);
    SPRTF("  %-*s%u.%02u\n", width, "linker version",
        optionalHeader->MajorLinkerVersion,
        optionalHeader->MinorLinkerVersion);
    SPRTF("  %-*s%X\n", width, "size of code", optionalHeader->SizeOfCode);
    SPRTF("  %-*s%X\n", width, "size of initialized data",
        optionalHeader->SizeOfInitializedData);
    SPRTF("  %-*s%X\n", width, "size of uninitialized data",
        optionalHeader->SizeOfUninitializedData);
    SPRTF("  %-*s%X\n", width, "entrypoint RVA",
        optionalHeader->AddressOfEntryPoint);
    SPRTF("  %-*s%X\n", width, "base of code", optionalHeader->BaseOfCode);
    SPRTF("  %-*s%X\n", width, "base of data", optionalHeader->BaseOfData);
    SPRTF("  %-*s%X\n", width, "image base", optionalHeader->ImageBase);

    SPRTF("  %-*s%X\n", width, "section align",
        optionalHeader->SectionAlignment);
    SPRTF("  %-*s%X\n", width, "file align", optionalHeader->FileAlignment);
    SPRTF("  %-*s%u.%02u\n", width, "required OS version",
        optionalHeader->MajorOperatingSystemVersion,
        optionalHeader->MinorOperatingSystemVersion);
    SPRTF("  %-*s%u.%02u\n", width, "image version",
        optionalHeader->MajorImageVersion,
        optionalHeader->MinorImageVersion);
    SPRTF("  %-*s%u.%02u\n", width, "subsystem version",
        optionalHeader->MajorSubsystemVersion,
        optionalHeader->MinorSubsystemVersion);
    SPRTF("  %-*s%X\n", width, "Win32 Version",
    	optionalHeader->Win32VersionValue);
    SPRTF("  %-*s%X\n", width, "size of image", optionalHeader->SizeOfImage);
    SPRTF("  %-*s%X\n", width, "size of headers",
            optionalHeader->SizeOfHeaders);
    SPRTF("  %-*s%X\n", width, "checksum", optionalHeader->CheckSum);
    switch( optionalHeader->Subsystem )
    {
        case IMAGE_SUBSYSTEM_NATIVE: s = "Native"; break;
        case IMAGE_SUBSYSTEM_WINDOWS_GUI: s = "Windows GUI"; break;
        case IMAGE_SUBSYSTEM_WINDOWS_CUI: s = "Windows character"; break;
        case IMAGE_SUBSYSTEM_OS2_CUI: s = "OS/2 character"; break;
        case IMAGE_SUBSYSTEM_POSIX_CUI: s = "Posix character"; break;
        default: s = "unknown";
    }
    SPRTF("  %-*s%04X (%s)\n", width, "Subsystem",
            optionalHeader->Subsystem, s);

// Marked as obsolete in MSDN CD 9
    SPRTF("  %-*s%04X\n", width, "DLL flags",
            optionalHeader->DllCharacteristics);
    for ( i=0; i < NUMBER_DLL_CHARACTERISTICS; i++ )
    {
        if ( optionalHeader->DllCharacteristics & 
             DllCharacteristics[i].flag )
            SPRTF( "  %-*s%s", width, " ", DllCharacteristics[i].name );
    }
    if ( optionalHeader->DllCharacteristics )
        SPRTF("\n");

    SPRTF("  %-*s%X\n", width, "stack reserve size",
        optionalHeader->SizeOfStackReserve);
    SPRTF("  %-*s%X\n", width, "stack commit size",
        optionalHeader->SizeOfStackCommit);
    SPRTF("  %-*s%X\n", width, "heap reserve size",
        optionalHeader->SizeOfHeapReserve);
    SPRTF("  %-*s%X\n", width, "heap commit size",
        optionalHeader->SizeOfHeapCommit);

#if 0
// Marked as obsolete in MSDN CD 9
    SPRTF("  %-*s%08X\n", width, "loader flags",
        optionalHeader->LoaderFlags);

    for ( i=0; i < NUMBER_LOADER_FLAGS; i++ )
    {
        if ( optionalHeader->LoaderFlags & 
             LoaderFlags[i].flag )
            SPRTF( "  %s", LoaderFlags[i].name );
    }
    if ( optionalHeader->LoaderFlags )
        SPRTF("\n");
#endif

    SPRTF("  %-*s%X\n", width, "RVAs & sizes",
        optionalHeader->NumberOfRvaAndSizes);

    SPRTF("\nData Directory - 32-bit\n");
    for ( i=0; i < optionalHeader->NumberOfRvaAndSizes; i++)
    {
        DWORD dwVA = optionalHeader->DataDirectory[i].VirtualAddress;
        DWORD dwSZ = optionalHeader->DataDirectory[i].Size;
        char *name = (i >= NUMBER_IMAGE_DIRECTORY_ENTRYS) ? "unused" : ImageDirectoryNames[i].name;
        char *var = (i >= NUMBER_IMAGE_DIRECTORY_ENTRYS) ? "none" : 
            (dwVA && dwSZ) ? ImageDirectoryNames[i].var : "N/A";
        SPRTF("  %-12s rva: %08X  size: %08X - %s\n",
            name,
            dwVA, // optionalHeader->DataDirectory[i].VirtualAddress,
            dwSZ, // optionalHeader->DataDirectory[i].Size,
            var);
        if (i < NUMBER_IMAGE_DIRECTORY_ENTRYS) {
            ImageDirectoryNames[i].dwVirtualAddess = dwVA;
            ImageDirectoryNames[i].dwSize = dwSZ;
        }
    }
}

/*
typedef struct _IMAGE_OPTIONAL_HEADER64 {
WORD        Magic;
BYTE        MajorLinkerVersion;
BYTE        MinorLinkerVersion;
DWORD       SizeOfCode;
DWORD       SizeOfInitializedData;
DWORD       SizeOfUninitializedData;
DWORD       AddressOfEntryPoint;
DWORD       BaseOfCode;
ULONGLONG   ImageBase;
DWORD       SectionAlignment;
DWORD       FileAlignment;
WORD        MajorOperatingSystemVersion;
WORD        MinorOperatingSystemVersion;
WORD        MajorImageVersion;
WORD        MinorImageVersion;
WORD        MajorSubsystemVersion;
WORD        MinorSubsystemVersion;
DWORD       Win32VersionValue;
DWORD       SizeOfImage;
DWORD       SizeOfHeaders;
DWORD       CheckSum;
WORD        Subsystem;
WORD        DllCharacteristics;
ULONGLONG   SizeOfStackReserve;
ULONGLONG   SizeOfStackCommit;
ULONGLONG   SizeOfHeapReserve;
ULONGLONG   SizeOfHeapCommit;
DWORD       LoaderFlags;
DWORD       NumberOfRvaAndSizes;
IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER64, *PIMAGE_OPTIONAL_HEADER64;

*/

void DumpOptionalHeader64(PIMAGE_OPTIONAL_HEADER64 optionalHeader)
{
    UINT width = 30;
    char *s;
    UINT i;

    SPRTF("Optional Header 64-bit\n");

    SPRTF("  %-*s%04X\n", width, "Magic", optionalHeader->Magic);
    SPRTF("  %-*s%u.%02u\n", width, "linker version",
        optionalHeader->MajorLinkerVersion,
        optionalHeader->MinorLinkerVersion);
    SPRTF("  %-*s%X\n", width, "size of code", optionalHeader->SizeOfCode);
    SPRTF("  %-*s%X\n", width, "size of initialized data",
        optionalHeader->SizeOfInitializedData);
    SPRTF("  %-*s%X\n", width, "size of uninitialized data",
        optionalHeader->SizeOfUninitializedData);
    SPRTF("  %-*s%X\n", width, "entrypoint RVA",
        optionalHeader->AddressOfEntryPoint);
    SPRTF("  %-*s%X\n", width, "base of code", optionalHeader->BaseOfCode);

    // SPRTF("  %-*s%X\n", width, "base of data", optionalHeader->BaseOfData);
    SPRTF("  %-*s%X\n", width, "image base", optionalHeader->ImageBase);

    SPRTF("  %-*s%X\n", width, "section align",
        optionalHeader->SectionAlignment);
    SPRTF("  %-*s%X\n", width, "file align", optionalHeader->FileAlignment);
    SPRTF("  %-*s%u.%02u\n", width, "required OS version",
        optionalHeader->MajorOperatingSystemVersion,
        optionalHeader->MinorOperatingSystemVersion);
    SPRTF("  %-*s%u.%02u\n", width, "image version",
        optionalHeader->MajorImageVersion,
        optionalHeader->MinorImageVersion);
    SPRTF("  %-*s%u.%02u\n", width, "subsystem version",
        optionalHeader->MajorSubsystemVersion,
        optionalHeader->MinorSubsystemVersion);
    SPRTF("  %-*s%X\n", width, "Win32 Version",
        optionalHeader->Win32VersionValue);
    SPRTF("  %-*s%X\n", width, "size of image", optionalHeader->SizeOfImage);
    SPRTF("  %-*s%X\n", width, "size of headers",
        optionalHeader->SizeOfHeaders);
    SPRTF("  %-*s%X\n", width, "checksum", optionalHeader->CheckSum);
    switch (optionalHeader->Subsystem)
    {
    case IMAGE_SUBSYSTEM_NATIVE: s = "Native"; break;
    case IMAGE_SUBSYSTEM_WINDOWS_GUI: s = "Windows GUI"; break;
    case IMAGE_SUBSYSTEM_WINDOWS_CUI: s = "Windows character"; break;
    case IMAGE_SUBSYSTEM_OS2_CUI: s = "OS/2 character"; break;
    case IMAGE_SUBSYSTEM_POSIX_CUI: s = "Posix character"; break;
    default: s = "unknown";
    }
    SPRTF("  %-*s%04X (%s)\n", width, "Subsystem",
        optionalHeader->Subsystem, s);

    // Marked as obsolete in MSDN CD 9
    SPRTF("  %-*s%04X\n", width, "DLL flags",
        optionalHeader->DllCharacteristics);
    for (i = 0; i < NUMBER_DLL_CHARACTERISTICS; i++)
    {
        if (optionalHeader->DllCharacteristics &
            DllCharacteristics[i].flag)
            SPRTF("  %-*s%s", width, " ", DllCharacteristics[i].name);
    }
    if (optionalHeader->DllCharacteristics)
        SPRTF("\n");

    SPRTF("  %-*s%X\n", width, "stack reserve size",
        optionalHeader->SizeOfStackReserve);
    SPRTF("  %-*s%X\n", width, "stack commit size",
        optionalHeader->SizeOfStackCommit);
    SPRTF("  %-*s%X\n", width, "heap reserve size",
        optionalHeader->SizeOfHeapReserve);
    SPRTF("  %-*s%X\n", width, "heap commit size",
        optionalHeader->SizeOfHeapCommit);

#if 0
    // Marked as obsolete in MSDN CD 9
    SPRTF("  %-*s%08X\n", width, "loader flags",
        optionalHeader->LoaderFlags);

    for (i = 0; i < NUMBER_LOADER_FLAGS; i++)
    {
        if (optionalHeader->LoaderFlags &
            LoaderFlags[i].flag)
            SPRTF("  %s", LoaderFlags[i].name);
    }
    if (optionalHeader->LoaderFlags)
        SPRTF("\n");
#endif

    SPRTF("  %-*s%X\n", width, "RVAs & sizes",
        optionalHeader->NumberOfRvaAndSizes);

    SPRTF("\nData Directory - 64-bit\n");
    for (i = 0; i < optionalHeader->NumberOfRvaAndSizes; i++)
    {
        DWORD dwVA = optionalHeader->DataDirectory[i].VirtualAddress;
        DWORD dwSZ = optionalHeader->DataDirectory[i].Size;
        char *name = (i >= NUMBER_IMAGE_DIRECTORY_ENTRYS) ? "unused" : ImageDirectoryNames[i].name;
        char *var = (i >= NUMBER_IMAGE_DIRECTORY_ENTRYS) ? "none" :
            (dwVA && dwSZ) ? ImageDirectoryNames[i].var : "N/A";
        SPRTF("  %-12s rva: %08X  size: %08X - %s\n",
            name,
            dwVA, // optionalHeader->DataDirectory[i].VirtualAddress,
            dwSZ, // optionalHeader->DataDirectory[i].Size,
            var);
        if (i < NUMBER_IMAGE_DIRECTORY_ENTRYS) {
            ImageDirectoryNames[i].dwVirtualAddess = dwVA;
            ImageDirectoryNames[i].dwSize = dwSZ;
        }
    }
}

#if 0 // 00000000000000000000000000000000000000000000000000000000000
// Original 1998 short list
PSTR GetMachineTypeName( WORD wMachineType )
{
    switch( wMachineType )
    {
        case IMAGE_FILE_MACHINE_I386: 	return "i386";
        // case IMAGE_FILE_MACHINE_I860:return "i860";
        case IMAGE_FILE_MACHINE_R3000:  return "R3000";
		case 160:                       return "R3000 big endian";
        case IMAGE_FILE_MACHINE_R4000:  return "R4000";
		case IMAGE_FILE_MACHINE_R10000: return "R10000";
        case IMAGE_FILE_MACHINE_ALPHA:  return "Alpha";
        case IMAGE_FILE_MACHINE_POWERPC:return "PowerPC";
        default:    					return "unknown";
    }
}
#endif // 00000000000000000000000000000000000000000000000000000000000
// 20140614: Increase Machine Type Names - from WinNT.h
PSTR GetMachineTypeName(WORD wMachineType)
{
    switch (wMachineType)
    {
    case IMAGE_FILE_MACHINE_UNKNOWN: return "Unknown";  // 0
    case IMAGE_FILE_MACHINE_I386: return "Intel 386";   // 0x014c  // Intel 386.
    case IMAGE_FILE_MACHINE_R3000: return "MIPS-LE";    // 0x0162  // MIPS little-endian, 0x160 big-endian
    case IMAGE_FILE_MACHINE_R4000: return "MIPS-LE";    // 0x0166  // MIPS little-endian
    case IMAGE_FILE_MACHINE_R10000: return "MIPS-LE";   // 0x0168  // MIPS little-endian
    case IMAGE_FILE_MACHINE_WCEMIPSV2: return "MIPS-LE_WCE"; // 0x0169  // MIPS little-endian WCE v2
    case IMAGE_FILE_MACHINE_ALPHA: return "Alpha_AXP";  // 0x0184  // Alpha_AXP
    case IMAGE_FILE_MACHINE_SH3: return "SH3-LE";       // 0x01a2  // SH3 little-endian
    case IMAGE_FILE_MACHINE_SH3DSP: return "SH3DSP";    // 0x01a3
    case IMAGE_FILE_MACHINE_SH3E: return "SH3E-LE";     // 0x01a4  // SH3E little-endian
    case IMAGE_FILE_MACHINE_SH4: return "SH4-LE";       // 0x01a6  // SH4 little-endian
    case IMAGE_FILE_MACHINE_SH5: return "SH5";          // 0x01a8  // SH5
    case IMAGE_FILE_MACHINE_ARM: return "ARM-LE";       // 0x01c0  // ARM Little-Endian
    case IMAGE_FILE_MACHINE_THUMB: return "Thumb";      // 0x01c2
    case IMAGE_FILE_MACHINE_AM33: return "ARM33";       // 0x01d3
    case IMAGE_FILE_MACHINE_POWERPC: return "PowerPC-LE"; // 0x01F0  // IBM PowerPC Little-Endian
    case IMAGE_FILE_MACHINE_POWERPCFP: return "PowerPCFP"; // 0x01f1
    case IMAGE_FILE_MACHINE_IA64: return "Intel 64";           // 0x0200  // Intel 64
    case IMAGE_FILE_MACHINE_MIPS16: return "MIPS16";    // 0x0266  // MIPS
    case IMAGE_FILE_MACHINE_ALPHA64: return "Alpha64";  // 0x0284  // ALPHA64
    case IMAGE_FILE_MACHINE_MIPSFPU: return "MIPSFPU";  // 0x0366  // MIPS
    case IMAGE_FILE_MACHINE_MIPSFPU16: return "MIPSFPU16";  // 0x0466  // MIPS
                                                            // case IMAGE_FILE_MACHINE_AXP64             IMAGE_FILE_MACHINE_ALPHA64
    case IMAGE_FILE_MACHINE_TRICORE: return "infineon"; // 0x0520  // Infineon
    case IMAGE_FILE_MACHINE_CEF: return "CEF";          // 0x0CEF
    case IMAGE_FILE_MACHINE_EBC: return "EFI-BC";       // 0x0EBC  // EFI Byte Code
    case IMAGE_FILE_MACHINE_AMD64: return "AMD64-K8";   // 0x8664  // AMD64 (K8)
    case IMAGE_FILE_MACHINE_M32R: return "M32R-LE";     // 0x9041  // M32R little-endian
    case IMAGE_FILE_MACHINE_CEE: return "CCE";          // 0xC0EE
    }
    return "UNLISTED";
}

int is_listed_machine_type(WORD wMachineType)
{
    PSTR ps = GetMachineTypeName(wMachineType);
    if (strcmp(ps, "UNLISTED") == 0)
        return 0;
    if (strcmp(ps, "Unknown") == 0)
        return 0;
    return 1;
}


/*----------------------------------------------------------------------------*/
//
// Section related stuff
//
/*----------------------------------------------------------------------------*/

//
// These aren't defined in the NT 4.0 WINNT.H, so we'll define them here
//
#ifndef IMAGE_SCN_TYPE_DSECT
#define IMAGE_SCN_TYPE_DSECT                 0x00000001  // Reserved.
#endif
#ifndef IMAGE_SCN_TYPE_NOLOAD
#define IMAGE_SCN_TYPE_NOLOAD                0x00000002  // Reserved.
#endif
#ifndef IMAGE_SCN_TYPE_GROUP
#define IMAGE_SCN_TYPE_GROUP                 0x00000004  // Reserved.
#endif
#ifndef IMAGE_SCN_TYPE_COPY
#define IMAGE_SCN_TYPE_COPY                  0x00000010  // Reserved.
#endif
#ifndef IMAGE_SCN_TYPE_OVER
#define IMAGE_SCN_TYPE_OVER                  0x00000400  // Reserved.
#endif
#ifndef IMAGE_SCN_MEM_PROTECTED
#define IMAGE_SCN_MEM_PROTECTED              0x00004000
#endif
#ifndef IMAGE_SCN_MEM_SYSHEAP
#define IMAGE_SCN_MEM_SYSHEAP                0x00010000
#endif

// Bitfield values and names for the IMAGE_SECTION_HEADER flags
DWORD_FLAG_DESCRIPTIONS SectionCharacteristics[] = 
{

{ IMAGE_SCN_TYPE_DSECT, "DSECT" },
{ IMAGE_SCN_TYPE_NOLOAD, "NOLOAD" },
{ IMAGE_SCN_TYPE_GROUP, "GROUP" },
{ IMAGE_SCN_TYPE_NO_PAD, "NO_PAD" },
{ IMAGE_SCN_TYPE_COPY, "COPY" },
{ IMAGE_SCN_CNT_CODE, "CODE" },
{ IMAGE_SCN_CNT_INITIALIZED_DATA, "INITIALIZED_DATA" },
{ IMAGE_SCN_CNT_UNINITIALIZED_DATA, "UNINITIALIZED_DATA" },
{ IMAGE_SCN_LNK_OTHER, "OTHER" },
{ IMAGE_SCN_LNK_INFO, "INFO" },
{ IMAGE_SCN_TYPE_OVER, "OVER" },
{ IMAGE_SCN_LNK_REMOVE, "REMOVE" },
{ IMAGE_SCN_LNK_COMDAT, "COMDAT" },
{ IMAGE_SCN_MEM_PROTECTED, "PROTECTED" },
{ IMAGE_SCN_MEM_FARDATA, "FARDATA" },
{ IMAGE_SCN_MEM_SYSHEAP, "SYSHEAP" },
{ IMAGE_SCN_MEM_PURGEABLE, "PURGEABLE" },
{ IMAGE_SCN_MEM_LOCKED, "LOCKED" },
{ IMAGE_SCN_MEM_PRELOAD, "PRELOAD" },
{ IMAGE_SCN_LNK_NRELOC_OVFL, "NRELOC_OVFL" },
{ IMAGE_SCN_MEM_DISCARDABLE, "DISCARDABLE" },
{ IMAGE_SCN_MEM_NOT_CACHED, "NOT_CACHED" },
{ IMAGE_SCN_MEM_NOT_PAGED, "NOT_PAGED" },
{ IMAGE_SCN_MEM_SHARED, "SHARED" },
{ IMAGE_SCN_MEM_EXECUTE, "EXECUTE" },
{ IMAGE_SCN_MEM_READ, "READ" },
{ IMAGE_SCN_MEM_WRITE, "WRITE" },
};

#define NUMBER_SECTION_CHARACTERISTICS \
    (sizeof(SectionCharacteristics) / sizeof(DWORD_FLAG_DESCRIPTIONS))

//
// Dump the section table from a PE file or an OBJ
//
void DumpSectionTable(PIMAGE_SECTION_HEADER section, unsigned cSections, BOOL IsEXE, const char *caller)
{
    unsigned i, j;
	PSTR pszAlign;

    SPRTF("Section Table - %u sections...\n", cSections);
    if (cSections) {
        if (IsAddressInRange((BYTE *)section, (int)sizeof(IMAGE_SECTION_HEADER))) {
            PIMAGE_SECTION_HEADER last = section + (cSections - 1);
            if (IsAddressInRange((BYTE *)last, (int)sizeof(IMAGE_SECTION_HEADER))) {
                for (i = 1; i <= cSections; i++, section++)
                {
                    if (!IsAddressInRange((BYTE *)section, (int)sizeof(IMAGE_SECTION_HEADER))) {
                        SPRTF("TODO: DumpSectionTable by %s - PIMAGE_SECTION_HEADER %u of %u out of range - %p\n", caller, i, cSections, section);
                        return;
                    }
                    SPRTF("  %02X %-8.8s  %s: %08X  VirtAddr:  %08X\n",
                        i, section->Name,
                        IsEXE ? "VirtSize" : "PhysAddr",
                        section->Misc.VirtualSize, section->VirtualAddress);
                    SPRTF("    raw data offs:   %08X  raw data size: %08X\n",
                        section->PointerToRawData, section->SizeOfRawData);
                    SPRTF("    relocation offs: %08X  relocations:   %08X\n",
                        section->PointerToRelocations, section->NumberOfRelocations);
                    SPRTF("    line # offs:     %08X  line #'s:      %08X\n",
                        section->PointerToLinenumbers, section->NumberOfLinenumbers);
                    DWORD Chars = section->Characteristics;
                    SPRTF("    characteristics: %08X\n", Chars);

                    SPRTF("    ");
                    for (j = 0; j < NUMBER_SECTION_CHARACTERISTICS; j++)
                    {
                        DWORD flag = SectionCharacteristics[j].flag;
                        //if ( section->Characteristics & 
                        //    SectionCharacteristics[j].flag )
                        if (Chars & flag) {
                            SPRTF("  %s", SectionCharacteristics[j].name);
                            Chars &= ~flag;
                            if (Chars == 0)
                                break;
                        }
                    }

#if 0 // 00000000000000000000000000000000000000000000000000000000
                    switch (section->Characteristics & IMAGE_SCN_ALIGN_64BYTES)
                    {
                    case IMAGE_SCN_ALIGN_1BYTES: pszAlign = "ALIGN_1BYTES"; break;
                    case IMAGE_SCN_ALIGN_2BYTES: pszAlign = "ALIGN_2BYTES"; break;
                    case IMAGE_SCN_ALIGN_4BYTES: pszAlign = "ALIGN_4BYTES"; break;
                    case IMAGE_SCN_ALIGN_8BYTES: pszAlign = "ALIGN_8BYTES"; break;
                    case IMAGE_SCN_ALIGN_16BYTES: pszAlign = "ALIGN_16BYTES"; break;
                    case IMAGE_SCN_ALIGN_32BYTES: pszAlign = "ALIGN_32BYTES"; break;
                    case IMAGE_SCN_ALIGN_64BYTES: pszAlign = "ALIGN_64BYTES"; break;
                    default: pszAlign = "ALIGN_DEFAULT(16)"; break;
                    }
#endif // 00000000000000000000000000000000000000000000000000000000
                    // updated 20171019
                    //#define IMAGE_SCN_ALIGN_MASK                 0x00F00000
                    switch (section->Characteristics & IMAGE_SCN_ALIGN_MASK)
                    {
                    case IMAGE_SCN_ALIGN_1BYTES: pszAlign = "ALIGN_1BYTES"; break; // 0x00100000  //
                    case IMAGE_SCN_ALIGN_2BYTES: pszAlign = "ALIGN_2BYTES"; break; // 0x00200000  //
                    case IMAGE_SCN_ALIGN_4BYTES: pszAlign = "ALIGN_4BYTES"; break; // 0x00300000  //
                    case IMAGE_SCN_ALIGN_8BYTES: pszAlign = "ALIGN_8BYTES"; break; // 0x00400000  //
                    case IMAGE_SCN_ALIGN_16BYTES: pszAlign = "ALIGN_16BYTES"; break; // 0x00500000  // Default alignment if no others are specified.
                    case IMAGE_SCN_ALIGN_32BYTES: pszAlign = "ALIGN_32BYTES"; break; // 0x00600000  //
                    case IMAGE_SCN_ALIGN_64BYTES:  pszAlign = "ALIGN_64BYTES"; break; // 0x00700000  //
                    case IMAGE_SCN_ALIGN_128BYTES: pszAlign = "ALIGN_128BYTES"; break; // 0x00800000  //
                    case IMAGE_SCN_ALIGN_256BYTES: pszAlign = "ALIGN_256BYTES"; break; // 0x00900000  //
                    case IMAGE_SCN_ALIGN_512BYTES: pszAlign = "ALIGN_512BYTES"; break; // 0x00A00000  //
                    case IMAGE_SCN_ALIGN_1024BYTES: pszAlign = "ALIGN_1024BYTES"; break; // 0x00B00000  //
                    case IMAGE_SCN_ALIGN_2048BYTES: pszAlign = "ALIGN_2048BYTES"; break; // 0x00C00000  //
                    case IMAGE_SCN_ALIGN_4096BYTES: pszAlign = "ALIGN_4096BYTES"; break; // 0x00D00000  //
                    case IMAGE_SCN_ALIGN_8192BYTES: pszAlign = "ALIGN_8192BYTES"; break; // 0x00E00000  //
                                                                                         // Unused                                    0x00F00000
                    default: pszAlign = "ALIGN_DEFAULT(16)"; break;
                    }
                    SPRTF("  %s", pszAlign);
                    Chars &= ~IMAGE_SCN_ALIGN_MASK;
                    if (Chars) {
                        SPRTF(" Unk(%u)", Chars);
                    }

                    SPRTF("\n\n");
                }
            }
            else {
                SPRTF("TODO: DumpSectionTable - last PIMAGE_SECTION_HEADER out of range - %p\n", last);
            }
        }
        else {
            SPRTF("TODO: DumpSectionTable - first PIMAGE_SECTION_HEADER out of range - %p\n", section);
        }
    }
}

//
// Given a section name, look it up in the section table and return a
// pointer to the start of its raw data area.
//
// LPVOID GetSectionPtr(PSTR name, PIMAGE_NT_HEADERS pNTHeader, DWORD imageBase)
LPVOID GetSectionPtr(PSTR name, PIMAGE_NT_HEADERS pNTHeader, BYTE *imageBase)
{
    PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(pNTHeader);
    unsigned i;
    
    for ( i=0; i < pNTHeader->FileHeader.NumberOfSections; i++, section++ )
    {
        if (strncmp((char *)section->Name, name, IMAGE_SIZEOF_SHORT_NAME) == 0)
            return (LPVOID)((BYTE *)imageBase + section->PointerToRawData);
            //return (LPVOID)(section->PointerToRawData + imageBase);
    }
    
    return 0;
}

// LPVOID GetPtrFromRVA(DWORD rva, PIMAGE_NT_HEADERS pNTHeader, DWORD imageBase)
LPVOID GetPtrFromRVA( DWORD rva, PIMAGE_NT_HEADERS pNTHeader, BYTE *imageBase )
{
	PIMAGE_SECTION_HEADER pSectionHdr;
	INT delta;
		
	pSectionHdr = GetEnclosingSectionHeader( rva, pNTHeader );
	if ( !pSectionHdr )
		return 0;

	delta = (INT)(pSectionHdr->VirtualAddress-pSectionHdr->PointerToRawData);
	return (PVOID) ((BYTE *)imageBase + rva - delta );
}

//
// Given a section name, look it up in the section table and return a
// pointer to its IMAGE_SECTION_HEADER
//
PIMAGE_SECTION_HEADER GetSectionHeader(PSTR name, PIMAGE_NT_HEADERS pNTHeader)
{
    PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(pNTHeader);
    unsigned i;
    
    for ( i=0; i < pNTHeader->FileHeader.NumberOfSections; i++, section++ )
    {
        if ( 0 == strncmp((char *)section->Name,name,IMAGE_SIZEOF_SHORT_NAME) )
            return section;
    }
    
    return 0;
}

//
// Given an RVA, look up the section header that encloses it and return a
// pointer to its IMAGE_SECTION_HEADER
//
PIMAGE_SECTION_HEADER GetEnclosingSectionHeader(DWORD rva,
                                                PIMAGE_NT_HEADERS pNTHeader)
{
    PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(pNTHeader);
    unsigned i;
    
    for ( i=0; i < pNTHeader->FileHeader.NumberOfSections; i++, section++ )
    {
        // Is the RVA within this section?
        if ( (rva >= section->VirtualAddress) && 
             (rva < (section->VirtualAddress + section->Misc.VirtualSize)))
            return section;
    }
    
    return 0;
}

PIMAGE_COFF_SYMBOLS_HEADER PCOFFDebugInfo = 0;

char *SzDebugFormats[] = {
"UNKNOWN/BORLAND","COFF","CODEVIEW","FPO","MISC","EXCEPTION","FIXUP",
"OMAP_TO_SRC", "OMAP_FROM_SRC"};

//
// Dump the debug directory array
//
// void DumpDebugDirectory(PIMAGE_DEBUG_DIRECTORY debugDir, DWORD size, DWORD base)
void DumpDebugDirectory(PIMAGE_DEBUG_DIRECTORY debugDir, DWORD size, BYTE *base)
{
    DWORD cDebugFormats = size / sizeof(IMAGE_DEBUG_DIRECTORY);
    PSTR szDebugFormat;
    unsigned i;
    
    if ( cDebugFormats == 0 )
        return;
    
    SPRTF(
    "Debug Formats in File\n"
    "  Type            Size     Address  FilePtr  Charactr TimeDate Version\n"
    "  --------------- -------- -------- -------- -------- -------- --------\n"
    );
    
    for ( i=0; i < cDebugFormats; i++ )
    {
        szDebugFormat = (debugDir->Type <= IMAGE_DEBUG_TYPE_OMAP_FROM_SRC )
                        ? SzDebugFormats[debugDir->Type] : "???";

        SPRTF("  %-15s %08X %08X %08X %08X %08X %u.%02u\n",
            szDebugFormat, debugDir->SizeOfData, debugDir->AddressOfRawData,
            debugDir->PointerToRawData, debugDir->Characteristics,
            debugDir->TimeDateStamp, debugDir->MajorVersion,
            debugDir->MinorVersion);

		switch( debugDir->Type )
		{
        	case IMAGE_DEBUG_TYPE_COFF:
	            g_pCOFFHeader =
                (PIMAGE_COFF_SYMBOLS_HEADER)(base+ debugDir->PointerToRawData);
				break;

			case IMAGE_DEBUG_TYPE_MISC:
				g_pMiscDebugInfo =
				(PIMAGE_DEBUG_MISC)(base + debugDir->PointerToRawData);
				break;

			case IMAGE_DEBUG_TYPE_CODEVIEW:
				g_pCVHeader = (PDWORD)(base + debugDir->PointerToRawData);
				break;
		}

        debugDir++;
    }
}

/*----------------------------------------------------------------------------*/
//
// Other assorted stuff
//
/*----------------------------------------------------------------------------*/

//
// Do a hexadecimal dump of the raw data for all the sections.  You
// could just dump one section by adjusting the PIMAGE_SECTION_HEADER
// and cSections parameters
//
void DumpRawSectionData(PIMAGE_SECTION_HEADER section,
                        PVOID base,
                        unsigned cSections)
{
    unsigned i;
    char name[IMAGE_SIZEOF_SHORT_NAME + 1];

    SPRTF("Section Hex Dumps - %u sections\n\n", cSections);
    
    //for (i = 1; i <= cSections; i++, section++)
    for ( i=0; i < cSections; i++, section++ )
    {
        if (IsAddressInRange((BYTE *)section, (int)sizeof(IMAGE_SECTION_HEADER))) {
            // Make a copy of the section name so that we can ensure that
            // it's null-terminated
            memcpy(name, section->Name, IMAGE_SIZEOF_SHORT_NAME);
            name[IMAGE_SIZEOF_SHORT_NAME] = 0;

            SPRTF("section %02X (%s)  size: %08X  file offs: %08X\n",
                (i + 1), name, section->SizeOfRawData, section->PointerToRawData);

            // Don't dump sections that don't exist in the file!
            if ((section->PointerToRawData == 0) || (section->SizeOfRawData == 0)) {
                SPRTF("\n");
                continue;
            }

            PBYTE pb = MakePtr(PBYTE, base, section->PointerToRawData);
            DWORD dwSz = section->SizeOfRawData;
            DWORD dwMax = (dwSz ? (dwSz - 1) : 0);
            if (IsAddressInRange(pb, dwMax)) {
                HexDump(pb, dwSz);
            }
            else {
                SPRTF("TODO: Skipped Hex Dump %u of %u - PBYTE out of range - %p\n", (i+1), cSections, pb);
                // break;
            }
            SPRTF("\n");

        }
        else {
            SPRTF("TODO: Abandon Hex Dump %u of %u - PIMAGE_SECTION_HEADER out of range - %p\n\n", (i+1), cSections, section);
            break;
        }
    }
}

// Number of hex values displayed per line
#define HEX_DUMP_WIDTH 16

//
// Dump a region of memory in a hexadecimal format
//
void HexDump(PBYTE ptr, DWORD length)
{
    char buffer[256];
    PSTR buffPtr, buffPtr2;
    unsigned cOutput, i;
    DWORD bytesToGo=length;

    while ( bytesToGo  )
    {
        cOutput = bytesToGo >= HEX_DUMP_WIDTH ? HEX_DUMP_WIDTH : bytesToGo;

        buffPtr = buffer;
        buffPtr += sprintf(buffPtr, "%08X:  ", length-bytesToGo );
        buffPtr2 = buffPtr + (HEX_DUMP_WIDTH * 3) + 1;
        
        for ( i=0; i < HEX_DUMP_WIDTH; i++ )
        {
            BYTE value = *(ptr+i);

            if ( i >= cOutput )
            {
                // On last line.  Pad with spaces
                *buffPtr++ = ' ';
                *buffPtr++ = ' ';
                *buffPtr++ = ' ';
            }
            else
            {
                if ( value < 0x10 )
                {
                    *buffPtr++ = '0';
                    itoa( value, buffPtr++, 16);
                }
                else
                {
                    itoa( value, buffPtr, 16);
                    buffPtr+=2;
                }
 
                *buffPtr++ = ' ';
                *buffPtr2++ = isprint(value) ? value : '.';
            }
            
            // Put an extra space between the 1st and 2nd half of the bytes
            // on each line.
            if ( i == (HEX_DUMP_WIDTH/2)-1 )
                *buffPtr++ = ' ';
        }

        *buffPtr2 = 0;  // Null terminate it.
        // Can't use SPRTF(), since there may be a '%'
        // in the string.
        strcat(buffer, "\n");
        direct_out_it(buffer);
        //puts(buffer);   
        bytesToGo -= cOutput;
        ptr += HEX_DUMP_WIDTH;
    }
}

DWORD GetImgDirEntryRVA(PVOID pNTHdr, DWORD IDE)
{
    DWORD rva = 0;
    if (Is32) {
        PIMAGE_NT_HEADERS32 p32 = (PIMAGE_NT_HEADERS32)pNTHdr;
        rva = p32->OptionalHeader.DataDirectory[IDE].VirtualAddress;
    }
    else {
        PIMAGE_NT_HEADERS64 p64 = (PIMAGE_NT_HEADERS64)pNTHdr;
        rva = p64->OptionalHeader.DataDirectory[IDE].VirtualAddress;
    }
    return rva;
}

DWORD GetImgDirEntrySize(PVOID pNTHdr, DWORD IDE)
{
    DWORD size = 0;
    if (Is32) {
        PIMAGE_NT_HEADERS32 p32 = (PIMAGE_NT_HEADERS32)pNTHdr;
        size = p32->OptionalHeader.DataDirectory[IDE].Size;
    }
    else {
        PIMAGE_NT_HEADERS64 p64 = (PIMAGE_NT_HEADERS64)pNTHdr;
        size = p64->OptionalHeader.DataDirectory[IDE].Size;
    }
    return size;

}

/* eof */

