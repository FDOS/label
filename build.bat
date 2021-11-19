@echo off

if "%1"=="NOCATS" set NOCATS=-DNOCATS
if "%1"=="NOCATS" shift
if "%1"=="" goto usage
goto %1

:usage
echo Please run build compiler
echo where compiler is one of watcom or tcc
echo e.g.  build watcom
echo Optional pass NOCATS to build without cats
echo e.g.  build NOCATS tcc
goto done

:watcom
rem ############# WATCOM ########################
set PATH=C:\bin;C:\watcom\binw;%PATH%
rem # -we treat all warnings as errors
rem # -wx set warning level to max
rem # -zq operate quietly
rem # -fm[=map_file]  generate map file
rem # -fe=executable  name executable file
rem # -m{t,s,m,c,l,h}  memory model

if "%WATCOM%"=="" set WATCOM=C:\watcom
if "%INCLUDE%"=="" set INCLUDE=%WATCOM%\h
set COMPILER=watcom
set EXEFLAGS=-mt
set CFLAGS=-oas -bt=DOS -bcl=DOS -D__MSDOS__ -zp1 -s -0 -wx -we -zq -fm %EXEFLAGS% -fe=
set TARGET=label.exe
goto doit

:tcc
rem ############# TURBO_C ########################
set PATH=C:\bin;C:\tc\bin;%PATH%
rem # -w warn -M create map -f- no floating point -Z register optimize
rem # -O jump optimize -k- no standard stack frome -K unsigned char
rem # -exxx executable name (must be last) -mt tiny (default is small)
rem # -N stack checking -a- byte alignment  -ln no default libs
rem # -lt create .com file -lx no map file ...
rem # tiny has near qsort / malloc pointers, very limited!
rem # compact large: large has far function pointers, compact only far heap.
rem # makes no real difference here, but compact saves a few bytes in size.

set COMPILER=borland
rem change to 0 if using Borland C/C++ instead of Turbo C/C++
if "%TURBO%"=="" set TURBO=1
set EXEFLAGS=-mt -Z -O -k-
set CFLAGS=-w -M -f- -a- -K -ln %EXEFLAGS% -e
rem tcc looks for includes from the current directory, not the location of the
rem file that's trying to include them, so add kitten's location
set CFLAGS=-I../kitten %CFLAGS%
set TARGET=label.exe
goto doit

:doit
rem # if you want to build without kitten call with NOCATS before compiler
set CFLAGS=%NOCATS% %CFLAGS%
if not "%NOCATS%"=="" set TARGET=_%TARGET%

rem We use GNU make for all targets
make -C src %TARGET%

:done
rem we clear all these env variables (even if set prior to calling)
rem do do not clear TURBO, WATCOM, or INCLUDE if set
set TARGET=
set COMPILER=
set CFLAGS=
set EXEFLAGS=
set NOCATS=
