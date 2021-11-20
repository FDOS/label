#!/bin/sh

if [ x"${COMPILER}" = "xgcc" ] ; then
  # Note requires installation of libi86-ia16-elf DOS compat library
  export CC="ia16-elf-gcc"
  export CFLAGS="-Wall -fpack-struct -mcmodel=small -Os -o "
  export LDFLAGS="-li86"
  TARGET="label.exe"

elif [ x"${COMPILER}" = "xwatcom" ] ; then
  if [ -z "${WATCOM}" ] ; then
    # Make sure this is set correctly for your system before running
    # this script, or we default to this
    export WATCOM="/opt/watcom"
  fi
  export PATH=${PATH}:${WATCOM}/binl64
  export INCLUDE=${WATCOM}/h
  export CC="wcl"
  export CFLAGS="-bt=DOS -bcl=DOS -D__MSDOS__ -zp1 -ms -lr -fe="
  export LDFLAGS=""
  TARGET="label.exe"

elif [ x"${COMPILER}" = "xwatcom-emu" ] ; then
  dosemu -q -td -K . -E "build.bat watcom"
  exit $?

elif [ x"${COMPILER}" = "xtcc-emu" ] ; then
  dosemu -q -td -K . -E "build.bat tcc"
  exit $?

else
  echo "Please set the COMPILER env var to one of"
  echo "Cross compile           : 'watcom' or 'gcc'"
  echo "Native compile (Dosemu) : 'watcom-emu' or 'tcc-emu'"
  exit 1
fi

# if you want to build without kitten uncomment the following
# export CFLAGS="-DNOCATS ${CFLAGS}"
# TARGET=_${TARGET}

make -C src ${TARGET}
