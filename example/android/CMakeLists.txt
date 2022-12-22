LIST(APPEND SRC 
  mytest.cc 
  android_out.cc 
  android_native_app_glue.c)

set(APPNAME tanmay)
set(LABEL ${APPNAME})
set(APKFILE ${APPNAME}.apk)
set(PACKAGENAME com.tanmayvijay.${APPNAME})
set(ANDROIDVERSION 30)
set(ANDROIDTARGET ${ANDROIDVERSION})
set(UNAME Darwin)

add_library(${APPNAME} MODULE ${SRC})

set_target_properties(${APPNAME} PROPERTIES LINKER_LANGUAGE CXX)
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
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")

message("CMake c flags ${CMAKE_C_FLAGS}")
include_directories(${NDK}/sysroot/usr/include ${NDK}/sysroot/usr/include/android ${NDK}/toolchains/llvm/prebuilt/${OS_NAME}/sysroot/usr/include/android ${CMAKE_CURRENT_SOURCE_DIR})
# target_include_directories(${APPNAME} PRIVATE rawdraw ${CMAKE_CURRENT_SOURCE_DIR})
# set(LIB_PATH ${NDK}/toolchains/llvm/prebuilt/${OS_NAME}/sysroot/usr/lib/x86_64-linux-android/${ANDROIDVERSION})
# target_link_libraries(${APPNAME} m GLESv3 EGL android log -shared -uANativeActivity_onCreate -landroid stdc++ -static-libstdc++)
target_link_libraries(${APPNAME} m GLESv3 EGL android log bob_ross_gles3)
# find_package(game-activity REQUIRED CONFIG)

set(KEYSTOREFILE my-release-key.keystore)
set(ALIASNAME standkey)
set(DNAME "CN=example.com, OU=ID, O=Example, L=Doe, S=John, C=GB")
set(STOREPASS password)

add_custom_target(keystore ALL 
  COMMAND keytool -genkey -v -keystore ${KEYSTOREFILE} -alias ${ALIASNAME} -keyalg RSA -keysize 2048 -validity 10000 -storepass password -keypass password -dname ${DNAME} || exit 0
  COMMENT "GENERATING keystorefile"
  VERBATIM)

# TODO: Fix the dependency, custom target should only run when testlib is changed
add_custom_target(manifest ALL
  COMMAND PACKAGENAME=${PACKAGENAME} ANDROIDVERSION=${ANDROIDVERSION} ANDROIDTARGET=${ANDROIDTARGET} APPNAME=${APPNAME} LABEL=${LABEL} envsubst '$$ANDROIDTARGET $$ANDROIDVERSION $$APPNAME $$PACKAGENAME $$LABEL' < ${CMAKE_CURRENT_SOURCE_DIR}/AndroidManifest.xml.template > AndroidManifest.xml)

add_custom_target(intermediate_apk ALL
  COMMAND ${CMAKE_COMMAND} -E make_directory apk/assets
  COMMAND ${CMAKE_COMMAND} -E make_directory apk/lib/arm64-v8a
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/src/assets ${CMAKE_CURRENT_BINARY_DIR}/apk/assets
  # COMMAND ${CMAKE_COMMAND} -E copy lib${APPNAME}.dylib apk/lib/arm64-v8a/lib${APPNAME}.dylib
  COMMAND ${CMAKE_COMMAND} -E copy lib${APPNAME}.so apk/lib/arm64-v8a/lib${APPNAME}.so
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/opengles2/libbob_ross_gles3.so apk/lib/arm64-v8a/libbob_ross_gles3.so
  COMMAND ${BUILD_TOOLS}/aapt package -f -F temp.apk -I ${ANDROIDSDK}/platforms/android-${ANDROIDVERSION}/android.jar -M AndroidManifest.xml -S ${CMAKE_CURRENT_SOURCE_DIR}/src/res -A apk/assets -v --target-sdk-version ${ANDROIDTARGET}
  COMMAND unzip -o temp.apk -d apk
  DEPENDS manifest
  COMMENT "Generating intermediate apk")

add_dependencies(intermediate_apk ${APPNAME})
add_custom_target(zip_apk ALL
  DEPENDS intermediate_apk
  COMMAND ${CMAKE_COMMAND} -E remove makecapk.apk
  COMMAND zip -D9r ../makecapk.apk . && zip -D0r ../makecapk.apk resources.arsc AndroidManifest.xml
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/apk)

add_custom_target(sign_apk ALL
  DEPENDS zip_apk
  COMMAND jarsigner -sigalg SHA1withRSA -digestalg SHA1 -verbose -keystore ${KEYSTOREFILE} -storepass password makecapk.apk ${ALIASNAME}
  COMMAND ${CMAKE_COMMAND} -E remove ${APKFILE}
  COMMAND echo "ZIP ALIGN"
  COMMAND ${BUILD_TOOLS}/zipalign -v 4 makecapk.apk ${APKFILE}
  COMMAND echo "APK SIGNER"
  COMMAND ${BUILD_TOOLS}/apksigner sign --key-pass pass:${STOREPASS} --ks-pass pass:${STOREPASS} --ks ${KEYSTOREFILE} ${APKFILE}
  COMMAND ${CMAKE_COMMAND} -E remove temp.apk
  COMMAND ${CMAKE_COMMAND} -E remove makecapk.apk
  DEPENDS zip_apk
  COMMENT "Signing apk")
