# Fork and minor update of PEDUMP, by Matt Pietrek 1994-1998
# CMakeLists.txt, generated gencmake.pl, on 2017/10/15 16:13:13
cmake_minimum_required( VERSION 2.8.8 )

# CMakeScripts or use the ones that come by default with CMake.
# set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMakeModules ${CMAKE_MODULE_PATH})

project( pedump2 )

#------------------------------------------------------------------------
# The version number.
# Release Information
#   Release version and date are found in `version.txt`; update *that*
#   file when required. It will be read into variable `versionFile`
#   (stripping any newlines or spaces). This file must be formatted into
#   two lines: the dot-separated MAJOR.MINOR.POINT version, followed by
#   the date separated YEAR.MONTH.DAY release date.
#------------------------------------------------------------------------
file(READ version.txt versionFile)
if (NOT versionFile)
    message(FATAL_ERROR "Unable to determine pedump version. version.txt file is missing.")
endif()

string(STRIP "${versionFile}" VERSION_TEXT)
string(REGEX REPLACE "(.*)[\r\n|\n](.*)" "\\1" PED_VERSION ${VERSION_TEXT})
string(REGEX REPLACE "(.*)[\r\n|\n](.*)" "\\2" PED_DATE ${VERSION_TEXT})

# Establish version number
if (PED_VERSION)
    string(REPLACE "." ";" VERSION_LIST ${PED_VERSION})
    list(GET VERSION_LIST 0 PED_MAJOR_VERSION)
    list(GET VERSION_LIST 1 PED_MINOR_VERSION)
    list(GET VERSION_LIST 2 PED_POINT_VERSION)
else ()
    message(FATAL_ERROR "*** FAILED to get a VERSION from version.txt!")
endif ()   
 
# Establish version date
if (PED_DATE)
    string(REPLACE "." ";" DATE_LIST ${PED_DATE})
    list(GET DATE_LIST 0 PED_YEAR)
    list(GET DATE_LIST 1 PED_MONTH)
    list(GET DATE_LIST 2 PED_DAY)
else ()
    message(FATAL_ERROR "*** FAILED to get a DATE from version.txt!")
endif ()

# Allow developer to select is Dynamic or static library built
set( LIB_TYPE STATIC )  # set default static
option( BUILD_SHARED_LIB "Set ON to build shared Library (DLL)" OFF )


if(CMAKE_COMPILER_IS_GNUCXX)
    set( WARNING_FLAGS -Wall )
endif(CMAKE_COMPILER_IS_GNUCXX)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang") 
   set( WARNING_FLAGS "-Wall -Wno-overloaded-virtual" )
endif() 

if(WIN32 AND MSVC)
    # turn off various warnings - none needed in this compile
    set(WARNING_FLAGS "${WARNING_FLAGS} /wd4996")
    # foreach(warning 4244 4251 4267 4275 4290 4786 4305)
    #     set(WARNING_FLAGS "${WARNING_FLAGS} /wd${warning}")
    # endforeach(warning)
    # exclude -DNOMINMAX 
    set( MSVC_FLAGS "-D_USE_MATH_DEFINES -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS -D__CRT_NONSTDC_NO_WARNINGS" )
    # to distinguish between debug and release libs
    set( CMAKE_DEBUG_POSTFIX "d" )
    list(APPEND add_LIBS Ws2_32.lib Dbghelp.lib)
else()
    # items for unix
endif()

set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARNING_FLAGS} ${MSVC_FLAGS} -D_REENTRANT" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARNING_FLAGS} ${MSVC_FLAGS} -D_REENTRANT" )
set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${MSVC_LD_FLAGS}" )

add_definitions ( -DPED_VERSION="${PED_VERSION}" )
add_definitions ( -DPED_DATE="${PED_YEAR}/${PED_MONTH}/${PED_DAY}" )

# configuration file, if needed
# configure_file( ${CMAKE_SOURCE_DIR}/config.h.cmake ${CMAKE_BINARY_DIR}/config.h )
# add_definitions( -DHAVE_CONFIG_H )
# include_directories( ${CMAKE_BINARY_DIR} )

#------------------------------------------------------------------------
# Static Windows Runtime
#   Option to statically link to the Windows runtime. Maybe only 
#   applies to WIN32/MSVC.
#------------------------------------------------------------------------
if (MSVC)
    option( USE_STATIC_RUNTIME "Set ON to change /MD(DLL) to /MT(static)" ON )
    if (USE_STATIC_RUNTIME)
        set(CompilerFlags
            CMAKE_CXX_FLAGS
            CMAKE_CXX_FLAGS_DEBUG
            CMAKE_CXX_FLAGS_RELEASE
            CMAKE_C_FLAGS
            CMAKE_C_FLAGS_DEBUG
            CMAKE_C_FLAGS_RELEASE
            )
        foreach(CompilerFlag ${CompilerFlags})
            string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
        endforeach()    
        message(STATUS "Using /MT STATIC runtime")
    else ()
        message(STATUS "Using /MD DYNAMIC runtime")
    endif ()
endif ()


if(BUILD_SHARED_LIB)
   set(LIB_TYPE SHARED)
   message(STATUS "*** Building DLL library ${LIB_TYPE}")
else()
   message(STATUS "*** Option BUILD_SHARED_LIB is OFF ${LIB_TYPE}")
endif()
# Total libs 1, exes 1
#################################################
##### LIBRARIES 1 #####
#################################################
set (name utilslib)
set (dir utils)
include_directories( ${dir} )
set(${name}_SRCS
    ${dir}/sprtf.cxx
    ${dir}/utils.cxx
    )
set(${name}_HDRS
    ${dir}/sprtf.hxx
    ${dir}/utils.hxx
    )
add_library( ${name}  ${LIB_TYPE} ${${name}_SRCS} ${${name}_HDRS} )
list(APPEND add_LIBS ${name})
# deal with install, if any...
#install( TARGETS ${name}
#    RUNTIME DESTINATION bin
#    LIBRARY DESTINATION lib
#    ARCHIVE DESTINATION lib )
#install(FILES ${${name}_HDRS} DESTINATION include)

#################################################
##### EXECUTABLE 1 #####
#################################################

# pedump EXECUTABLE from [F:\Projects\pedump],
# have 10 C/C++ sources, 11 headers
set(name pedump2)
set(dir src)
set(${name}_SRCS
    ${dir}/common.cpp
    ${dir}/dbgdump.cpp
    ${dir}/exedump.cpp
    ${dir}/libdump.cpp
    ${dir}/objdump.cpp
    ${dir}/pedump.cpp
    ${dir}/coffsymboltable.cpp
    ${dir}/resdump.cpp
    ${dir}/romimage.cpp
    ${dir}/symboltablesupport.cpp
    ${dir}/pdbdump.cpp
    )
set(${name}_HDRS
    ${dir}/common.h
    ${dir}/dbgdump.h
    ${dir}/exedump.h
    ${dir}/extrnvar.h
    ${dir}/libdump.h
    ${dir}/objdump.h
    ${dir}/romimage.h
    ${dir}/coffsymboltable.h
    ${dir}/cv_dbg.h
    ${dir}/resdump.h
    ${dir}/symboltablesupport.h
    ${dir}/pdbdump.h
    )
add_executable( ${name} ${${name}_SRCS} ${${name}_HDRS} )
if (add_LIBS)
    target_link_libraries( ${name} ${add_LIBS} )
endif ()
if (MSVC)
    set_target_properties( ${name} PROPERTIES DEBUG_POSTFIX d )
endif ()
# deal with install, if any...
#install( TARGETS ${name} DESTINATION bin )

# eof
