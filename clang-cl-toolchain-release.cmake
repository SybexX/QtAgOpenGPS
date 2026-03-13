# CMake toolchain file for cross-compiling to Windows using clang-cl (RELEASE)
# with Windows SDK installed via xwin
#
# Usage:
#   cmake -DCMAKE_TOOLCHAIN_FILE=/opt/xwin/clang-cl-toolchain-release.cmake \
#         -DCMAKE_PREFIX_PATH=/opt/Qt/6.10.1/llvm-mingw_64 \
#         -S . -B build-release

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)

# Force Release build type
set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)

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
)

# Use dynamic release runtime (/MD)
add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:/MD>)

# Optional: Add security flags (recommended for production)
# Uncomment these if needed:
# add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:/GS>)        # Buffer security check
# add_link_options(/DYNAMICBASE)                                # ASLR (Address Space Layout Randomization)
# add_link_options(/NXCOMPAT)                                   # DEP (Data Execution Prevention)
# add_link_options(/guard:cf)                                   # Control Flow Guard

# Optional: Architecture-specific optimizations
# add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:/arch:AVX2>) # Use AVX2 instructions

# Use Linux Qt host tools (moc, rcc, uic) for cross-compilation
set(QT_HOST_PATH "/opt/Qt/6.10.1/gcc_64" CACHE PATH "Qt host tools path")
