#!/bin/bash

#install MS windows 11 SDK using xwin (https://crates.io/crates/xwin)
XWIN_ROOT="/opt/xwin"

# path to where MSVC 2022 version of Qt is, either
# brought over from Windows or installed with 
# aqtinstall (https://github.com/miurahr/aqtinstall)

QTDIR='/opt/Qt'
QTVERS='6.10.1'

#location of native linux QT
QTHOSTDIR='/opt/Qt'
QTHOSTVERS='6.10.1'

# debug build
CLANG_CL_TOOLCHAIN_CMAKE="clang-cl-toolchain-debug.cmake"

BUILDDIR="build/MSVC2022Debug"

cmake -DCMAKE_TOOLCHAIN_FILE="$CLANG_CL_TOOLCHAIN_CMAKE" \
        -DLOCAL_QML=on -DWIN32_EXECUTABLE=off -DCMAKE_PREFIX_PATH="$QTDIR/$QTVERS/msvc2022_64" \
        -DQT_HOST_PATH="$QTHOSTDIR/$QTHOSTVERS/gcc_64" \
        -S . -B "$BUILDDIR"
