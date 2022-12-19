# ==================== CHANGE START =============================
set(APPNAME "cnfgtest")
set(LABEL ${APPNAME})
set(APKFILE ${APPNAME}.apk)
set(PACKAGENAME com.tanmayvijay.${APPNAME})
set(ANDROIDVERSION 29)
set(ANDROIDTARGET ${ANDROIDVERSION})
set(UNAME Darwin)
# ==================== CHANGE END ===============================

MACRO(SUBDIRLIST result curdir)
  FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
  SET(dirlist "")
  FOREACH(child ${children})
    IF(IS_DIRECTORY ${curdir}/${child})
      LIST(APPEND dirlist ${child})
    ENDIF()
  ENDFOREACH()
  SET(${result} ${dirlist})
ENDMACRO()
set(ANDROIDSDK $ENV{ANDROID_HOME})
SUBDIRLIST(SUBDIRS ${ANDROIDSDK}/ndk)
list(LENGTH SUBDIRS list_len)
math(EXPR list_last "${list_len} - 1")
list(GET SUBDIRS ${list_last} NDK)
set(NDK "${ANDROIDSDK}/ndk/${NDK}")
SUBDIRLIST(SUBDIRS ${ANDROIDSDK}/build-tools)
math(EXPR list_last "${list_len} - 1")
list(GET SUBDIRS ${list_last} BUILD_TOOLS)
set(BUILD_TOOLS "${ANDROIDSDK}/build-tools/${BUILD_TOOLS}")


message("ANDROIDSDK: ${ANDROIDSDK}")
message("NDK: ${NDK}")
message("BUILD_TOOLS: ${BUILD_TOOLS}")
message("CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")


set(CMAKE_C_FLAGS "-ffunction-sections -Os -fdata-sections -Wall -fvisibility=hidden -m64 -DBUILD_SHARED_LIBS=OFF")
set(CMAKE_CXX_FLAGS "-ffunction-sections -Os -fdata-sections -Wall -fvisibility=hidden -m64 -DBUILD_SHARED_LIBS=OFF")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os -DANDROID -DAPPNAME=\\\"${APPNAME}\\\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os -DANDROID -DAPPNAME=\\\"${APPNAME}\\\"")
set(ANDROID_FULLSCREEN y)

if(${UNAME} STREQUAL "Darwin")
  set(OS_NAME darwin-x86_64)
endif()

if(${UNAME} STREQUAL "Linux")
  set(OS_NAME linux-x86_64)
endif()

if(${UNAME} STREQUAL "Windows_NT")
  set(OS_NAME windows-x86_64)
endif()

set(CC_ARM64 ${NDK}/toolchains/llvm/prebuilt/${OS_NAME}/bin/aarch64-linux-android${ANDROIDVERSION}-clang)
set(CC_ARM32 ${NDK}/toolchains/llvm/prebuilt/${OS_NAME}/bin/armv7a-linux-androideabi${ANDROIDVERSION}-clang)
set(CC_x86 ${NDK}/toolchains/llvm/prebuilt/${OS_NAME}/bin/i686-linux-android${ANDROIDVERSION}-clang)
set(CC_x86_64 ${NDK}/toolchains/llvm/prebuilt/${OS_NAME}/bin/x86_64-linux-android${ANDROIDVERSION}-clang)
set(AAPT ${BUILD_TOOLS}/aapt)

set(CMAKE_CXX_COMPILER ${CC_ARM64})
set(CMAKE_C_COMPILER ${CC_ARM64})
include_directories(${NDK}/sysroot/usr/include ${NDK}/sysroot/usr/include/android ${NDK}/toolchains/llvm/prebuilt/${OS_NAME}/sysroot/usr/include/android ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

