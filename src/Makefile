
all: label.exe

ifndef COMPILER
COMPILER:=watcom
endif


# misc and non portable shell commands needed
# trick to auto determine OS, assumes PATH env var set with at least 2 components (i.e. ; or : separated)
ifneq '$(findstring ;,$(PATH))' ';'
LINUX:=1
# Note: ensure no extra whitespace after DIRSEP
DIRSEP=/
RM=@rm -f
CP=cp
else
# Windows or DOS
# Note: ensure no extra whitespace after DIRSEP
DIRSEP=\\
ifeq ($(OS),Windows_NT)     # is Windows_NT on XP, 2000, 7, Vista, 10...
# Windows specific
WIN32:=1
RM=@del
else
# DOS  specific
#RM=$($(ROOTPATH)utils/rmfiles.bat"
RM=@rm -f
endif
CP=copy
endif


ifeq ($(COMPILER),watcom)
# Watcom
# -0 generate code for 8086+
# -bt=DOS compile for DOS
# -bcl=DOS link for DOS
# -s remove statck overflow checks
# -zp1 pack structures align on byte
# -we treat all warnings as errors
# -wx set warning level to max
# -zq operate quietly
# -fm[=map_file]  generate map file
# -fe=executable  name executable file
# -m{t,s,m,c,l,h}  memory model
CC=wcl
EXEFLAGS?=-mt
CFLAGS?=-oas -bt=DOS -bcl=DOS -D__MSDOS__ -zp1 -s -0 -wx -we -zq -fm $(EXEFLAGS) -fe=
LDFLAGS?=
endif

ifeq ($(COMPILER),borland)
# Borland
# -w warn -M create map -f- no floating point -Z register optimize
# -O jump optimize -k- no standard stack frome -K unsigned char
# -exxx executable name (must be last) -mt tiny (default is small)
# -N stack checking -a- byte alignment  -ln no default libs
# -lt create .com file -lx no map file ...
# tiny has near qsort / malloc pointers, very limited!
# compact large: large has far function pointers, compact only far heap.
# makes no real difference here, but compact saves a few bytes in size.
# tcc looks for includes from the current directory, not the location of the
# file that's trying to include them, so add kitten's location -I
ifeq ($(TURBO),0)
CC=bcc
else
CC=tcc
endif
EXEFLAGS?=-mt -Z -O -k-
CFLAGS?=-I../kitten -w -M -f- -a- -K -ln $(EXEFLAGS) -e
LDFLAGS?=
endif

ifeq ($(COMPILER),gcc)
ifdef LINUX
#long name version
CC=ia16-elf-gcc
OBJCOPY?=ia16-elf-objcopy
else
#DOS short name version
CC=i16gcc
OBJCOPY?=i16objco
endif
#EXEFLAGS=-ffreestanding -mrtd 
EXEFLAGS?=
LDFLAGS?=-li86
CFLAGS?=-I../kitten -mcmodel=small  -fpack-struct -Werror -Os -fno-strict-aliasing $(EXEFLAGS) -o 
endif


UPX?=upx --8086
# if you don't want to use UPX set
#     UPX=-rem (DOS)
#   or
#     UPX=true (Unix)
# if you use UPX: then options are
#     --8086 for 8086 compatibility
#   or
#     --best for smallest


ifneq ($(findstring NOCATS,$(CFLAGS)),)
	KITTEN:=
else
	KITTEN:=..$(DIRSEP)kitten$(DIRSEP)kitten.c
endif

label.exe _label.exe: prf.c label.c $(KITTEN)
	$(CC) $(CFLAGS)$@ $^ $(LDFLAGS)
	-$(UPX) $@

clean:
	$(RM) *.exe
	$(RM) *.o
	$(RM) *.obj
	$(RM) *.map
	$(RM) *.err

