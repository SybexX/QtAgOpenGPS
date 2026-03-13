#ifndef RENDERING_H
#define RENDERING_H

#include <qglobal.h>

// For Windows and Android, use direct openGL rendering using
// the QML engine's GL context
#if defined(Q_OS_WINDOWS)
    //use direct rendering
#else
#  if defined(Q_OS_ANDROID)
#     undef USE_INDIRECT_RENDERING //use direct on android
#  else //not windows and not android
     //use indirec rendering for all other platforms
#    define USE_INDIRECT_RENDERING
#endif
#endif

// If you want to use the QSGRenderNode-based AOGRendererItem,
// uncomment this #define:
//#define USE_QSGRENDERNODE 1

#endif // RENDERING_H
