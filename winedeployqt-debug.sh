#/bin/bash

# path to where MSVC 2022 version of Qt is, either
# brought over from Windows or installed with 
# aqtinstall (https://github.com/miurahr/aqtinstall)

QTDIR='/opt/Qt'
QTVERS='6.10.1'

# directory where exe is, where windeployqt will put all
# the dlls and Qt runtime files
DISTDIR="build/MSVC2022Debug/dist"

# optional, place where VC runtime files are placed. To run
# QtAgOpenGPS.exe, you will need the MS VC runtime 14.0
# installed into wine, or the dlls copied into the DISTDIR
# folder.

#place where VC_redist.x64.exe is
VCINSTALLDIR=/opt/xwin/vcredist 

QMLDIR='qml'

wine "$QTDIR/$QTVERS/msvc2022_64/bin/windeployqt6.exe" --qmldir "$QMLDIR" --debug "$DISTDIR/QtAgOpenGPS.exe"
