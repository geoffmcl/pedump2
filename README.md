PEDUMP2 Project
---------------

This is a fork and update of Matt Pietrek PEDUMP, pedump.zip from [downloads](http://www.wheaty.net/downloads.htm), circa 1998. The update included handling the 64-bit PE files.

This project is mainly for **fun** and educational purposes. I have always had an interest in the format of an executable file... 

#### Building

This project uses [CMake](https://cmake.org/) to generate supported native build files. With a version of MSVC installed this is as simple as -

 - cd build
 - cmake .. [options]
 - cmake --build . --config Release
 
There is also a `build-me.bat` which may be of use to you, maybe with some small local modifications to suit your specfic environment. It is presently setup for MSVC 14 in x64 mode...

#### Original ReadMe.txt by Matt Pietrek

This is my PEDUMP code as January 1998.  It's certainly better than the original PEDUMP from my MSJ column, and from my "Windows 95 System Programming Secrets" book.  There are additions for several new file types, and additional resources.

Having said that, this is by no means the best PEDUMP sources I have.  I've made extensive additions since then for things like PE32+ (Win64) support.  I plan to release them publicly as part of future article in MSDN magazine.   This article will essentially be an update of my 1994 article on the PE format.  There have been many exciting changes since then, and I'll cover them, as well as correct some errors in the original.

In the meanwhile, enjoy this code.  I know there are some bugs in a few places. (Yes, I know that there is a new .LIB file format, for instance.)  These issues are  most likely fixed in the version I have locally.  Comments are welcome, but I don't expect to actively work on this branch of my PEDUMP code.

You could build the sources with the enclosed PEDUMP.MAK file, but this has been removed in this **fork**. As shown above it now used the CMake build file generator, but it is not really a cross-ported product, in that it included the `Windows.h` header.

Enjoy...  
Geoff.  
2018-01-12

; eof
