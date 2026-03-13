# CMake toolchain file for cross-compiling to Windows using clang-cl (DEBUG)
# with Windows SDK installed via xwin
#
# Usage:
#   cmake -DCMAKE_TOOLCHAIN_FILE=/opt/xwin/clang-cl-toolchain-debug.cmake \
#         -DCMAKE_PREFIX_PATH=/opt/Qt/6.10.1/llvm-mingw_64 \
#         -S . -B build-debug

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)

# Force Debug build type
set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)

# Compiler configuration
set(CMAKE_C_COMPILER clang-cl)
set(CMAKE_CXX_COMPILER clang-cl)
set(CMAKE_RC_COMPILER llvm-rc)
set(CMAKE_LINKER lld-link)

# Windows SDK root
set(XWIN_ROOT "/opt/xwin")

# Set sysroot to prevent using Linux system headers
set(CMAKE_SYSROOT "${XWIN_ROOT}/crt")

# Target architecture (x86_64)
set(XWIN_ARCH "x86_64")

# Windows SDK include directories
set(WINSDK_INCLUDE_DIRS
    ${XWIN_ROOT}/crt/include
    ${XWIN_ROOT}/sdk/include/ucrt
    ${XWIN_ROOT}/sdk/include/um
    ${XWIN_ROOT}/sdk/include/shared
    ${XWIN_ROOT}/sdk/include/winrt
    ${XWIN_ROOT}/sdk/include/cppwinrt
)

# Windows SDK library directories
set(WINSDK_LIB_DIRS
    ${XWIN_ROOT}/crt/lib/${XWIN_ARCH}
    ${XWIN_ROOT}/sdk/lib/ucrt/${XWIN_ARCH}
    ${XWIN_ROOT}/sdk/lib/um/${XWIN_ARCH}
)

# Prevent using Linux system includes
add_compile_options($<$<COMPILE_LANGUAGE:C>:-nostdinc>)
# Note: -nostdinc++ removed as it's not recognized by clang-cl

# Add include directories using -imsvc flag (for clang-cl)
foreach(dir ${WINSDK_INCLUDE_DIRS})
    add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:-imsvc${dir}>)
endforeach()

# Add library directories
foreach(dir ${WINSDK_LIB_DIRS})
    add_link_options(/LIBPATH:${dir})
endforeach()

# Set target triple for clang
set(CMAKE_C_COMPILER_TARGET x86_64-pc-windows-msvc)
set(CMAKE_CXX_COMPILER_TARGET x86_64-pc-windows-msvc)

# Ensure we find libraries and includes for Windows
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)

# Additional flags for Windows compatibility
add_compile_options(
    $<$<COMPILE_LANGUAGE:C,CXX>:-fms-compatibility-version=19.44>
    $<$<COMPILE_LANGUAGE:C,CXX>:-Wno-unused-command-line-argument>
    $<$<COMPILE_LANGUAGE:CXX>:-D_SILENCE_CLANG_CONCEPTS_MESSAGE>
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-ignored-pragma-intrinsic>
)

# Use dynamic debug runtime (/MDd)
add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:/MDd>)

# Debug symbol generation
# /Zi - Generate complete debug information (PDB file)
# /Z7 - Generate debug info in .obj files (alternative, no PDB)
add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:/Zi>)

# Debug linker flags
# /DEBUG - Generate debug information
# /OPT:NOREF - Don't remove unreferenced functions/data (keeps all symbols)
# /OPT:NOICF - Don't perform identical COMDAT folding (better debugging)
add_link_options(/DEBUG /OPT:NOREF /OPT:NOICF)

# Use Linux Qt host tools (moc, rcc, uic) for cross-compilation
set(QT_HOST_PATH "/opt/Qt/6.10.1/gcc_64" CACHE PATH "Qt host tools path")
